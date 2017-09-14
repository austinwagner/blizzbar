// Lightweight hook DLL. Has no linkage against the CRT to minimize size.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propkey.h>

#include "../BlizzbarCommon.h"

BOOL isProcessOfInterest = FALSE;
GameInfo gameInfo;

errno_t my_wcscpy_s(wchar_t* dest, rsize_t len, const wchar_t* src)
{
	for (rsize_t i = 0; i < len; ++i)
	{
		dest[i] = src[i];
		if (src[i] == L'\0')
		{
			break;
		}
	}

	return 0;
}

void SetWindowData(HWND window)
{
	IPropertyStore* propStore;
	SHGetPropertyStoreForWindow(window, &IID_IPropertyStore, (void**)&propStore);
	PROPVARIANT pv;
	pv.vt = VT_LPWSTR;

	pv.pwszVal = (wchar_t*)gameInfo.appUserModelId;
	propStore->lpVtbl->SetValue(propStore, &PKEY_AppUserModel_ID, &pv);

	pv.pwszVal = (wchar_t*)gameInfo.relaunchCommand;
	propStore->lpVtbl->SetValue(propStore, &PKEY_AppUserModel_RelaunchCommand, &pv);

	wchar_t title[128];
	title[0] = L'0';
	if (GetWindowTextW(window, title, sizeof(title) / sizeof(wchar_t)) == 0)
	{
		my_wcscpy_s(title, sizeof(title) / sizeof(wchar_t), L"Unknown Game");
	}

	pv.pwszVal = (wchar_t*)title;
	propStore->lpVtbl->SetValue(propStore, &PKEY_AppUserModel_RelaunchDisplayNameResource, &pv);

	propStore->lpVtbl->Release(propStore);
}

void ShellHookHandlerInternal(int code, WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(lparam);

	if (code == HSHELL_WINDOWCREATED)
	{
		HWND window = (HWND)wparam;
		SetWindowData(window);
	}
}

const GameInfo* LookupGameInfoByExeName(const wchar_t* exeName, const Config* config)
{
	for (size_t i = 0; i < config->elemCount; i++)
	{
		const GameInfo* gi = &config->gameInfoArr[i];

#ifdef _WIN64
		const wchar_t* giExe = gi->exe64;
#else
		const wchar_t* giExe = gi->exe32;
#endif

		if (CompareStringOrdinal(exeName, -1, giExe, -1, TRUE) == CSTR_EQUAL)
		{
			return gi;
		}
	}

	return NULL;
}

__declspec(dllexport) LRESULT ShellHookHandler(int code, WPARAM wparam, LPARAM lparam)
{
	if (isProcessOfInterest && code >= 0)
	{
		ShellHookHandlerInternal(code, wparam, lparam);
	}

	return CallNextHookEx(NULL, code, wparam, lparam);
}

HANDLE AcquireReleaseReaderLock(BOOL acquire, wchar_t mmapName[sizeof(MMAP_NAME_1)], HANDLE existingLock)
{
	HANDLE result = NULL;

	HANDLE readMutex = OpenMutexW(SYNCHRONIZE, FALSE, MMAP_READ_MUTEX_NAME);
	if (readMutex == NULL)
	{
		return result;
	}

	if (WaitForSingleObject(readMutex, 1000) != WAIT_OBJECT_0)
	{
		goto cleanup1;
	}
	
	HANDLE syncMap = OpenFileMappingW(
		FILE_MAP_READ,
		FALSE,
		MMAP_SYNC_NAME
	);
	if (syncMap == NULL)
	{
		goto cleanup2;
	}

	void* syncMapView = MapViewOfFile(
		syncMap,
		FILE_MAP_READ,
		0,
		0,
		sizeof(MmapSyncData));
	if (syncMapView == NULL)
	{
		goto cleanup3;
	}

	MmapSyncData* syncData = (MmapSyncData*)syncMapView;
	if (acquire)
	{
		syncData->numReaders++;
		my_wcscpy_s(mmapName, sizeof(MMAP_NAME_1), syncData->mmapName);
		if (syncData->numReaders == 1)
		{
			result = OpenMutexW(SYNCHRONIZE, FALSE, MMAP_WRITE_MUTEX_NAME);
			if (result == NULL)
			{
				goto cleanup4;
			}

			if (WaitForSingleObject(existingLock, 5000) != WAIT_OBJECT_0)
			{
				CloseHandle(result);
				result = NULL;
			}
		}
	}
	else
	{
		syncData->numReaders--;
		if (syncData->numReaders == 0)
		{
			ReleaseMutex(existingLock);
			CloseHandle(existingLock);
		}
	}

cleanup4:
	UnmapViewOfFile(syncMapView);
cleanup3:
	CloseHandle(syncMap);
cleanup2:
	ReleaseMutex(readMutex);
cleanup1:
	CloseHandle(readMutex);

	return result;
}

