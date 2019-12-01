#include "hacklib/CrashHandler.h"


bool hl::CrashHandler(const std::function<void()>& body)
{
    bool success = true;
    hl::CrashHandler(body, [&](uint32_t) { success = false; });
    return success;
}