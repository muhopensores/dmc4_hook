#include "hacklib/CrashHandler.h"
#include <Windows.h>


// Microsoft does not seem to provide a definition.
#define EXCEPTION_CPP 0xE06D7363


void hl::CrashHandler(const std::function<void()>& body, const std::function<void(uint32_t)>& handler)
{
    __try
    {
        body();
    }
    __except (GetExceptionCode() == EXCEPTION_CPP ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER)
    {
        handler(GetExceptionCode());
    }
}