#pragma once

// TODO(): uncomment this before merging
#include "MtObject.hpp"
#include "MtThread.hpp"

class cSystem : public MtObject {
public:
    class MtCriticalSection critical_section; // 0x0004
}; // Size: 0x0020
static_assert(sizeof(cSystem) == 0x0020);