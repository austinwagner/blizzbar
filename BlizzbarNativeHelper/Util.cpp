#include "Util.h"
#include <memory>
#include "numeric_cast.h"
#include "Win32Exception.h"

std::wstring GetUnknownLengthString(std::function<void(wchar_t*, DWORD)> f)
{
	DWORD strLen = 512;
	std::unique_ptr<wchar_t[]> strPtr(new wchar_t[strLen]);
	f(strPtr.get(), strLen);

	while (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
	{
		strLen *= 2;
		strPtr.reset(new wchar_t[strLen]);
		f(strPtr.get(), strLen);
	}

	return std::wstring(strPtr.get());
}

std::wstring utf8_to_utf16(const std::string& str)
{
	const int strLen = numeric_cast<int>(str.size());

	int resultLen = MultiByteToWideChar(
		CP_UTF8,
		0,
		const_cast<char*>(str.data()),
		strLen,
		nullptr,
		0);
	if (resultLen == 0)
	{
		return L"STRING CONVERSION FAILURE";
	}

	std::wstring result(resultLen, L'\0');
	resultLen = MultiByteToWideChar(
		CP_UTF8,
		0,
		const_cast<char*>(str.data()),
		strLen,
		const_cast<wchar_t*>(result.data()),
		resultLen);
	if (resultLen == 0)
	{
		return L"STRING CONVERSION FAILURE";
	}

	return result;
}

std::string utf16_to_utf8(const std::wstring& str)
{
	const int strLen = numeric_cast<int>(str.size());

	int resultLen = WideCharToMultiByte(
		CP_UTF8,
		0,
		const_cast<wchar_t*>(str.data()),
		strLen,
		nullptr,
		0,
		nullptr,
		nullptr);
	if (resultLen == 0)
	{
		return "STRING CONVERSION FAILURE";
	}

	std::string result(resultLen, '\0');
	resultLen = WideCharToMultiByte(
		CP_UTF8,
		0,
		const_cast<wchar_t*>(str.data()),
		strLen,
		const_cast<char*>(result.data()),
		resultLen,
		nullptr,
		nullptr);
	if (resultLen == 0)
	{
		return "STRING CONVERSION FAILURE";
	}

	return result;
}