#pragma once
#include <exception>
#include <string>

class Win32Exception : public std::exception
{
public:
	Win32Exception();
	explicit Win32Exception(const std::string& detail);

	void initMessage(const std::string& detail);
	const char* what() const;

private:
	std::string m_message;
};
