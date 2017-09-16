// Lightweight 64-bit hook injector. Has no linkage against the CRT because it barely does anything.

#include <Windows.h>
#include "../BlizzbarCommon.h"

#define ELLIFY_IMPL(x) L ## x
#define ELLIFY(x) ELLIFY_IMPL(x)
#define DLL_NAME L"BlizzbarHooks64.dll"
#define FUNC_NAME "ShellHookHandler"
#define LFUNC_NAME ELLIFY(FUNC_NAME)
#define TITLE L"Blizzbar 64-bit Surrogate"

inline void ErrorMessage(LPCWSTR message) {
	MessageBoxW(NULL, message, TITLE, MB_OK | MB_ICONERROR);
}

void main() {
	UINT exitCode = 1;

	const HANDLE singleInstMutex = CreateMutexW(NULL, FALSE, L"Local\\" APP_GUID L".surrogate_single_instance");
	if (!singleInstMutex) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			exitCode = 2;
		} else {
			ErrorMessage(L"Failed to ensure single instance.");
		}
		
		goto cleanup0;
	}

	const HANDLE barrier = OpenMutexW(GENERIC_READ, FALSE, L"Local\\" APP_GUID L".surrogate_barrier");
	if (!barrier) {
		ErrorMessage(L"Failed to open surrogate barrier mutex.");
		goto cleanup1;
	}

	const HMODULE lib = LoadLibraryW(DLL_NAME);
	if (!lib) {
		ErrorMessage(L"Could not find " DLL_NAME L".");
		goto cleanup2;
	}

	const HOOKPROC hookProc = (HOOKPROC)GetProcAddress(lib, FUNC_NAME);
	if (!hookProc) {
		FreeLibrary(lib);
		ErrorMessage(L"Could not find method " LFUNC_NAME L" in " DLL_NAME L".");
		goto cleanup3;
	}

	const HHOOK hook = SetWindowsHookW(WH_SHELL, hookProc);
	if (!hook) {
		ErrorMessage(L"Failed to install Windows hook.");
		goto cleanup3;
	}

	if (WaitForSingleObject(barrier, INFINITE) != WAIT_OBJECT_0) {
		ErrorMessage(L"Process exited unexpectedly.");
		goto cleanup4;
	}

	exitCode = 0;

cleanup4:
	UnhookWindowsHookEx(hook);
	// Wake all message loops to get Windows to release the DLL
	SendMessageTimeoutW(HWND_BROADCAST, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000, NULL);
cleanup3:
	FreeLibrary(lib);
cleanup2:
	CloseHandle(barrier);
cleanup1:
	CloseHandle(singleInstMutex);
cleanup0:
	ExitProcess(exitCode);
}
