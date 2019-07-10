#pragma once

class Logging
{
public:
    Logging();
    ~Logging();

    void print(const char* fmt, ...);
};

extern Logging gLogging;

template<typename... Args> inline void logPrint(const char* fmt, Args... args)
{
    gLogging.print(fmt, args...);
}
