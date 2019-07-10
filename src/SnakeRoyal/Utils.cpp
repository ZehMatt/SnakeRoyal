#include <windows.h>
#include <chrono>
#include "Utils.h"

namespace Utils
{
static std::chrono::high_resolution_clock _clock;
static const auto _timeStart = _clock.now();

double getTime()
{
    auto deltaTime = _clock.now() - _timeStart;
    return std::chrono::duration_cast<std::chrono::nanoseconds>(deltaTime)
               .count()
           / 1000000000.0;
}

std::wstring toWString(const std::string& str)
{
    int num_chars = MultiByteToWideChar(
        CP_UTF8, 0, str.c_str(), str.length(), NULL, 0);
    std::wstring wstrTo;
    if (num_chars)
    {
        wstrTo.resize(num_chars);
        MultiByteToWideChar(
            CP_UTF8, 0, str.c_str(), str.length(), &wstrTo[0], num_chars);
    }
    return wstrTo;
}

std::string toMBString(const std::wstring& wstr)
{
    int num_chars = WideCharToMultiByte(
        CP_UTF8, 0, wstr.c_str(), wstr.length(), NULL, 0, NULL, NULL);
    std::string strTo;
    if (num_chars > 0)
    {
        strTo.resize(num_chars);
        WideCharToMultiByte(
            CP_UTF8, 0, wstr.c_str(), wstr.length(), &strTo[0], num_chars, NULL,
            NULL);
    }
    return strTo;
}

void getUsername(char* buffer, size_t maxBuffer)
{
    memset(buffer, 0, maxBuffer);

    DWORD bufferSize = maxBuffer;
    GetUserNameA(buffer, &bufferSize);
}

} // namespace Utils