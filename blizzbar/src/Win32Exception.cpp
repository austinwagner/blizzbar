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

namespace
{
    struct LocalFreeDeleter
    {
        void operator()(void* p) const { if (p != nullptr) LocalFree(p); }
    };
}

void Win32Exception::initMessage(const std::string& detail)
{
	const auto errorCode = GetLastError();

	std::unique_ptr<wchar_t, LocalFreeDeleter> errorMessageBuffer;
	const auto len = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPWSTR>(&errorMessageBuffer),
		0,
		nullptr);

	const std::wstring errorMessageWide(errorMessageBuffer.get(), len);

    // Since exceptions deriving from std::exception have to be able to return
    // a char*, we convert to UTF-8 to be sure that nothing is lost in translation
	std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
	const auto errorMessage = converter.to_bytes(errorMessageWide);

	m_message = detail.empty()
		? errorMessage
		: (detail + " " + errorMessage);
}

const char* Win32Exception::what() const
{
	return m_message.c_str();
}
