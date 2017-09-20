#include <Windows.h>
#include "../BlizzbarCommon.h"

#define ELLIFY_IMPL(x) L ## x
#define ELLIFY(x) ELLIFY_IMPL(x)
#define DLL_NAME L"BlizzbarHooks64.dll"
#define FUNC_NAME "ShellHookHandler"
#define LFUNC_NAME ELLIFY(FUNC_NAME)
#define TITLE L"Blizzbar 64-bit Surrogate"

void ErrorMessage(LPCWSTR message) {
	const auto err = GetLastError();
	if (err == ERROR_SUCCESS) {
		MessageBoxW(nullptr, message, TITLE, MB_OK | MB_ICONERROR);
		return;
	}

	LPWSTR sysErrMsg;

	const auto sysErrMsgLen = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPTSTR>(&sysErrMsg),
		0,
		nullptr
	);
	if (sysErrMsgLen == 0) {
		MessageBoxW(nullptr, message, TITLE, MB_OK | MB_ICONERROR);
		return;
	}
	SCOPE_EXIT{ LocalFree(sysErrMsg); };

	std::wstring errMsg(message);
	errMsg += L"\n";
	errMsg += sysErrMsg;

	MessageBoxW(nullptr, errMsg.data(), TITLE, MB_OK | MB_ICONERROR);
}

int WINAPI wWinMain(
	_In_ HINSTANCE inst,
	_In_opt_ HINSTANCE prevInst,
	_In_ LPWSTR cmdLine,
	_In_ int showCmd)
{
	UNREFERENCED_PARAMETER(inst);
	UNREFERENCED_PARAMETER(prevInst);
	UNREFERENCED_PARAMETER(cmdLine);
	UNREFERENCED_PARAMETER(showCmd);

	const auto singleInstMutex = CreateMutexW(nullptr, FALSE, L"Local\\" APP_GUID L".surrogate_single_instance");
	if (!singleInstMutex) {
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			return 2;
		}

		ErrorMessage(L"Failed to ensure single instance.");
		return 1;
	}
	SCOPE_EXIT{ CloseHandle(singleInstMutex); };

	const auto barrier = OpenMutexW(SYNCHRONIZE, FALSE, L"Local\\" APP_GUID L".surrogate_barrier");
	if (!barrier) {
		ErrorMessage(L"Failed to open surrogate barrier mutex.");
		return 1;
	}
	SCOPE_EXIT{ CloseHandle(barrier); };

	const auto lib = LoadLibraryW(DLL_NAME);
	if (!lib) {
		ErrorMessage(L"Could not find " DLL_NAME L".");
		return 1;
	}
	SCOPE_EXIT{ FreeLibrary(lib); };

	const auto hookProc = reinterpret_cast<HOOKPROC>(GetProcAddress(lib, FUNC_NAME));
	if (!hookProc) {
		ErrorMessage(L"Could not find method " LFUNC_NAME L" in " DLL_NAME L".");
		return 1;
	}

	const auto hook = SetWindowsHookExW(WH_SHELL, hookProc, lib, 0);
	if (!hook) {
		ErrorMessage(L"Failed to install Windows hook.");
		return 1;
	}
	SCOPE_EXIT{
		UnhookWindowsHookEx(hook);
		// Wake all message loops to get Windows to release the DLL
		SendMessageTimeoutW(HWND_BROADCAST, WM_NULL, 0, 0, SMTO_ABORTIFHUNG | SMTO_NOTIMEOUTIFNOTHUNG, 1000, nullptr);
	};

	if (WaitForSingleObject(barrier, INFINITE) != WAIT_OBJECT_0) {
		// Message box is annoying here. If the main process is killed, there's no reason for this one
		// not to exit silently.
		return 1;
	}

	return 0;
}
