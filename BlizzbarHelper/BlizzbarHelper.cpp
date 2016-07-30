// Pretty message boxes
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propkey.h>

#include <string>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <fstream>
#include <locale>
#include <limits>
#include <functional>
#include <unordered_map>

#include "Resource.h"
#include "Handle.h"
#include "numeric_cast.h"
#include "NotifyIcon.h"
#include "WindowClass.h"
#include "../BlizzbarCommon.h"

namespace fs = std::experimental::filesystem;

const UINT NotifyIconCallbackMessage = WM_USER + 1;

#define APP_NAME L"BlizzbarHelper"
#define APP_FRIENDLY_NAME L"Blizzbar Helper"

#define CONFIG_FILE_NAME L"games.txt"

#define SURROGATE_EXE APP_NAME L"64.exe"

#define DLL_NAME L"BlizzbarHooks" BIT_MODIFIER L".dll"

#define MUTEX_PREFIX L"net.austinwagner.blizzbar."

#define X64_MUTEX_NAME MUTEX_PREFIX L"sync_exit"

#define SINGLE_INSTANCE_MUTEX_NAME MUTEX_PREFIX L"single." BIT_MODIFIER

// Globals for WndProc
static HINSTANCE g_instance;
static Window* g_messageWindow;

std::wifstream OpenConfigFile(const fs::path& path)
{
	// Automatic UTF-8 to UTF-16 conversion http://stackoverflow.com/a/4776366
	const std::locale empty_locale = std::locale::empty();
	typedef std::codecvt_utf8<wchar_t> converter_type;
	const converter_type* converter = new converter_type;
	const std::locale utf8_locale = std::locale(empty_locale, converter);

	std::wifstream file(path.wstring());
	if (file.fail())
	{
		throw std::runtime_error("Config file not found.");
	}

	file.imbue(utf8_locale);
	return file;
}

std::vector<GameInfo> ParseConfig(std::wistream& file)
{
	// Pipe delimeted file parsing adapted from http://archive.is/HN3vj
	std::vector<GameInfo> result;
	std::wstring line;
	while (std::getline(file, line))
	{
		GameInfo gameInfo;
		gameInfo.exe32[0] = L'\0';
		gameInfo.exe64[0] = L'\0';

		const wchar_t* token = line.c_str();
		for (int i = 0; *token; ++i)
		{
			int len = numeric_cast<int>(std::wcscspn(token, L"|\n"));
			switch (i)
			{
			case 1:
				swprintf_s(gameInfo.appUserModelId, APP_GUID L".%.*s", len, token);
				break;
			case 4:
				swprintf_s(gameInfo.exe32, L"%.*s", len, token);
			case 5:
				swprintf_s(gameInfo.exe64, L"%.*s", len, token);
			}

			token += len + 1;
		}

		result.push_back(gameInfo);
	}

	return result;
}

std::vector<GameInfo> LoadConfig(const fs::path& configDir)
{
	auto file = OpenConfigFile(configDir / CONFIG_FILE_NAME);
	return ParseConfig(file);
}

