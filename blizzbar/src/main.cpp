// Pretty message boxes
#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#include "../res/resource.h"
#include "Handle.h"
#include "NotifyIconWindow.h"
#include "Win32Exception.h"
#include "../res/strings.h"
#include <blizzbar_common.h>

#include <gsl/gsl_util>

#include <Windows.h>
#include <Shlobj.h>

#include <string>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <fstream>
#include <locale>
#include <functional>
#include <unordered_map>
#include <cassert>

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

struct ComDeleter
{
	void operator()(void* p) { CoTaskMemFree(p); }
};

fs::path GetExplorerPath()
{
	wchar_t* windowsDirStr;
	const auto res = SHGetKnownFolderPath(FOLDERID_Windows, 0, nullptr, &windowsDirStr);
    if (res != S_OK)
	{
		throw Win32Exception(res, "Failed to get Windows directory path.");
	}
	std::unique_ptr<wchar_t, ComDeleter> windowsDirStrGuard(windowsDirStr);

    fs::path windowsDir{windowsDirStr};

	return fs::canonical(windowsDir / "explorer.exe");
}

std::vector<GameInfo> ParseConfig(std::wistream& file)
{
    const auto explorerExe = GetExplorerPath().wstring();

	// Pipe delimeted file parsing adapted from http://archive.is/HN3vj
	std::vector<GameInfo> result;
	std::wstring line;
	while (std::getline(file, line))
	{
		GameInfo gameInfo;

		const wchar_t* token = line.c_str();
		for (int i = 0; *token; ++i)
		{
			int len = gsl::narrow<int>(std::wcscspn(token, L"|\n"));
			switch (i)
			{
			case 0:
				gameInfo.gameName.sprintf(L"%.*s", len, token);
				break;
			case 1:
				gameInfo.appUserModelId.sprintf(APP_GUID L".%.*s", len, token);
				gameInfo.relaunchCommand.sprintf(L"\"%s\" battlenet://%.*s", explorerExe.c_str(), len, token);
				break;
#ifdef _WIN64
            case 3:
#else
			case 2:
#endif
				gameInfo.exe.sprintf(L"%.*s", len, token);
				break;
			}

			token += len + 1;
		}

        if (gameInfo.exe.size() > 0)
        {
		    result.push_back(gameInfo);
        }
	}

	return result;
}

std::vector<GameInfo> LoadConfig(const fs::path& configDir)
{
	auto file = OpenConfigFile(configDir / CONFIG_FILE_NAME);
	return ParseConfig(file);
}

std::pair<FileMapping, FileMappingView> MapConfigFile(const fs::path& configDir)
{
	auto gameInfoVec = LoadConfig(configDir);

	DWORD mmapGameInfoSize = gsl::narrow<DWORD>(sizeof(GameInfo) * gameInfoVec.size());
	DWORD mmapSize = sizeof(Config::Header) + mmapGameInfoSize;

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
	config->copy_from(gameInfoVec);

	return std::make_pair(std::move(mmap), std::move(view));
}

std::pair<Library, Hook> RegisterHook()
{
	Library hookLib(DLL_NAME);
	FARPROC shellHookHandler = GetProcAddress(hookLib, "ShellHookHandler");
	if (shellHookHandler == nullptr)
	{
		throw Win32Exception("DLL does not contain expected hook handler.");
	}

	Hook hook(WH_SHELL, reinterpret_cast<HOOKPROC>(shellHookHandler), hookLib, 0);
	return {std::move(hookLib), std::move(hook)};
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

struct CloseHandleDeleter
{
	void operator()(HANDLE h) const
	{
		CloseHandle(h);
	}
};

void Main(HINSTANCE instance)
{
	const auto exeDir = GetExeDir();

	// Need to hold this pair of objects so the memory mapped file
	// lives as long as this process is running
	const auto mmapAndView = MapConfigFile(exeDir);

#ifdef _WIN64
	UNREFERENCED_PARAMETER(instance);
	auto hook = RegisterHook();

	auto mutex = Mutex::open(SYNCHRONIZE, false, X64_MUTEX_NAME);
	mutex.wait(INFINITE);
#else
	auto mutex = Mutex::create(nullptr, true, X64_MUTEX_NAME);
	StartSurrogateProcess(exeDir);

	auto hook = RegisterHook();

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
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring message = converter.from_bytes(ex.what());

		MessageBoxW(nullptr, message.c_str(), APP_FRIENDLY_NAME L" (" BIT_MODIFIER L"-bit)", MB_OK | MB_ICONERROR);
		return 1;
	}
}
