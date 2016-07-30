#include "NotifyIcon.h"

#include "Win32Exception.h"

NotifyIcon::NotifyIcon(HWND hwnd, UINT callback, HICON icon, const std::wstring& tooltip)
{
	m_data.cbSize = sizeof(NOTIFYICONDATAW);
	m_data.uVersion = NOTIFYICON_VERSION_4;
	m_data.uCallbackMessage = callback;
	m_data.hWnd = hwnd;
	m_data.hIcon = icon;
	m_data.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP;

	wcscpy_s(m_data.szTip, tooltip.c_str());

	if (!Shell_NotifyIconW(NIM_ADD, &m_data))
	{
		throw Win32Exception("Failed to add icon to notification area.");
	}
	
	try
	{
		if (!Shell_NotifyIconW(NIM_SETVERSION, &m_data))
		{
			throw Win32Exception("Failed to set notification icon version.");
		}
	}
	catch (...)
	{
		Shell_NotifyIconW(NIM_DELETE, &m_data);
		throw;
	}
}


NotifyIcon::~NotifyIcon()
{
	Shell_NotifyIconW(NIM_DELETE, &m_data);
}
