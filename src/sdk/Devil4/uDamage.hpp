#pragma once

class uEnemyDamagePossibility {
public:
    char pad_00[4]; // 0x0
}; // Size: 0x4

class uDamage_Old {
public:
    char pad_0[0x18];
    float HP;    // 0x18
    float HPMax; // 0x1c
    char pad_20[0x8];
    float prevDamageResist; // 0x28 // damage resist * difficulty
    float HPTaken;          // 0x2c
    char pad_30[0xc];
    int stun[5];         // 0x3c
    int displacement[5]; // 0x50
    int unknown[8];      // 0x64
    char pad_84[0x8];
    bool isDead; // 0x8c
    char pad_8d[0x1f];
    float stunResetTimer; // 0xAC
}; // Size: 0xb0
static_assert(sizeof(uDamage_Old) == 0xb0);