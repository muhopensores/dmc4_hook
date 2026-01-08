#pragma once

#include <cstdint>

class sStylishCount {
public:
    char pad_0000[32];           // 0x0000
    uint32_t current_style_tier; // 0x0020
    char pad_0024[508];          // 0x0024
}; // Size: 0x0220
static_assert(sizeof(sStylishCount) == 0x220);
