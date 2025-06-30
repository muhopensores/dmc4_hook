#pragma once
#ifndef ENEMY_HPP
#define ENEMY_HPP
#endif

#include "MtMath.hpp"
#include "uCollisionMgr.hpp"

class uEnemy {
public:
    uintptr_t vtable;  // 0x0
    int flags;         // 0x4
    uEnemy* nextEnemy; // 0x8
    char pad_c[0x4];
    float delta;      // 0x10
    uint8_t moveID;   // 0x14
    uint8_t movePart; // 0x15
    char pad_16[0x1a];
    Vector3f position; // 0x30
    char pad_3c[0x4];
    Vector3f rotation; // 0x40
    char pad_4c[0x4];
    Vector3f scale; // 0x50
    char pad_5c[0x74];
    Vector3f positionKnockback; // 0xd0
    char pad_dc[0x14];
    Vector3f rotationKnockback; // 0xf0
    char pad_fc[0x34];
    bool isLit; // 0x130
    char pad_131[0x3];
    bool causeShadow; // 0x134
    char pad_135[0x1ab];
    int m_joint_array_size; // 0x2e0
    UModelJointArr* joints; // 0x2e4
    int something_size;     // 0x2e8
    char pad_2ec[0x38];
    bool playAnims; // 0x324
    char pad_325[0xf];
    uint16_t animID; // 0x334
    char pad_336[0x12];
    float animFrame;    // 0x348
    float animFrameMax; // 0x34c
    char pad_350[0xb50];
    uint16_t launchStateThing1; // 0xea0
    char pad_ea2[0x6];
    uint16_t launchStateThing2; // 0xea8
    char pad_eaa[0x46];
    int lockOnSphereCount; // 0xef0
    char pad_ef4[0xc];
    lockOnSphereData jcSpheres[5]; // 0xf00
    char pad_ff0[0x320];
    uintptr_t something;                  // 0x1310
    uintptr_t something2;                 // 0x1314
    kEmJumpDataArr* enemyStepSphereArray; // 0x1318
    int m_enemystepSphereCount;           // 0x131c
    int intAt1320;                        // 0x1320
    char pad_1324[0xe0];
    bool inBattle; // 0x1404
    bool isActive; // 0x1405
    char pad_1406[0xa];
    ENEMY_ID ID; // 0x1410
    int team;    // 0x1414
    char pad_1418[0x70];
    float DTTimer; // 0x1488
    bool DT;       // 0x148c
    char pad_148d[0x73];
    uEnemyDamagePossibility DamagePtrMephistoFaustAssaultGladius; // 0x1500
    uEnemyDamagePossibility DamagePtrAltoBiancoFrostBlitzBoss;    // 0x1504
    uEnemyDamagePossibility DamagePtrCutlass;                     // 0x1508
    char pad_150c[0x20];
    uEnemyDamagePossibility DamagePtrScarecrowLegArmMega; // 0x152c
    char pad_1530[0xd0];
    uCollisionMgr collisionSettings; // 0x1600
    char pad_19f0[0x150];
    Vector3f velocity; // 0x1b40
    char pad_1b4c[0x20];
    float berialFireHalf; // 0x1b6c
    float berialFireMax;  // 0x1b70
    char pad_1b74[0x8];
    float berialFireTimer; // 0x1b7c
    float berialFire;      // 0x1b80
    char pad_1b84[0x3ec];
    float assaultDifficulty; // 0x1f70
    char pad_1f74[0xe8];
    float gladiusTimer; // 0x205c
    char pad_2060[0x5c];
    float faustCloakTimer; // 0x20bc
    char pad_20c0[0xc4];
    float blitzElectric;                     // 0x2184
    float blitzElectricSuicideHPRequirement; // 0x2188
    float blitzElectricTimer;                // 0x218c
    float blitzElectricSuicideTimer;         // 0x2190
    char pad_2194[0x40];
    bool gladiusBuried; // 0x21d4
    char pad_21d5[0x10b];
    float faustCloak;    // 0x22e0
    float faustCloakMax; // 0x22e4
    char pad_22e8[0x14e0];
    float angeloShield;    // 0x37c8
    float angeloShieldMax; // 0x37cc
    char pad_37d0[0xb4];
    float credoShieldTimer; // 0x3884
    char pad_3888[0x34];
    float credoShield; // 0x38bc
    char pad_38c0[0x4704];
    uEnemyDamagePossibility DamagePtrBasilisk; // 0x7fc4
    char pad_7fc8[0x18f8];
    int frostHealCount; // 0x98c0
    char pad_98c4[0xf03c];
    float sanctusShieldM11; // 0x18900
    char pad_18904[0x8];
    float sanctusShieldTimerM11; // 0x1890c
    char pad_18910[0x56e4];
    float sanctusShieldM20; // 0x1dff4
    char pad_1dff8[0x8];
    float sanctusShieldTimerM20; // 0x1e000
};                               // Size: 0x1e004
static_assert(sizeof(uEnemy) == 0x1e004);