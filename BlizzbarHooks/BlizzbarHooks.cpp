#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propkey.h>
#include <comdef.h>
#include <comip.h>

#include "../BlizzbarCommon.h"
#include <stdlib.h>
#include <array>
#include <regex>
#include <optional>

bool g_isProcessOfInterest = false;
std::wstring g_appUserModelId;
std::wstring g_relaunchCommand;
std::wstring g_iconPath;

_COM_SMARTPTR_TYPEDEF(IPropertyStore, __uuidof(IPropertyStore));

void SetWindowData(HWND window)
{
    IPropertyStorePtr propStore;
    SHGetPropertyStoreForWindow(window, IID_IPropertyStore, reinterpret_cast<void**>(&propStore));
    PROPVARIANT pv;
    pv.vt = VT_LPWSTR;

    pv.pwszVal = g_appUserModelId.data();
    propStore->SetValue(PKEY_AppUserModel_ID, pv);

    pv.pwszVal = g_relaunchCommand.data();
    propStore->SetValue(PKEY_AppUserModel_RelaunchCommand, pv);

    pv.pwszVal = g_iconPath.data();
    propStore->SetValue(PKEY_AppUserModel_RelaunchIconResource, pv);

    std::array<wchar_t, 128> title;
    title[0] = L'\0';
    if (GetWindowTextW(window, title.data(), title.size()) == 0)
    {
        wcscpy_s(title.data(), title.size(), L"Unknown Game");
    }

    pv.pwszVal = title.data();
    propStore->SetValue(PKEY_AppUserModel_RelaunchDisplayNameResource, pv);

    pv.vt = VT_BOOL;
    pv.boolVal = VARIANT_FALSE;
    propStore->SetValue(PKEY_AppUserModel_PreventPinning, pv);
}

void ShellHookHandlerInternal(int code, WPARAM wparam, LPARAM lparam)
{
    UNREFERENCED_PARAMETER(lparam);

    if (code == HSHELL_WINDOWCREATED)
    {
        const auto window = reinterpret_cast<HWND>(wparam);
        SetWindowData(window);
    }
}

// Layout is [regex_len][regex_strz][short_name_len][short_namez]...0
// Regex will be normalized to have \ as the path separator
std::optional<std::wstring> LookupGameInfoByExeName(const std::wstring& fullPath, const std::byte* config)
{
    while (true)
    {
        // Read 2 bytes to get the regex string length (excluding terminator)
        const auto regexLen = *reinterpret_cast<const uint16_t*>(config);

        // A length of 0 indicates that we've reached the end of the written memory
        if (regexLen == 0)
        {
            return std::nullopt;
        }

        // Move 2 bytes forward to get to the 0 terminated regex string
        config += sizeof(uint16_t);

        // This line is obvious but looks naked without a comment :)
        const std::wregex regex(
            reinterpret_cast<const wchar_t*>(config),
            std::regex_constants::ECMAScript | std::regex_constants::icase);

        // Move past the regex (+1 for terminator) to get to the short-name length
        config += (regexLen + 1) * sizeof(wchar_t);

        // Read 2 bytes to get the short-name length
        const auto shortNameLen = *reinterpret_cast<const uint16_t*>(config);

        // Move 2 bytes forward to get to the 0 terminated short-name string
        config += sizeof(uint16_t);

        // If our path matches the regex, return the short-name string
        if (regex_match(fullPath.begin(), fullPath.end(), regex))
        {
            return std::make_optional(reinterpret_cast<const wchar_t*>(config));
        }

        // Move past the short-name (+1 for terminator) to get to the next entry
        config += (shortNameLen + 1) * sizeof(wchar_t);
    }
}

extern "C"
{
    __declspec(dllexport) LRESULT ShellHookHandler(int code, WPARAM wparam, LPARAM lparam)
    {
        if (g_isProcessOfInterest && code >= 0)
        {
            ShellHookHandlerInternal(code, wparam, lparam);
        }

        return CallNextHookEx(nullptr, code, wparam, lparam);
    }
}

