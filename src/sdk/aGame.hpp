#pragma once

#include "Math.hpp"

class roomInfo {
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

    char n0000127_e[20];    // 0x11E5888
    char pad_0x0018[0x3EC]; // 0x0018

}; // Size=0x0404
static_assert(sizeof(roomInfo) == 0x404);

class aGame {
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

    char pad_4[0x28];
    roomInfo* roomInfoPtr1; // 0x2c
    char pad_30[0x20];
    Vector3f unk_vec;  // 0x50
    Vector3f unk_vec2; // 0x5c
    int init_jump;     // 0x68
    int room_id;       // 0x6c
    int door_entered;  // 0x70
    int bp_floor;      // 0x74
    char pad_78[0x159];
    bool m_paused; // 0x1d1
}; // Size: 0x1d4
static_assert(sizeof(aGame) == 0x1d4); // NOTE(): got rid of pragma pack 
