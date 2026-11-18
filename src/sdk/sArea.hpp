#pragma once

#include <cstdint>
#include <cstddef>

class aGame;
class currentMission {
public:
    char pad_0[0x20];
    int restart; // 0x20
    char pad_24[0x4];
    int currentCharacter; // 0x28
    char pad_2c[0xdc];
    int roomNumber;  // 0x108
    int roomNumber2; // 0x10c
}; // Size: 0x110
static_assert(sizeof(currentMission) == 0x110);

class pauseMenu {
public:
    using uint8_t = unsigned char;
    char pad_0[0x14];
    uint8_t draw; // 0x14
    char pad_15[0x2b];
    bool optionHovered; // 0x40
}; // Size: 0x41
static_assert(sizeof(pauseMenu) == 0x41);

class sResource; // arc I think
class cResource;

// cEventScript (cBranchScript + a wait timer): a room's / mission's event script being run (Vibed/RE/event_script.md)
struct EventScript {
    void* vtable;                 // 0x00
    const uint8_t* data;          // 0x04 the next code (mpData)
    uint8_t depth;                // 0x08 IF / SWITCH nesting (mDepthCnt)
    char pad_9[0x3];
    float wait;                   // 0x0C frames left in a WAIT (mWaitTimer)
};
static_assert(sizeof(EventScript) == 0x10);

// The room area (cArea). Each room is a class aRoomNNN derived from it (Vibed/RE/room_loading.md); custom rooms derive
// through sdk/RoomClasses. Vtable: 15 slots (MtObject 0-4, cArea 5 load / 6 init / 7 move / 8 final / 9 parent layer /
// 10 getName, aRoom 11-14 script data).
class aRoom {
public:
    using uint32_t = unsigned int;
    void* vtable;                 // 0x00
    uint8_t mode;                 // 0x04 aRoom::move state: 2 changing room, 4 cutscene, 5 message, 6 game over, 7 play
    uint8_t sub_state;            // 0x05 within the mode (changing: 100 = done)
    char pad_6[0x1];              // 0x06
    int8_t busy; // 0x07          // load state (cArea load, vtable slot 5): 0-6, back to 0 when loaded
    char pad_8[0x18];             // 0x08 CRITICAL_SECTION
    bool allowPause; // 0x20
    char pad_21[0x7];             // 0x21
    int room_no;                  // 0x28 room number: arc rom\room\st<room>, sMediator::mRoomNo
    int stage_no;                 // 0x2C stage number: the room's files scr\st<stage>\...
    char pad_30[0x1];             // 0x30
    bool isPaused; // 0x31
    char pad_32[0xA];
    uint32_t restartLastEvent; // 0x3C
    char pad_40[0xC];             // 0x40
    cResource* room_default;      // 0x4C rRoomDefault scr\stNNN\etc\stNNN (start position, env map, wind)
    char pad_50[0x8];             // 0x50
    void* scheduler;              // 0x58 uScheduler of scr\stNNN\sdl\stNNN
    char pad_5c[0x14];            // 0x5C
    sResource* enemyResource[16]; // 0x70
    char pad_b0[0x10];            // 0xB0
    cResource* room_arc;          // 0xC0 rom\room\st<room>: the load waits until it's loaded (bit 1 of +0x4C)
    cResource* stage_arc;         // 0xC4 rom\room\st<stage> when the stage number differs (not waited for)
    char pad_c8[0x20];            // 0xC8
    void* camera;                 // 0xE8 uCameraCtrl (scr\stNNN\cam\stNNN)
    char pad_ec[0x4];             // 0xEC
    void* event_hit;              // 0xF0 cEventHit: door / trigger volumes (rEventHit)
    EventScript script;           // 0xF4 the load script (vtable slot 11), run once in the load state
    EventScript after_script;     // 0x104 the per-frame script (slot 12): aRoom::main runs it, restarts it when it ends
    pauseMenu* pauseMenuPtr1; // 0x114
    char pad_118[0x4];            // 0x118
    cResource* sound_request;     // 0x11C rSoundRequest sound\se\room\stNNN\stNNN
    cResource* stream_request;    // 0x120 rSoundStreamRequest sound\bgm\room\roomNNN\roomNNN
    char pad_124[0xC];            // 0x124
    char se_generator[0x90];      // 0x130 uSeGenerator (sound\seg\stNNN)
};

static_assert(sizeof(aRoom) == 0x1C0);
static_assert(offsetof(aRoom, room_no) == 0x28 && offsetof(aRoom, stage_no) == 0x2C);
static_assert(offsetof(aRoom, room_arc) == 0xC0 && offsetof(aRoom, event_hit) == 0xF0);
static_assert(offsetof(aRoom, script) == 0xF4 && offsetof(aRoom, after_script) == 0x104);
static_assert(offsetof(aRoom, pauseMenuPtr1) == 0x114 && offsetof(aRoom, se_generator) == 0x130);

class sArea {
public:
    char pad_0x0000[0x3830];           // 0x0000
    aGame* aGamePtr;                   // 0x3830 aGame
    currentMission* currentMissionPtr; // 0x3834
    aRoom* aRoomPtr;       // x03838
}; // Size: 0x383C
static_assert(sizeof(sArea) == 0x383C);
