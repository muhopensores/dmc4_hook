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

class currentRoom {
public:
    using uint32_t = unsigned int;
    char pad_0[0x20];
    bool allowPause; // 0x20
    char pad_21[0x10];
    bool isPaused; // 0x31
    char pad_32[0xa];
    uint32_t restartLastEvent; // 0x3c
    char pad_40[0xd4];
    pauseMenu* pauseMenuPtr1; // 0x114
}; // Size: 0x118
static_assert(sizeof(currentRoom) == 0x118);

class sArea {
public:
    char pad_0x0000[0x3830];           // 0x0000
    aGame* aGamePtr;                   // 0x3830 aGame
    currentMission* currentMissionPtr; // 0x3834
    currentRoom* currentRoomPtr;       // x03838
}; // Size: 0x383C
static_assert(sizeof(sArea) == 0x383C);