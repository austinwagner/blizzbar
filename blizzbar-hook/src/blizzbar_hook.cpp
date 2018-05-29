#include <blizzbar_common.h>
#include "ConfigHelpers.h"
#include "ThreadLocal.h"

#include <MinHook.h>
#include <gsl/gsl_util>

#include <Windows.h>
#include <Psapi.h>
#include <shellapi.h>
#include <objbase.h>
#include <ShObjIdl.h>
#include <propkey.h>

#include <unordered_set>
#include <algorithm>
#include <string>
#include <memory>
#include <string_view>
#include <type_traits>
#include <filesystem>

using namespace std::chrono_literals;
namespace fs = std::filesystem;

constexpr size_t MaxLongPath = 32768;

typedef BOOL (WINAPI *GetMessageProcW)(LPMSG, HWND, UINT, UINT);


GetMessageProcW g_origGetMessageProc = nullptr;
ConfigFileMapping g_configMapping;
ConfigFileView g_configView;
const GameInfo* g_gameInfo = nullptr;
std::wstring g_iconPath;
HANDLE g_thread;
bool g_hookInstalled;

ThreadLocal<std::unordered_set<HWND>> tl_hwnds;


class WindowPropStore {
public:
	static std::unique_ptr<WindowPropStore> Create(HWND window)
	{
		IPropertyStore* p;
		if (SHGetPropertyStoreForWindow(window, IID_IPropertyStore, reinterpret_cast<void**>(&p)) != S_OK)
		{
			return nullptr;
		}

		return std::make_unique<WindowPropStore>(p);
	}

	explicit WindowPropStore(IPropertyStore* p) : m_propStore(p)
	{
	}

	~WindowPropStore() { m_propStore->Release(); }
	WindowPropStore(const WindowPropStore& other) : m_propStore(other.m_propStore) { m_propStore->AddRef(); }
	WindowPropStore& operator=(const WindowPropStore& other)
	{
		if (this != &other)
		{
			if (m_propStore != nullptr) m_propStore->Release();
			m_propStore = other.m_propStore;
			m_propStore->AddRef();
		}

		return *this;
	}
	WindowPropStore(WindowPropStore&& other) noexcept : m_propStore(other.m_propStore)
	{
		other.m_propStore = nullptr;
	}
	WindowPropStore& operator=(WindowPropStore&& other) noexcept
	{
		if (m_propStore != nullptr) m_propStore->Release();
		m_propStore = other.m_propStore;
		other.m_propStore = nullptr;
		return *this;
	}

	template <class String>
	void setString(const PROPERTYKEY& pkey, const String& str)
	{
		static_assert(std::is_same_v<typename String::value_type, wchar_t>);

		PROPVARIANT pv;
		pv.vt = VT_LPWSTR;
		pv.pwszVal = const_cast<wchar_t*>(str.c_str());
		m_propStore->SetValue(pkey, pv);
	}

	void clearValue(const PROPERTYKEY& pkey)
	{
		PROPVARIANT pv;
		pv.vt = VT_EMPTY;
		m_propStore->SetValue(pkey, pv);
	}

private:
	IPropertyStore * m_propStore;
};

template <class T1, class T2>
bool CompareStringIgnoreCase(T1&& lhs, T2&& rhs)
{
	if (lhs.size() != rhs.size()) return false;
	return _wcsnicmp(lhs.data(), rhs.data(), lhs.size()) == 0;
}

void SetWindowProps(HWND window)
{
	auto propStore = WindowPropStore::Create(window);
	if (!propStore) return;

	tl_hwnds->insert(window);

	propStore->setString(PKEY_AppUserModel_RelaunchCommand, g_gameInfo->relaunchCommand);
	propStore->setString(PKEY_AppUserModel_RelaunchDisplayNameResource, g_gameInfo->gameName);
	propStore->setString(PKEY_AppUserModel_RelaunchIconResource, g_iconPath);
	propStore->setString(PKEY_AppUserModel_ID, g_gameInfo->appUserModelId);
}

void ClearWindowProps(HWND window)
{
	const auto findIter = tl_hwnds->find(window);
	if (findIter == tl_hwnds->end()) return;

	auto propStore = WindowPropStore::Create(window);
	if (!propStore) return;

	tl_hwnds->erase(findIter);

	propStore->clearValue(PKEY_AppUserModel_RelaunchCommand);
	propStore->clearValue(PKEY_AppUserModel_RelaunchDisplayNameResource);
	propStore->clearValue(PKEY_AppUserModel_RelaunchIconResource);
	propStore->clearValue(PKEY_AppUserModel_ID);
}

void ShellHookHandlerInternal(int code, WPARAM wparam, LPARAM)
{
	switch (code)
	{
	case HSHELL_WINDOWCREATED:
	case HSHELL_WINDOWREPLACING:
	{
		const auto window = reinterpret_cast<HWND>(wparam);
		SetWindowProps(window);
		break;
	}
	case HSHELL_WINDOWDESTROYED:
	case HSHELL_WINDOWREPLACED:
	{
		const auto window = reinterpret_cast<HWND>(wparam);
		ClearWindowProps(window);
		break;
	}
	}
}