// Reads the configuration file into a memory-mapped file
// as a C structure: 
//
// struct Config {
//     size_t elemCount;
//     GameInfo gameInfoArr[];
// };
std::pair<FileMapping, FileMappingView> MapConfigFile(const fs::path& configDir)
{
	auto gameInfoVec = LoadConfig(configDir);

	DWORD mmapGameInfoSize = numeric_cast<DWORD>(sizeof(GameInfo) * gameInfoVec.size());
	DWORD mmapSize = sizeof(Config) + mmapGameInfoSize;

	FileMapping mmap(
		INVALID_HANDLE_VALUE,
		nullptr,
		PAGE_READWRITE,
		mmapSize,
		MMAP_NAME);

	FileMappingView view(
		mmap,
		FILE_MAP_WRITE,
		0,
		mmapSize);

	Config* config = reinterpret_cast<Config*>(view.get());
	config->elemCount = gameInfoVec.size();
	std::memcpy(config->gameInfoArr, gameInfoVec.data(), mmapGameInfoSize);

	return std::make_pair(std::move(mmap), std::move(view));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		switch (wmId)
		{
		case IDM_EXIT:
			PostQuitMessage(0);
			break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	case NotifyIconCallbackMessage:
	{
		switch (LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
		{
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			HMENU menu = LoadMenuW(g_instance, MAKEINTRESOURCEW(IDC_BLIZZBARHELPER));
			HMENU subMenu = GetSubMenu(menu, 0);

			UINT menuAlignment =
				GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0 ?
				TPM_LEFTALIGN :
				TPM_RIGHTALIGN;

			// SetForegroundWindow and PostMessage are needed to prevent the context menu from getting stuck
			SetForegroundWindow(*g_messageWindow);
			TrackPopupMenu(subMenu, menuAlignment | TPM_BOTTOMALIGN, cursorPos.x, cursorPos.y, 0, hWnd, nullptr);
			PostMessageW(*g_messageWindow, WM_NULL, 0, 0);

			DestroyMenu(menu);
		}
		break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	default:
		return DefWindowProcW(hWnd, message, wParam, lParam);

	}
	return 0;
}

Hook RegisterHook()
{
	Library hookLib(DLL_NAME);
	FARPROC shellHookHandler = GetProcAddress(hookLib, "ShellHookHandler");
	if (shellHookHandler == nullptr)
	{
		throw Win32Exception("DLL does not contain expected hook handler.");
	}

	return Hook(WH_SHELL, reinterpret_cast<HOOKPROC>(shellHookHandler), hookLib, 0);
}

std::wstring GetUnknownLengthString(std::function<void(wchar_t*, DWORD)> f)
{
	DWORD strLen = 512;
	std::unique_ptr<wchar_t[]> strPtr(new wchar_t[strLen]);
	f(strPtr.get(), strLen);

	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		strLen *= 2;
		strPtr.reset(new wchar_t[strLen]);
		f(strPtr.get(), strLen);
	}

	return std::wstring(strPtr.get());
}

fs::path GetExeDir()
{
	std::wstring exePathStr = GetUnknownLengthString([](wchar_t* str, DWORD len) { GetModuleFileNameW(nullptr, str, len); });
	fs::path exePath(exePathStr);
	return exePath.parent_path();
}

bool Is64BitWindows()
{
#ifdef _WIN64
	return true;
#else
	// As documented, 32-bit Windows won't have the IsWow64Process, so it needs to be loaded dynamically
	typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
	auto kernel32 = GetModuleHandleW(L"kernel32");
	auto isWow64Process = reinterpret_cast<LPFN_ISWOW64PROCESS>(GetProcAddress(kernel32, "IsWow64Process"));
	if (isWow64Process == nullptr)
	{
		return false;
	}

	BOOL is64BitWindows;
	if (!isWow64Process(GetCurrentProcess(), &is64BitWindows))
	{
		throw Win32Exception("Cannot determine if running on 64-bit Windows.");
	}

	return is64BitWindows == TRUE;
#endif
}

void StartSurrogateProcess(const fs::path& exeDir)
{
	if (!Is64BitWindows())
	{
		return;
	}

	fs::path path = exeDir / SURROGATE_EXE;

	STARTUPINFOW si = { 0 };
	si.cb = sizeof(STARTUPINFOW);
	si.dwFlags = STARTF_FORCEOFFFEEDBACK;

	PROCESS_INFORMATION pi;
	if (!CreateProcessW(path.wstring().c_str(), nullptr, nullptr, nullptr, false, 0, nullptr, nullptr, &si, &pi))
	{
		throw Win32Exception("Failed to start surrogate process.");
	}
}

void SetWindowAppId(HWND window, const wchar_t* appId)
{
	IPropertyStore* propStore;
	SHGetPropertyStoreForWindow(window, IID_IPropertyStore, (void**)&propStore);
	PROPVARIANT pv;
	pv.vt = VT_LPWSTR;
	pv.pwszVal = (wchar_t*)appId;
	propStore->SetValue(PKEY_AppUserModel_ID, pv);
	propStore->Release();
}

struct CloseHandleDeleter
{
	void operator()(HANDLE h) const
	{
		CloseHandle(h);
	}
};

struct EnumWindowsCallbackParam
{
	std::unordered_map<DWORD, GameInfo*> checkedProcesses;
	Config* config;
};

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	EnumWindowsCallbackParam* param = reinterpret_cast<EnumWindowsCallbackParam*>(lParam);
	DWORD pid;
	GetWindowThreadProcessId(hwnd, &pid);

	auto checkedProcessesIter = param->checkedProcesses.find(pid);
	if (checkedProcessesIter != param->checkedProcesses.end())
	{
		if (checkedProcessesIter->second != nullptr)
		{
			SetWindowAppId(hwnd, checkedProcessesIter->second->appUserModelId);
		}

		return TRUE;
	}

	Handle<HANDLE, CloseHandleDeleter> process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
	if (process.empty())
	{
		return TRUE;
	}

	std::wstring exePathStr = GetUnknownLengthString([&](wchar_t* str, DWORD len) { QueryFullProcessImageNameW(process, 0, str, &len); });
	fs::path exePath(exePathStr);
	std::wstring exeName = exePath.stem().wstring();

	for (size_t i = 0; i < param->config->elemCount; ++i)
	{
		GameInfo* gameInfo = &param->config->gameInfoArr[i];
		if (_wcsicmp(exeName.c_str(), gameInfo->exe32) == 0 || _wcsicmp(exeName.c_str(), gameInfo->exe64) == 0)
		{
			SetWindowAppId(hwnd, gameInfo->appUserModelId);
			param->checkedProcesses[pid] = gameInfo;
			return TRUE;
		}
	}

	param->checkedProcesses[pid] = nullptr;
	return TRUE;
}

