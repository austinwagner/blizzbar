#include "NotifyIconWindow.h"
#include "Win32Exception.h"

#include <cassert>
#include <filesystem>
#include <gsl/gsl>
#include <locale>
#include <codecvt>

#include "../res/strings.h"
#include "../res/resource.h"

namespace fs = std::filesystem;
using namespace std::string_literals;

constexpr size_t MaxLongPath = 32768;

NotifyIconWindow::NotifyIconWindow(HINSTANCE instance) :
	m_instance(instance),
	m_windowClass(m_instance, messageHandler, L"BlizzbarHidden"),
	m_window(0, m_windowClass.className(), APP_NAME, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, m_instance, nullptr),
	m_notifyIcon(m_window, CallbackMessage, LoadIconW(m_instance, MAKEINTRESOURCEW(IDI_BLIZZBAR)), APP_FRIENDLY_NAME)
{
	SetWindowLongPtrW(m_window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
}

void NotifyIconWindow::runMessageLoop()
{
	MSG msg;
	while (GetMessageW(&msg, m_window, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
	}
}

LRESULT CALLBACK NotifyIconWindow::messageHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	auto window = reinterpret_cast<NotifyIconWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
	if (window == nullptr) 
	{
		return DefWindowProcW(hwnd, message, wparam, lparam);
	}

    try
    {
	    return window->messageHandlerImpl(hwnd, message, wparam, lparam);
    }
    catch (const std::exception& ex)
    {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		std::wstring message = converter.from_bytes(ex.what());

		MessageBoxW(nullptr, message.c_str(), APP_FRIENDLY_NAME, MB_OK | MB_ICONERROR);
		return 0;
    }
}

namespace
{
    fs::path procPath;
    std::wstring procCmdLine;

    std::pair<const fs::path&, const std::wstring&> GetProcPath()
    {
        if (procPath.empty())
        {
	        std::vector<wchar_t> path;
	        path.resize(512);

            auto err = ~ERROR_SUCCESS;
	        while (err != ERROR_SUCCESS)
	        {
		        GetModuleFileNameW(nullptr, path.data(), gsl::narrow<DWORD>(path.size()));
                err = GetLastError();
                if (err == ERROR_INSUFFICIENT_BUFFER)
                {
			        if (path.size() == MaxLongPath) throw Win32Exception("Unable to get process path.");

			        path.resize(std::min(path.size() * 2, MaxLongPath));
                }
                else if (err != ERROR_SUCCESS)
                {
			        throw Win32Exception(err, "Unable to get process path.");
                }
	        }

            procPath = fs::canonical({path.data()});
            procCmdLine = L'"' + procPath.wstring() + L'"';
        }

        return {procPath, procCmdLine};
    }

    const auto AutoRunKeyPath = LR"(Software\Microsoft\Windows\CurrentVersion\Run)";
    const auto AutoRunValueName = L"net.austinwagner.blizzbar";

    bool IsRegisteredForStartup()
    {
        const auto autoRunKey = RegistryKey::open(HKEY_CURRENT_USER, AutoRunKeyPath, KEY_QUERY_VALUE);
        if (!autoRunKey) return false;

        DWORD len;
        if (RegGetValueW(*autoRunKey, nullptr, AutoRunValueName,
            RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ, nullptr, nullptr, &len) != ERROR_SUCCESS)
        {
            return false;
        }

        std::vector<wchar_t> pathVec;
        pathVec.resize(len / sizeof(wchar_t));

        if (RegGetValueW(*autoRunKey, nullptr, AutoRunValueName,
            RRF_RT_REG_EXPAND_SZ | RRF_RT_REG_SZ, nullptr,
            reinterpret_cast<void*>(pathVec.data()), &len) != ERROR_SUCCESS)
        {
            return false;
        }

        const auto& currProcPath = std::get<0>(GetProcPath());

        // With the pathVec[1] below, strips the quotes
        pathVec[pathVec.size() - 3] = 0;

        try
        {
            return fs::equivalent({&pathVec[1]}, currProcPath);
        }
        catch (const fs::filesystem_error&)
        {
            return false;
        }
    }

    void RegisterForStartup()
    {
        const auto& cmdLine = std::get<1>(GetProcPath());
        if (cmdLine.size() > 260)
        {
            throw std::runtime_error("Cannot set to run at startup; Windows places a length restriction on startup entries.\nMove this program to a location with a path of 258 characters or less (including the EXE file name).");
        }

        auto autoRunKey = RegistryKey::create(HKEY_CURRENT_USER, AutoRunKeyPath, nullptr, 0,
            KEY_SET_VALUE | KEY_CREATE_SUB_KEY, nullptr, nullptr);
        const auto res = RegSetKeyValueW(autoRunKey, nullptr, AutoRunValueName, REG_SZ,
            reinterpret_cast<const void*>(cmdLine.data()), gsl::narrow<DWORD>(cmdLine.size() * sizeof(wchar_t)));
        if (res != ERROR_SUCCESS) throw Win32Exception(res, "Unable to create startup entry.");
    }

    void UnregisterForStartup()
    {
        const auto res = RegDeleteKeyValueW(HKEY_CURRENT_USER, AutoRunKeyPath, AutoRunValueName);
        if (res != ERROR_SUCCESS) throw Win32Exception(res, "Unable to remove startup entry.");
    }
}

LRESULT NotifyIconWindow::messageHandlerImpl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
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
        case IDM_RUNATSTARTUP:
            if (IsRegisteredForStartup())
            {
                UnregisterForStartup();
            }
            else
            {
                RegisterForStartup();
            }
            break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);
		}
	}
	break;
	case CallbackMessage:
	{
		switch (LOWORD(lParam))
		{
		case WM_CONTEXTMENU:
		{
			POINT cursorPos;
			GetCursorPos(&cursorPos);

			HMENU menu = LoadMenuW(m_instance, MAKEINTRESOURCEW(IDC_BLIZZBARHELPER));
			HMENU subMenu = GetSubMenu(menu, 0);

			UINT menuAlignment =
				GetSystemMetrics(SM_MENUDROPALIGNMENT) == 0 ?
				TPM_LEFTALIGN :
				TPM_RIGHTALIGN;

            CheckMenuItem(subMenu, IDM_RUNATSTARTUP, IsRegisteredForStartup() ? MF_CHECKED : MF_UNCHECKED);

			// SetForegroundWindow and PostMessage are needed to prevent the context menu from getting stuck
			SetForegroundWindow(m_window);
			TrackPopupMenu(subMenu, menuAlignment | TPM_BOTTOMALIGN, cursorPos.x, cursorPos.y, 0, hWnd, nullptr);
			PostMessageW(m_window, WM_NULL, 0, 0);

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
