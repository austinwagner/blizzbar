#pragma once
#include <Windows.h>
#include "Handle.h"
#include "WindowClass.h"
#include "NotifyIcon.h"

class NotifyIconWindow
{
public:
	explicit NotifyIconWindow(HINSTANCE instance);

	void runMessageLoop();

private:
	static LRESULT CALLBACK messageHandler(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
	LRESULT messageHandlerImpl(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

	HINSTANCE m_instance;
	WindowClass m_windowClass;
	Window m_window;
	NotifyIcon m_notifyIcon;

	static const UINT CallbackMessage = WM_USER + 1;
};

