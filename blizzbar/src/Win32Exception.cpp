#include "Win32Exception.h"

#include <Windows.h>

#include <codecvt>
#include <locale>

namespace {
struct LocalFreeDeleter {
    void operator()(void* p) const
    {
        if (p != nullptr) {
            LocalFree(p);
        }
    }
};

std::string FormatSystemMessage(uint32_t errorCode, const std::string& detail)
{
    std::unique_ptr<wchar_t, LocalFreeDeleter> errorMessageBuffer;
    const auto len = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPWSTR>(&errorMessageBuffer), 0, nullptr);

    const std::wstring errorMessageWide(errorMessageBuffer.get(), len);

    // Since exceptions deriving from std::exception have to be able to return
    // a char*, we convert to UTF-8 to be sure that nothing is lost in
    // translation
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    const auto errorMessage = converter.to_bytes(errorMessageWide);

    return detail.empty() ? errorMessage : (detail + " " + errorMessage);
}
}

Win32Exception::Win32Exception()
    : Win32Exception(GetLastError())
{
}

Win32Exception::Win32Exception(const std::string& detail)
    : Win32Exception(GetLastError(), detail)
{
}

Win32Exception::Win32Exception(uint32_t error)
    : Win32Exception(error, "")
{
}

Win32Exception::Win32Exception(uint32_t error, const std::string& detail)
    : m_message{ FormatSystemMessage(error, detail) }
{
}

const char* Win32Exception::what() const { return m_message.c_str(); }
