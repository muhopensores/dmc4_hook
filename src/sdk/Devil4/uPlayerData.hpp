#pragma once

#include "..\MtMath.hpp"

class CharacterSettingsOne {
public:
    char pad_0[0x1c];
    bool groundedActual; // 0x1c
    char pad_1d[0x143];
    float offsetFromGround; // 0x160
}; // Size: 0x164
static_assert(sizeof(CharacterSettingsOne) == 0x164);

class CharacterSettingsTwo {
public:
    char pad_0[0xC8];
    float sprintTimer;           // 0xc8
    float sprintSpeedMultiplier; // 0xd0
    char pad_d4[0xa4];
    float fallSpeed; // 0x174
}; // Size: 0x178
static_assert(sizeof(CharacterSettingsTwo) == 0x178);

class CharacterSettingsThree {
public:
    char pad_0[0x8];
    float stingerSpeed;  // 0x8
    float stingerLength; // 0xc
    char pad_10[0x14];
    float firstShotEbonyIvoryBounce; // 0x24
    char pad_28[0x40];
    float rainstormBounce; // 0x68
    char pad_6c[0xc8];
    float trickTravelSpeed;    // 0x134
    float trickTravelDistance; // 0x138
    char pad_13c[0x18];
    float rainstormHeightRestriction; // 0x154
    char pad_158[0x20];
    float crossunderBanDistance; // 0x178
    char pad_17c[0x8c];
    float jealousyBounce; // 0x208
    char pad_20c[0x8];
    float snatchLengthLv1; // 0x214
    float snatchLengthLv2; // 0x218
    float snatchLengthLv3; // 0x21c
    char pad_220[0xc];
    Vector3f snatchEnemyOffset; // 0x22c
    char pad_238[0x164];
    float funshipDrainRate; // 0x39c
    char pad_3a0[0x40];
    float funshipBounce;   // 0x3e0
    float funshipUnbounce; // 0x3e4
}; // Size: 0x3e8
static_assert(sizeof(CharacterSettingsThree) == 0x3e8);
