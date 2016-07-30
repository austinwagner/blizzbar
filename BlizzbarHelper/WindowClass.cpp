#include "WindowClass.h"

#include "Win32Exception.h"


WindowClass::WindowClass(HINSTANCE hInst, WNDPROC wndProc, std::wstring name)
	: m_name(name)
{
	m_class = { 0 };
	m_class.hInstance = hInst;
	m_class.lpfnWndProc = wndProc;

	// Need to be sure name is captured as a member since the WNDCLASS structure
	// holds a reference to it
	m_class.lpszClassName = m_name.c_str();

	if (RegisterClassW(&m_class) == 0)
	{
		throw Win32Exception("Failed to register window class.");
	}
}

WindowClass::~WindowClass()
{
	UnregisterClassW(m_class.lpszClassName, m_class.hInstance);
}

std::wstring WindowClass::className() const
{
	return m_name;
}