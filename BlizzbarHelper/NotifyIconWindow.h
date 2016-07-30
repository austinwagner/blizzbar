#pragma once
#include <Windows.h>
#include "Handle.h"
#include "WindowClass.h"
#include "NotifyIcon.h"

class NotifyIconWindow
{
public:
	NotifyIconWindow(HINSTANCE instance);

	void runMessageLoop();

private:
	static LRESULT CALLBACK messageHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	LRESULT messageHandlerImpl(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	HINSTANCE m_instance;
	WindowClass m_windowClass;
	Window m_window;
	NotifyIcon m_notifyIcon;

	static const UINT CallbackMessage = WM_USER + 1;
};

