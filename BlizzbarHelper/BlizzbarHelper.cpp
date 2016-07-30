// Pretty message boxes
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include <Windows.h>
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
#include <functional>
#include <unordered_map>
#include <cassert>

#include "Resource.h"
#include "Handle.h"
#include "numeric_cast.h"
#include "NotifyIconWindow.h"
#include "Strings.h"
#include "../BlizzbarCommon.h"

namespace fs = std::experimental::filesystem;

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

	Config* config = view.as<Config>();
	config->elemCount = gameInfoVec.size();
	std::memcpy(config->gameInfoArr, gameInfoVec.data(), mmapGameInfoSize);

	return std::make_pair(std::move(mmap), std::move(view));
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
	std::unordered_map<DWORD, const GameInfo*> checkedProcesses;
	const Config* config;
};

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam)
{
	EnumWindowsCallbackParam* param = reinterpret_cast<EnumWindowsCallbackParam*>(lParam);
	assert(param != nullptr && "EnumWindowsCallback missing EnumWindowsCallbackParam");

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
		auto gameInfo = &param->config->gameInfoArr[i];
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

void ChangeExistingWindowsAppIds(const Config* config)
{
	EnumWindowsCallbackParam param;
	param.config = config;
	EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&param));
}

void Main(HINSTANCE instance)
{
	auto exeDir = GetExeDir();
	auto mmapAndView = MapConfigFile(exeDir);

#ifdef _WIN64
	auto hook = RegisterHook();

	auto mutex = Mutex::open(SYNCHRONIZE, false, X64_MUTEX_NAME);
	mutex.wait(INFINITE);
#else
	auto mutex = Mutex::create(nullptr, true, X64_MUTEX_NAME);
	StartSurrogateProcess(exeDir);

	auto hook = RegisterHook();

	ChangeExistingWindowsAppIds(mmapAndView.second.as<Config>());

	NotifyIconWindow window(instance);
	window.runMessageLoop();
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

		Main(instance);

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