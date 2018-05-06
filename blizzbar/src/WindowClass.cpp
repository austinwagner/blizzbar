#include "WindowClass.h"
#include <utility>

#include "Win32Exception.h"


WindowClass::WindowClass(HINSTANCE hInst, WNDPROC wndProc, std::wstring name)
	: m_name(std::move(name))
{
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
    if (m_class.lpszClassName == nullptr) return;

	UnregisterClassW(m_class.lpszClassName, m_class.hInstance);
}

std::wstring WindowClass::className() const
{
	return m_name;
}

WindowClass::WindowClass(WindowClass&& other) noexcept
    : m_name{std::move(other.m_name)}
{
    std::swap(m_class, other.m_class);
}

WindowClass& WindowClass::operator=(WindowClass&& other) noexcept
{
    if (this != &other)
    {
        m_name = std::move(other.m_name);
        std::swap(m_class, other.m_class);
    }

    return *this;
}
