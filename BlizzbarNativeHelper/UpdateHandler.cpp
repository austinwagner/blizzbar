#include "UpdateHandler.h"

#include <string>

#include "Handle.h"
#include "Util.h"
#include "../BlizzbarCommon.h"
#include "numeric_cast.h"
#include <memory>
#include <unordered_map>
#include <cassert>
#include <filesystem>
#include <cstdlib>

#include <shellapi.h>
#include <propsys.h>
#include <propkey.h>

namespace fs = std::experimental::filesystem;

extern "C" 
{
	struct UpdateHandlerResult
	{
		UpdateHandler* updateHandler = nullptr;
		wchar_t* errorMessage = nullptr;
	};

	__declspec(dllexport) UpdateHandlerResult NewUpdateHandler()
	{
		UpdateHandlerResult result;

		try
		{
			result.updateHandler = new UpdateHandler();
		}
		catch (const std::exception& ex)
		{
			auto wwhat = utf8_to_utf16(ex.what());
			result.errorMessage = new wchar_t[wwhat.size() + 1];
			wcscpy_s(result.errorMessage, wwhat.size() + 1, wwhat.c_str());
		}

		return result;
	}

	__declspec(dllexport) void FreeUpdateHandler(UpdateHandler* uh)
	{
		delete uh;
	}

	__declspec(dllexport) void FreeErrorMessage(wchar_t err[])
	{
		delete[] err;
	}

	__declspec(dllexport) wchar_t* SetGameInfo(UpdateHandler* uh, const GameInfo gi[], size_t ngi)
	{
		wchar_t* result = nullptr;

		try
		{
			uh->SetGameInfo(gi, ngi);
		}
		catch (const std::exception& ex)
		{
			auto wwhat = utf8_to_utf16(ex.what());
			result = new wchar_t[wwhat.size() + 1];
			wcscpy_s(result, wwhat.size() + 1, wwhat.c_str());
		}

		return result;
	}
}

UpdateHandler::UpdateHandler() :
	writeLock_(Mutex::create(nullptr, false, MMAP_WRITE_MUTEX_NAME)),
	syncMap_(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, sizeof(MmapSyncData), MMAP_SYNC_NAME),
	syncMapView_(syncMap_, FILE_MAP_READ | FILE_MAP_WRITE, 0, sizeof(MmapSyncData)),
	mmapSyncData_(syncMapView_.as<MmapSyncData>()),
	mmap_(new FileMapping(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, sizeof(Config), MMAP_NAME_1)),
	mmapView_(new FileMappingView(*mmap_, FILE_MAP_READ | FILE_MAP_WRITE, 0, sizeof(Config))),
	mappedConfig_(mmapView_->as<Config>())
{
	mappedConfig_->elemCount = 0;
	mmapSyncData_->numReaders = 0;
	wcscpy_s(mmapSyncData_->mmapName, MMAP_NAME_1);
}

void UpdateHandler::SetGameInfo(const GameInfo gameInfoArr[], size_t gameInfoArrLen)
{
	{
		auto lock = writeLock_.acquire();

		DWORD mmapGameInfoSize = numeric_cast<DWORD>(sizeof(GameInfo) * gameInfoArrLen);
		DWORD mmapSize = sizeof(Config) + mmapGameInfoSize;

		auto newMmap = std::make_unique<FileMapping>(
			INVALID_HANDLE_VALUE,
			nullptr,
			PAGE_READWRITE,
			mmapSize,
			mmapSyncData_->mmapName[_countof(MMAP_NAME_1) - 1] == L'1' ? MMAP_NAME_2 : MMAP_NAME_1);

		auto newView = std::make_unique<FileMappingView>(
			*newMmap,
			FILE_MAP_READ | FILE_MAP_WRITE,
			0,
			mmapSize);

		auto newConfig = newView->as<Config>();

		newConfig->elemCount = gameInfoArrLen;
		std::memcpy(newConfig->gameInfoArr, gameInfoArr, mmapGameInfoSize);

		mmapView_.reset();
		mmap_.reset();

		mmap_ = std::move(newMmap);
		mmapView_ = std::move(newView);
		mappedConfig_ = newConfig;
	}

	ChangeExistingWindowsAppIds();
}

struct EnumWindowsCallbackParam
{
	std::unordered_map<DWORD, const GameInfo*> checkedProcesses;
	const Config* config = nullptr;
};

void SetWindowData(HWND window, const GameInfo* gameInfo)
{
	IPropertyStore* propStore;
	SHGetPropertyStoreForWindow(window, IID_IPropertyStore, reinterpret_cast<void**>(&propStore));

	PROPVARIANT pv;
	pv.vt = VT_LPWSTR;

	pv.pwszVal = const_cast<wchar_t*>(gameInfo->appUserModelId);
	propStore->SetValue(PKEY_AppUserModel_ID, pv);

	pv.pwszVal = const_cast<wchar_t*>(gameInfo->relaunchCommand);
	propStore->SetValue(PKEY_AppUserModel_RelaunchCommand, pv);

	wchar_t title[128];
	title[0] = L'0';
	if (GetWindowTextW(window, title, _countof(title)) == 0)
	{
		wcscpy_s(title, _countof(title), L"Unknown Game");
	}

	pv.pwszVal = static_cast<wchar_t*>(title);
	propStore->SetValue(PKEY_AppUserModel_RelaunchDisplayNameResource, pv);

	propStore->Release();
}

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
			SetWindowData(hwnd, checkedProcessesIter->second);
		}

		return TRUE;
	}

	GenericHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, pid));
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
			SetWindowData(hwnd, gameInfo);
			param->checkedProcesses[pid] = gameInfo;
			return TRUE;
		}
	}

	param->checkedProcesses[pid] = nullptr;
	return TRUE;
}

void UpdateHandler::ChangeExistingWindowsAppIds() const
{
	EnumWindowsCallbackParam param;
	param.config = mappedConfig_;
	EnumWindows(EnumWindowsCallback, reinterpret_cast<LPARAM>(&param));
}