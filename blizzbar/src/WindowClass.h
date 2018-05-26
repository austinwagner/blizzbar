#pragma once
#include <Windows.h>

#include <string>

class WindowClass
{
public:
	WindowClass(HINSTANCE inst, WNDPROC wndProc, std::wstring name);
	~WindowClass();

	std::wstring className() const;

    WindowClass(WindowClass&& other) noexcept;
    WindowClass& operator=(WindowClass&& other) noexcept;

	WindowClass(const WindowClass&) = delete;
	WindowClass& operator=(const WindowClass&) = delete;

private:
	std::wstring m_name;
	WNDCLASSW m_class{};
};
