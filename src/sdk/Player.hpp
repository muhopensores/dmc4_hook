#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP
#endif
#include "Enemy.hpp"
#include "uActor.hpp"

class uPlayer : public UCoord {
public:
    char pad_e0[0x10];
    Vector4f rotationKnockback; // 0xf0
    char pad_100[0x30];
    uint8_t armShadows; // 0x130
    char pad_131[0x3];
    uint8_t causeShadows; // 0x134
    char pad_135[0x3];
    uint8_t bodyShadows; // 0x138
    char pad_139[0x43];
    uint32_t boneMapping[5]; // 0x17c
    char pad_190[0xec];
    uint32_t meshCount; // 0x27c
    char pad_280[0x24];
    uPlayer* player_ptr; // 0x2a4
    char pad_2a8[0x38];
    int m_joint_array_size;      // 0x2e0
    UModelJointArr* joint_array; // 0x2e4
    char pad_2e8[0x4c];
    uint16_t animID; // 0x334
    char pad_336[0x12];
    float animFrame;    // 0x348
    float animFrameMax; // 0x34c
    char pad_350[0x6e8];
    MotionPtr* motionPtr1; // 0xa38
    MotionPtr* motionPtr2; // 0xa3c
    char pad_a40[0x2b4];
    float speedUnsettable; // 0xcf4
    char pad_cf8[0x1ac];
    uint8_t damageIDMaybe; // 0xea4
    char pad_ea5[0x3];
    uint8_t grounded; // 0xea8
    char pad_ea9[0x17];
    float groundInertiaX; // 0xec0
    float inertiaY;       // 0xec4
    float groundInertiaZ; // 0xec8
    char pad_ecc[0x335];
    bool hitstop; // 0x1201
    char pad_1202[0xe];
    float facingDirection; // 0x1210
    char pad_1214[0x160];
    uint16_t buttonInputRaw; // 0x1374
    uint16_t stickInputRaw;  // 0x1376
    char pad_1378[0x4c];
    float stickDirection; // 0x13c4
    char pad_13c8[0x44];
    uint8_t inputHold[4];    // 0x140c
    uint8_t inputPress[4];   // 0x1410
    uint8_t inputRelease[4]; // 0x1414
    char pad_1418[0x4];
    uint32_t stickThreshold;        // 0x141c
    float stickFacingWorldAdjusted; // 0x1420
    bool stickPastThreshold;        // 0x1424
    char pad_1425[0x23];
    uint8_t isExceeding; // 0x1448
    char pad_1449[0x1b];
    float characterSpeed; // 0x1464
    char pad_1468[0x2c];
    uint32_t controllerID; // 0x1494
    char pad_1498[0x58];
    uint32_t canWeaponChange; // 0x14f0
    char pad_14f4[0x4];
    uint32_t isLockonAnimation; // 0x14f8
    char pad_14fc[0x4];
    uint32_t moveBank; // 0x1500
    uint32_t movePart; // 0x1504
    char pad_1508[0x1];
    bool isActive; // 0x1509
    char pad_150a[0x3];
    bool roomCollision; // 0x150d
    char pad_150e[0x1];
    bool enemyCollision;     // 0x150f
    bool triggerBigJump;     // 0x1510
    bool triggerStingerJump; // 0x1511
    char pad_1512[0x3e];
    uint8_t cancels1[4]; // 0x1550
    uint8_t cancels2[4]; // 0x1554
    char pad_1558[0xc];
    uint32_t moveID2; // 0x1564
    char pad_1568[0x4c];
    uDamage damageStruct; // 0x15b4
    char pad_1664[0x48];
    uint32_t walkType; // 0x16ac
    char pad_16b0[0x10];
    float inertiaDirectionReal; // 0x16c0
    char pad_16c4[0xc];
    bool lockedOn; // 0x16d0
    char pad_16d1[0x3f];
    float rotation2; // 0x1710
    char pad_1714[0x6c];
    float groundInertia; // 0x1780
    char pad_1784[0x1c];
    uintptr_t* collisionPtr1; // 0x17a0
    char pad_17a4[0x5db];
    uint8_t canExceed; // 0x1d7f
    char pad_1d80[0x14];
    DevilArm* gilgamesh;  // 0x1d94
    DevilArm* lucifer;    // 0x1d98
    DevilArm* ebonyIvory; // 0x1d9c
    DevilArm* coyoteA;    // 0x1da0
    DevilArm* pandora;    // 0x1da4
    char pad_1da8[0xc];
    int nextSword;    // 0x1db4
    uint32_t nextGun; // 0x1db8
    char pad_1dbc[0x4];
    uint32_t currentSword; // 0x1dc0
    uint32_t currentGun;   // 0x1dc4
    char pad_1dc8[0x4];
    kAtckDefTbl* kAtckDefTblPtr; // 0x1dcc
    char pad_1dd0[0x40];
    uint8_t meleeCancelPermissions1[4]; // 0x1e10
    uint8_t jumpCancelPermissions1[4];  // 0x1e14
    char pad_1e18[0x4];
    float inertia; // 0x1e1c
    char pad_1e20[0x4];
    float rotation3; // 0x1e24
    char pad_1e28[0x28];
    Vector3f m_d_velocity; // 0x1e50
    char pad_1e5c[0x4];
    float m_d_vel_magnitude;    // 0x1e60
    float directionOfMagnitude; // 0x1e64
    float hitstopTimer;         // 0x1e68
    char pad_1e6c[0xf];
    uint8_t airAttackCount; // 0x1e7b
    char pad_1e7c[0x1];
    uint8_t weight;       // 0x1e7d
    uint8_t airHikeCount; // 0x1e7e
    bool canAirHike;      // 0x1e7f
    char pad_1e80[0xc];
    uCollisionMgr* collisionSettings; // 0x1e8c
    char pad_1e90[0xc];
    CharacterSettingsTwo* charactersettingsTwo; // 0x1e9c
    char pad_1ea0[0x8];
    CharacterSettingsThree* charactersettingsThree; // 0x1ea8
    char pad_1eac[0x78];
    float DT;    // 0x1f24
    float maxDT; // 0x1f28
    char pad_1f2c[0x24];
    bool fullHouseSomething1; // 0x1f50
    char pad_1f51[0x2e];
    bool canWallHike; // 0x1f7f
    char pad_1f80[0x74];
    float jcTimer; // 0x1ff4
    char pad_1ff8[0x8];
    uEnemy* lockOnTargetPtr1; // 0x2000
    char pad_2004[0x1];
    bool enemyStepEnabled; // 0x2005
    char pad_2006[0x2];
    bool grounded2; // 0x2008
    char pad_2009[0x167];
    Vector3f location2; // 0x2170
    char pad_217c[0x50];
    bool tiltForward; // 0x21cc
    char pad_21cd[0x13];
    bool tiltBack; // 0x21e0
    char pad_21e1[0x217];
    float roundTripCharge; // 0x23f8
    char pad_23fc[0x3fc];
    bool tiltBackForward; // 0x27f8
    char pad_27f9[0xab];
    uintptr_t* animFrameInfo; // 0x28a4
    char pad_28a8[0x16];
    uint16_t playMoveOnce; // 0x28be
    char pad_28c0[0x9c];
    uint32_t moveID3; // 0x295c
    char pad_2960[0x38];
    uint32_t moveIDBest; // 0x2998
    char pad_299c[0x6e4];
    uEnemy* lockOnTargetPtr2; // 0x3080
    uEnemy* lockOnTargetPtr3; // 0x3084
    char pad_3088[0x3c];
    uint8_t bufferPermissions; // 0x30c4
    char pad_30c5[0x2b];
    uint8_t meleeCancelPermissions2; // 0x30f0
    char pad_30f1[0x2B];
    uint8_t unknPermission1; // 0x311c
    char pad_311d[0x2B];
    uint8_t directionalMeleePermissions; // 0x3148
    char pad_3149[0x2B];
    uint8_t jumpCancelPermissions2; // 0x3174
    char pad_3175[0x3];
    uint8_t bufferedJump; // 0x3178
    char pad_3179[0x27];
    uint8_t unknPermission2; // 0x31a0
    char pad_31a1[0x2B];
    uint8_t gunCancelPermissions2; // 0x31cc
    char pad_31cd[0x2B];
    uint8_t unknPermission3; // 0x31f8
    char pad_31f9[0x9AEF];
    uint8_t exceedLevel; // 0xcce8
    char pad_cce9[0xd3];
    float exceedTimer; // 0xcdbc
    char pad_cdc0[0x7f14];
    bool guardReq1; // 0x14cd4
    bool guardReq2; // 0x14cd5
    char pad_14cd6[0x5e];
    bool dtOutfit; // 0x14d34
    char pad_14d35[0x3];
    bool dtActive; // 0x14d38
    char pad_14d39[0xb];
    float guardTimer; // 0x14d44
    char pad_14d48[0x50];
    int currentStyle; // 0x14d98
    char pad_14d9c[0x10];
    float revengeGauge; // 0x14dac
    char pad_14db0[0xc];
    LuciferPin* luciferPins[15]; // 0x14dbc
    char pad_14df8[0x3fc];
    float disasterGauge; // 0x151f4
    char pad_151f8[0xa8];
    int dreadnaught; // 0x152a0
    char pad_152a4[0x7c4];
    uEnemy* lockOnTargetPtr4; // 0x15a68
};                            // Size: 0x15a6c