HANDLE AcquireReaderLock(wchar_t mmapName[sizeof(MMAP_NAME_1)])
{
	return AcquireReleaseReaderLock(TRUE, mmapName, NULL);
}

void ReleaseReaderLock(HANDLE lockHandle)
{
	AcquireReleaseReaderLock(FALSE, NULL, lockHandle);
}

void CheckIfProcessOfInterest()
{
	wchar_t mmapName[sizeof(MMAP_NAME_1)];
	HANDLE lockHandle = AcquireReaderLock(mmapName);
	if (lockHandle == NULL)
	{
		return;
	}

	HANDLE mmap = OpenFileMappingW(
		PAGE_READONLY,
		FALSE,
		mmapName);
	if (mmap == NULL)
	{
		goto cleanup1;
	}

	void* mmapView = MapViewOfFile(
		mmap,
		FILE_MAP_READ,
		0,
		0,
		sizeof(Config));
	if (mmapView == NULL)
	{
		goto cleanup2;
	}

	Config* config = (Config*)mmapView;
	size_t elemCount = config->elemCount;

	UnmapViewOfFile(mmapView);

	mmapView = MapViewOfFile(
		mmap,
		FILE_MAP_READ,
		0,
		0,
		sizeof(Config) + (sizeof(GameInfo) * elemCount));
	if (mmapView == NULL)
	{
		goto cleanup2;
	}

	DWORD exeDirMaxLen = 512;
	wchar_t* path = HeapAlloc(GetProcessHeap(), 0, exeDirMaxLen * sizeof(wchar_t));
	if (path == NULL)
	{
		goto cleanup3;
	}

	DWORD pathLen = GetModuleFileNameW(NULL, path, exeDirMaxLen);
	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		HeapFree(GetProcessHeap(), 0, path);
		exeDirMaxLen *= 2;
		path = HeapAlloc(GetProcessHeap(), 0, exeDirMaxLen * sizeof(wchar_t));

		if (path == NULL)
		{
			goto cleanup3;
		}

		pathLen = GetModuleFileNameW(NULL, path, exeDirMaxLen);
	}

	// C:\foo\bar.exe0 -> C:\foo\bar0exe0
	pathLen -= 4;
	path[pathLen] = L'\0';

	pathLen--;
	while (pathLen != DWORD_MAX && path[pathLen] != L'\\')
	{
		pathLen--;
	}
	pathLen++;

	// C:\foo\bar0exe0
	// |------^ pathLen 
	const GameInfo* gameInfoPtr = LookupGameInfoByExeName(path + pathLen, config);
	if (gameInfoPtr == NULL)
	{
		goto cleanup4;
	}

	gameInfo = *gameInfoPtr;
	isProcessOfInterest = TRUE;

cleanup4:
	HeapFree(GetProcessHeap(), 0, path);
cleanup3:
	UnmapViewOfFile(mmapView);
cleanup2:
	CloseHandle(mmap);
cleanup1:
	ReleaseReaderLock(lockHandle);
}

BOOL WINAPI _DllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hDllHandle);
	UNREFERENCED_PARAMETER(lpReserved);

	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CheckIfProcessOfInterest();
	}

	return TRUE;
}