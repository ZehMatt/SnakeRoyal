#pragma once

#include <string>

namespace Utils
{
double getTime();
std::wstring toWString(const std::string& str);
std::string toMBString(const std::wstring& wstr);

template<typename T> int mod(T x, T divisor)
{
    T m = x % divisor;
    return m + ((m >> ((sizeof(T) * 8) - 1)) & divisor);
}

void getUsername(char* buffer, size_t maxBuffer);

} // namespace Utils
