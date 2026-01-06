#pragma once

#include <cstdint>

enum class GameDifficulty : uint32_t {
    HUMAN                 = 0,
    DEVIL_HUNTER          = 1,
    SON_OF_SPARDA_HOH_HAH = 2,
    DANTE_MUST_DIE        = 3,
    LEGENDARY_DARK_KNIGHT = 4,
};

enum class GameMode : uint32_t {
    DEFAULT        = 0,
    HEAVEN_OR_HELL = 1,
    HELL_AND_HELL  = 2,
};

class sMediator { // TODO(): actually inherits from cSystem i think
public:
    char pad_0[0x24];
    class uPlayer* player_ptr; // 0x24
    char pad_28[0x88];
    class uEnemy_Old* uBoss1; // 0xb0
    char pad_b4[0x1c];
    class uCameraCtrl* camera1; // 0xd0
    char pad_d4[0xc];
    uint32_t missionID; // 0xe0 // always shows 50 for BP
    uint32_t roomID;    // 0xe4
    char pad_e8[0x2c];
    uint32_t orbCount;            // 0x114
    uint32_t orbMissionCurrent;   // 0x118
    uint32_t orbMissionPotential; // 0x11C
    char pad_118[0x14];
    float enemyDifficulty; // 0x134
    char pad_138[0x4];
    uint32_t hellGoldOrbCount;     // 0x13c
    GameDifficulty gameDifficulty; // 0x140
    GameMode gameMode;             // 0x144
    char pad_148[0x4];
    float stylePoints; // 0x14C
    char pad_150[0x68];
    class uEnemy_Old* uEnemies[10]; // 0x1b8
    uint32_t enemyCount[3];         // 0x1e0
    char pad_1ec[0x64];
    float bpTimer; // 0x250
    char pad_254[0x114];
    bool hasUnlockedDT;      // 0x368
    bool hasUnlockedAirHike; // 0x369
    char pad_36a[0x1];
    bool hasUnlockedSprint; // 0x36b
    char pad_36c[0x36];
    bool hasUnlockedHold;   // 0x3a2
    bool hasUnlockedMaxAct; // 0x3a3
    char pad_3a4[0x5];
    bool displayDisasterGauge; // 0x3a9
    char pad_3aa[0xea];
    bool turboEnabled; // 0x494
    char pad_495[0x38b];
    sWorkRate workRate; // 0x820
}; // Size: 0x878
static_assert(sizeof(sMediator) == 0x878);