const GameInfo* LookupGameInfoByExeName(const std::wstring& exeName)
{
    const auto exeNameNoExt = ((std::wstring_view)exeName).substr(0, exeName.size() - 4);
	for (size_t i = 0; i < g_configView->size(); i++)
	{
		const auto gi = (*g_configView)[i];

		if (CompareStringIgnoreCase(exeNameNoExt, gi->exe))
		{
			return gi;
		}
	}

	return nullptr;
}

fs::path GetProcPath()
{
	std::vector<wchar_t> path;
	path.resize(512);

	while (true)
	{
		GetModuleFileNameW(nullptr, path.data(), gsl::narrow<DWORD>(path.size()));
		switch (GetLastError())
		{
		case ERROR_SUCCESS:
			return fs::path(path.data());
		case ERROR_INSUFFICIENT_BUFFER:
			if (path.size() == MaxLongPath) throw std::exception();
			path.resize(std::min(path.size() * 2, MaxLongPath));
			break;
		default:
			throw std::exception();
		}
	}
}

BOOL CALLBACK EnumThreadWindowsCallback(HWND hwnd, LPARAM)
{
	tl_hwnds->insert(hwnd);
	SetWindowProps(hwnd);
	return TRUE;
}

BOOL WINAPI GetMessageDetourW(LPMSG msg, HWND hwnd, UINT msgFilterMin, UINT msgFilterMax)
{
	if (!tl_hwnds.hasValue())
	{
		tl_hwnds.alloc();

		EnumThreadWindows(GetCurrentThreadId(), EnumThreadWindowsCallback, 0);
	}

	return g_origGetMessageProc(msg, hwnd, msgFilterMin, msgFilterMax);
}

void MinHookTry(MH_STATUS status)
{
	if (status != MH_OK) throw std::exception();
}

void CheckIfProcessOfInterest()
{
	auto path = fs::canonical(GetProcPath());

	auto exeName = path.filename();
	g_gameInfo = LookupGameInfoByExeName(exeName.wstring());
	if (g_gameInfo != nullptr)
	{
		g_iconPath = path.wstring() + L",0";
		MinHookTry(MH_Initialize());
		MinHookTry(MH_CreateHook(reinterpret_cast<void*>(&GetMessageW), reinterpret_cast<void*>(&GetMessageDetourW), reinterpret_cast<void**>(&g_origGetMessageProc)));
		MinHookTry(MH_EnableHook(reinterpret_cast<void*>(&GetMessageW)));
        g_hookInstalled = true;

		// Broadcast so the newly installed hook is called on all threads
		PostMessageW(HWND_BROADCAST, WM_NULL, 0, 0);

		// Should be plenty of time for all of the threads to enumerate their windows
		Sleep(10000);
		MH_RemoveHook(reinterpret_cast<void*>(&GetMessageW));
		MH_Uninitialize();
        g_hookInstalled = false;
	}
}

extern "C" __declspec(dllexport) LRESULT ShellHookHandler(int code, WPARAM wparam, LPARAM lparam)
{
	if (g_gameInfo != nullptr && code >= 0)
	{
		ShellHookHandlerInternal(code, wparam, lparam);
	}

	return CallNextHookEx(nullptr, code, wparam, lparam);
}


void OpenConfigFileMapping()
{
	g_configMapping = ConfigFileMapping(MMAP_NAME);
	g_configView = ConfigFileView(g_configMapping);
}

DWORD WINAPI OnAttached(LPVOID)
{
	try
	{
		OpenConfigFileMapping();
		CheckIfProcessOfInterest();
	}
	catch (...)
	{
		// If anything goes wrong, just reset the state and ignore it.
		// We absolutely do not want to bring down the process since this
		// is injected into anything that creates a window.
		g_gameInfo = nullptr;
		g_iconPath = std::wstring();
		g_configView = ConfigFileView();
		g_configMapping = ConfigFileMapping();
	}

	return 0;
}

BOOL WINAPI DllMain(HANDLE, DWORD reason, LPVOID)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		g_thread = CreateThread(nullptr, 0, &OnAttached, nullptr, 0, nullptr);
		break;
	case DLL_THREAD_ATTACH:
		tl_hwnds.alloc();
		break;
	case DLL_THREAD_DETACH:
		if (tl_hwnds.hasValue())
		{
			for (auto hwnd : *tl_hwnds)
			{
				ClearWindowProps(hwnd);
			}
			tl_hwnds.free();
		}
		break;
	case DLL_PROCESS_DETACH:
		if (g_hookInstalled)
		{
			MH_RemoveHook(reinterpret_cast<void*>(&GetMessageW));
			MH_Uninitialize();
		}

		CloseHandle(g_thread);

		break;
	}

	return true;
}
