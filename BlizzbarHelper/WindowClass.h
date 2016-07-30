#pragma once
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>

class WindowClass
{
public:
	WindowClass(HINSTANCE inst, WNDPROC wndProc, std::wstring name);
	~WindowClass();

	std::wstring className() const;

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;

private:
	std::wstring m_name;
	WNDCLASSW m_class;
};
