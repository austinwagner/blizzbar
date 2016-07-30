#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "Win32Exception.h"

#include <locale>
#include <codecvt>

Win32Exception::Win32Exception() 
{ 
	initMessage(""); 
}

Win32Exception::Win32Exception(const std::string& detail) 
{ 
	initMessage(detail); 
}

void Win32Exception::initMessage(const std::string& detail)
{
	DWORD errorCode = GetLastError();

	LPWSTR errorMessageBuffer;
	DWORD len = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&errorMessageBuffer),
		0,
		nullptr);

	std::wstring errorMessageWide(errorMessageBuffer, len);
	LocalFree(errorMessageBuffer);
	
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	std::string errorMessage = converter.to_bytes(errorMessageWide);

	m_message = detail.empty()
		? errorMessage
		: (detail + " " + errorMessage);
}

const char* Win32Exception::what() const
{
	return m_message.c_str();
}
