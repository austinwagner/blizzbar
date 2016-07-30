// Lightweight hook DLL. Has no linkage against the CRT to minimize size.

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propkey.h>

#include "../BlizzbarCommon.h"

static Config* config = NULL;
static GameInfo* gameInfo = NULL;
static HANDLE mmap = NULL;

void SetWindowAppId(HWND window, const wchar_t* appId)
{
	IPropertyStore* propStore;
	SHGetPropertyStoreForWindow(window, &IID_IPropertyStore, (void**)&propStore);
	PROPVARIANT pv;
	pv.vt = VT_LPWSTR;
	pv.pwszVal = (wchar_t*)appId;
	propStore->lpVtbl->SetValue(propStore, &PKEY_AppUserModel_ID, &pv);
	propStore->lpVtbl->Release(propStore);
}

void ShellHookHandlerInternal(int code, WPARAM wparam, LPARAM lparam)
{
	UNREFERENCED_PARAMETER(lparam);

	if (code == HSHELL_WINDOWCREATED)
	{
		HWND window = (HWND)wparam;
		SetWindowAppId(window, gameInfo->appUserModelId);
	}
}

GameInfo* LookupGameInfoByExeName(const wchar_t* exeName)
{
	for (size_t i = 0; i < config->elemCount; i++)
	{
		GameInfo* gi = &config->gameInfoArr[i];

#ifdef _WIN64
		wchar_t* giExe = gi->exe64;
#else
		wchar_t* giExe = gi->exe32;
#endif

		if (CompareStringOrdinal(exeName, -1, giExe, -1, TRUE) == CSTR_EQUAL)
		{
			return gi;
		}
	}

	return NULL;
}

void CheckIfProcessOfInterest()
{
	DWORD exeDirMaxLen = 512;
	wchar_t* path = HeapAlloc(GetProcessHeap(), 0, exeDirMaxLen * sizeof(wchar_t));
	if (path == NULL) return;
	DWORD pathLen = GetModuleFileNameW(NULL, path, exeDirMaxLen);

	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		HeapFree(GetProcessHeap(), 0, path);
		exeDirMaxLen *= 2;
		path = HeapAlloc(GetProcessHeap(), 0, exeDirMaxLen * sizeof(wchar_t));
		if (path == NULL) return;
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
	gameInfo = LookupGameInfoByExeName(path + pathLen);
}

__declspec(dllexport) LRESULT ShellHookHandler(int code, WPARAM wparam, LPARAM lparam)
{
	if (gameInfo != NULL && code >= 0)
	{
		ShellHookHandlerInternal(code, wparam, lparam);
	}

	return CallNextHookEx(NULL, code, wparam, lparam);
}

BOOL MapGameInfoArr()
{
	mmap = OpenFileMappingW(
		FILE_MAP_READ,
		FALSE,
		MMAP_NAME);
	if (mmap == NULL) return FALSE;

	void* mmapView = MapViewOfFile(
		mmap,
		FILE_MAP_READ,
		0,
		0,
		sizeof(Config));
	if (mmapView == NULL)
	{
		CloseHandle(mmap);
		mmap = NULL;
		return FALSE;
	}

	config = (Config*)mmapView;
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
		CloseHandle(mmap);
		mmap = NULL;
		return FALSE;
	}

	config = (Config*)mmapView;

	return TRUE;
}

BOOL WINAPI _DllMainCRTStartup(HANDLE hDllHandle, DWORD dwReason, LPVOID lpReserved)
{
	UNREFERENCED_PARAMETER(hDllHandle);
	UNREFERENCED_PARAMETER(lpReserved);

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (MapGameInfoArr())
		{
			CheckIfProcessOfInterest();
		}
		break;
	}
	case DLL_PROCESS_DETACH:
		if (config != NULL)
		{
			UnmapViewOfFile(config);
		}

		if (mmap != NULL)
		{
			CloseHandle(mmap);
		}
		break;
	}

	return TRUE;
}