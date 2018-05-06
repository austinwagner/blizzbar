#include "NotifyIconWindow.h"

#include <cassert>

#include "../res/strings.h"
#include "../res/resource.h"

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


	return window->messageHandlerImpl(hwnd, message, wparam, lparam);
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
