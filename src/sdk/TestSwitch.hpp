#pragma once
// test_switch - the DMC4_* environment variables: test switches and diagnostics (netplay's soak runs, forced desyncs,
// investigation logs, frame-rate checks). Debug and dev builds only: a public build never reads them, so a stray variable
// on a player's machine changes nothing.
#include <cstdlib>

inline const char* test_switch(const char* name)
{
#if defined(_DEBUG) || !defined(PUBLIC_BUILD)
    return std::getenv(name);
#else
    (void)name;
    return nullptr;
#endif
}
