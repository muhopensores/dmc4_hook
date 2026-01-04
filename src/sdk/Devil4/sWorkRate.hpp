#pragma once

class sWorkRate;
class unkClass;
class unkClass2;

class sWorkRate {
public:
    virtual void function0(); // 0+0x00
    virtual void function1(); // 0+0x04
    virtual void function2(); // 0+0x08
    virtual void function3(); // 0+0x0C
    virtual void function4(); // 0+0x10
    virtual void function5(); // 0+0x14
    virtual void function6(); // 0+0x18
    virtual void function7(); // 0+0x1C
    virtual void function8(); // 0+0x20
    virtual void function9(); // 0+0x24

    char pad_0x0004[0x1C];   // 0x0004
    float game_speed;        // 0x0020
    float unk_float2;        // 0x0024
    float global_speed;      // 0x0028
    float player_speed;      // 0x002C
    float enemy_speed;       // 0x0030
    float room_speed;        // 0x0034
    float turbo_speed;       // 0x0038
    float unk_float3;        // 0x003C
    char pad_0x0040[0x10];   // 0x0040
    unkClass* unk_class_ptr; // 0x0050
    char pad_0x0054[0x4];    // 0x0054

}; // Size=0x0058
static_assert(sizeof(sWorkRate) == 0x58);

class unkClass {
public:
    using uint32_t = unsigned int;
    unkClass2* next;       // 0x0000
    uint32_t n0000045_c;   // 0x0004
    char pad_0x0008[0x4];  // 0x0008
    uint32_t n0000045_e;   // 0x000C
    char pad_0x0010[0x10]; // 0x0010
    uint32_t n00000463;    // 0x0020
    char pad_0x0024[0x8];  // 0x0024

}; // Size=0x002C
static_assert(sizeof(unkClass) == 0x2C);

class unkClass2 {
public:
    using uint32_t = unsigned int;
    uint32_t next_ptr;      // 0x0000
    char pad_0x0004[0x400]; // 0x0004

}; // Size=0x0404
static_assert(sizeof(unkClass2) == 0x404);
