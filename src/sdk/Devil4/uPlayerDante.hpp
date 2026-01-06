#pragma once

#include <cstdint>
#include "../MtMath.hpp"

class LuciferPin {
public:
    uint8_t pad_0[0x14];
    uint8_t pin_state;      // 0x14 (3-pinned, 4-primed, 7-climax)
    uint8_t pop_state;      // 0x15
    char pad_15[2];         // 0x16
    uintptr_t PinnedTarget; // 0x18
    char pad_1C[0x14];      // 0x1C
    Vector3f position;      // 0x30
    char pad_3C[4];         // 0x3C
    Vector4 rotation;       // 0x40
    Vector3f scale;         // 0x50
    char pad_5c[0x1734];    // 0x5C
    float timer;            // 0x1790
    float timerMax;         // 0x1794
    char pad_1798[0x1c];
    bool penetrated; // 0x17b4
}; // Size: 0x17b5
static_assert(sizeof(LuciferPin) == 0x17b8);

class DevilArm {
public:
    char pad_0[0x1370];
    uint8_t ID; // 0x1370
    char pad_1374[0xb];
    bool visible; // 0x137c
    char pad_137d[0x47];
    float roseYAxisVelocity; // 0x13c4
    char pad_13c8[0x20];
    bool roseAlive; // 0x13e8
}; // Size: 0x13e9
static_assert(sizeof(DevilArm) == 0x13ec);
