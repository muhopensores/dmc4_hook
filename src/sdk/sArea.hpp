#pragma once

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

class aRoom {
public:
    using uint32_t = unsigned int;
    char pad_0[0x7]; // 0x00
    int8_t busy; // 0x07
    char pad_8[0x18];
    bool allowPause; // 0x20
    char pad_21[0x10]; // 0x21
    bool isPaused; // 0x31
    char pad_32[0xA];
    uint32_t restartLastEvent; // 0x3C
    char pad_40a[0x30];
    sResource* enemyResource[16]; // 0x70
    char pad_40b[0x64];
    pauseMenu* pauseMenuPtr1; // 0x114
};

static_assert(sizeof(aRoom) == 0x118);

class sArea {
public:
    char pad_0x0000[0x3830];           // 0x0000
    aGame* aGamePtr;                   // 0x3830 aGame
    currentMission* currentMissionPtr; // 0x3834
    aRoom* aRoomPtr;       // x03838
}; // Size: 0x383C
static_assert(sizeof(sArea) == 0x383C);
