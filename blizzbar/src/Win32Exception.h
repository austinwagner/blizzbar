#pragma once

#include <cstdint>
#include <exception>
#include <string>

class Win32Exception : public std::exception {
public:
    Win32Exception();
    explicit Win32Exception(uint32_t error);
    explicit Win32Exception(const std::string& detail);
    Win32Exception(uint32_t error, const std::string& detail);
    const char* what() const override;

private:
    std::string m_message;
};
