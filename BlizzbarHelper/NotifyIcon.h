#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <shellapi.h>

#include <string>

class NotifyIcon
{
public:
	NotifyIcon(HWND hwnd, UINT callback, HICON icon, const std::wstring& tooltip);
	~NotifyIcon();

	NotifyIcon(const NotifyIcon&) = delete;
	NotifyIcon& operator=(const NotifyIcon&) = delete;

private:
	NOTIFYICONDATAW m_data;
};

