#pragma once

// NOTE(): to avoid conflicts with c runtime library
struct AWESOME_RTL_CRITICAL_SECTION_DEBUG;

struct AWESOME_RTL_CRITICAL_SECTION { /* Size=0x18 */
  AWESOME_RTL_CRITICAL_SECTION_DEBUG* DebugInfo;
  long LockCount;
  long RecursionCount;
  void* OwningThread;
  void* LockSemaphore;
  unsigned long SpinCount;
};

class MtCriticalSection {
public:
    AWESOME_RTL_CRITICAL_SECTION cs; // 0x0000
    int pad; // idunno why i've set this as 28 bytes, oh well
}; // Size: 0x001C
static_assert(sizeof(MtCriticalSection) == 0x1C);
