#pragma once
#include "Windows.h"
#include <string>
#include <functional>

std::wstring GetUnknownLengthString(std::function<void(wchar_t*, DWORD)> f);

std::wstring utf8_to_utf16(const std::string& str);

std::string utf16_to_utf8(const std::wstring& str);
