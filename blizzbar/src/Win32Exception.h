#pragma once
#include <exception>
#include <string>

class Win32Exception : public std::exception
{
public:
	Win32Exception();
	explicit Win32Exception(const std::string& detail);
	const char* what() const override;

private:
	void initMessage(const std::string& detail);

	std::string m_message;
};
