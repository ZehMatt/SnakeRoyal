#include "Logging.h"

#include <conio.h>
#include <windows.h>
#include <varargs.h>

Logging gLogging;

Logging::Logging()
{
    AllocConsole();
}

Logging::~Logging()
{
    FreeConsole();
}

void Logging::print(const char* fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    _vcprintf(fmt, vl);
    va_end(vl);
}