void PlatformMain(const fs::path& exeDir, FileMappingView& view)
{
#ifdef _WIN64
	UNREFERENCED_PARAMETER(exeDir);
	UNREFERENCED_PARAMETER(view);

	auto hook = RegisterHook();

	auto mutex = Mutex::open(SYNCHRONIZE, false, X64_MUTEX_NAME);
	mutex.wait(INFINITE);
#else
	WindowClass windowClass(g_instance, WndProc, L"BlizzbarHidden");

	Window window(
		0, windowClass.className().c_str(), APP_NAME, 0,
		0, 0, 0, 0, HWND_MESSAGE, nullptr, g_instance, nullptr);

	g_messageWindow = &window;

	NotifyIcon notifyIcon(
		window,
		NotifyIconCallbackMessage,
		LoadIconW(g_instance, MAKEINTRESOURCEW(IDI_BLIZZBAR)),
		APP_FRIENDLY_NAME);

	auto mutex = Mutex::create(nullptr, true, X64_MUTEX_NAME);

	StartSurrogateProcess(exeDir);

	auto hook = RegisterHook();

	{
		EnumWindowsCallbackParam param;
		param.config = reinterpret_cast<Config*>(view.get());
		EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&param));
	}

	MSG msg;
	while (GetMessageW(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
#endif
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int)
{
	try
	{
		auto mutex = Mutex::create(nullptr, false, SINGLE_INSTANCE_MUTEX_NAME);
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			return 1;
		}

		g_instance = instance;

		auto exeDir = GetExeDir();
		auto mmapAndView = MapConfigFile(exeDir);
		PlatformMain(exeDir, mmapAndView.second);

		return 0;
	}
	catch (std::exception& ex)
	{
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		std::wstring message = converter.from_bytes(ex.what());

		MessageBoxW(nullptr, message.c_str(), APP_FRIENDLY_NAME L" (" BIT_MODIFIER L"-bit)", MB_OK | MB_ICONERROR);
		return 1;
	}
}