HANDLE AcquireReleaseReaderLock(bool acquire, wchar_t mmapName[_countof(MMAP_NAME_1)], HANDLE existingLock)
{
    const auto readMutex = CreateMutexW(nullptr, false, MMAP_READ_MUTEX_NAME);
    if (!readMutex)
    {
        return nullptr;
    }
    SCOPE_EXIT { CloseHandle(readMutex); };

    if (WaitForSingleObject(readMutex, 1000) != WAIT_OBJECT_0)
    {
        return nullptr;
    }
    SCOPE_EXIT { ReleaseMutex(readMutex); };

    const auto syncMap = OpenFileMappingW(
        FILE_MAP_READ | FILE_MAP_WRITE,
        false,
        MMAP_SYNC_NAME
    );
    if (!syncMap) return nullptr;
    SCOPE_EXIT { CloseHandle(syncMap); };

    const auto syncMapView = MapViewOfFile(
        syncMap,
        FILE_MAP_READ | FILE_MAP_WRITE,
        0,
        0,
        sizeof(MmapSyncData)
    );
    if (!syncMapView) return nullptr;
    SCOPE_EXIT { UnmapViewOfFile(syncMapView); };

    auto syncData = static_cast<MmapSyncData*>(syncMapView);
    if (acquire)
    {
        syncData->numReaders++;
        wcscpy_s(mmapName, _countof(MMAP_NAME_1), syncData->mmapName.data());
        if (syncData->numReaders == 1)
        {
            const auto result = OpenMutexW(SYNCHRONIZE, FALSE, MMAP_WRITE_MUTEX_NAME);
            if (!result) return nullptr;

            if (WaitForSingleObject(result, 5000) != WAIT_OBJECT_0)
            {
                CloseHandle(result);
                return nullptr;
            }

            return result;
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

    return nullptr;
}

HANDLE AcquireReaderLock(wchar_t mmapName[_countof(MMAP_NAME_1)])
{
    return AcquireReleaseReaderLock(true, mmapName, nullptr);
}

void ReleaseReaderLock(HANDLE lockHandle)
{
    AcquireReleaseReaderLock(false, nullptr, lockHandle);
}

void CheckIfProcessOfInterest()
{
    std::array<wchar_t, _countof(MMAP_NAME_1)> mmapName;
    const auto lockHandle = AcquireReaderLock(mmapName.data());
    if (!lockHandle) return;
    SCOPE_EXIT { ReleaseReaderLock(lockHandle); };

    const HANDLE mmap = OpenFileMappingW(
        FILE_MAP_READ,
        false,
        mmapName.data()
    );
    if (!mmap) return;
    SCOPE_EXIT { CloseHandle(mmap); };

    uint32_t totalSize;
    {
        auto mmapView = MapViewOfFile(
            mmap,
            FILE_MAP_READ,
            0,
            0,
            sizeof(uint32_t)
        );
        if (!mmapView) return;
        SCOPE_EXIT { UnmapViewOfFile(mmapView); };

        totalSize = *reinterpret_cast<const uint32_t*>(mmapView);
    }

    auto mmapViewBase = MapViewOfFile(
        mmap,
        FILE_MAP_READ,
        0,
        0,
        totalSize);
    if (!mmapViewBase) return;
    SCOPE_EXIT { UnmapViewOfFile(mmapViewBase); };

    const auto mmapView = reinterpret_cast<const std::byte*>(mmapViewBase) + sizeof(uint32_t);

    size_t pathMaxLen = 512;
    std::vector<wchar_t> path(pathMaxLen);

    auto pathLen = GetModuleFileNameW(nullptr, path.data(), path.size());
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        pathMaxLen *= 2;
        path.resize(pathMaxLen);
        pathLen = GetModuleFileNameW(nullptr, path.data(), path.size());
    }

    const std::wstring pathStr(path.data(), pathLen);
    const auto shortNameOpt = LookupGameInfoByExeName(pathStr, mmapView);
    if (!shortNameOpt.has_value()) return;

    const auto shortName = shortNameOpt.value();

    g_isProcessOfInterest = true;
    g_appUserModelId = L"BlizzardEntertainment.BattleNet." + shortName;
    g_relaunchCommand = L"battlenet://" + shortName;
    g_iconPath = L"\"" + pathStr + L"\",0";
}

BOOL WINAPI DllMain(HANDLE dllHandle, DWORD reason, LPVOID reserved)
{
    UNREFERENCED_PARAMETER(dllHandle);
    UNREFERENCED_PARAMETER(reserved);

    if (reason == DLL_PROCESS_ATTACH)
    {
        CheckIfProcessOfInterest();
    }

    return TRUE;
}
