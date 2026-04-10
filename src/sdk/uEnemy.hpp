#pragma once
#ifndef ENEMY_HPP
#define ENEMY_HPP
#endif

#include "MtMath.hpp"
#include "uCollisionMgr.hpp"
#include "uActor.hpp"

#define lockOnSphereData LOCKONSPHERE_DAT // TODO(): to compile for now

enum ENEMY_ID : int {
  SCARECROW_LEG = 0,
  SCARECROW_ARM = 1,
  CHIMERA_LEG = 2, // not actually but this is convenient
  SCARECROW_MEGA = 3,
  CHIMERA_ARM = 4, // not actually but this is convenient
  ANGELO_BIANCO = 5,
  ANGELO_ALTO = 6,
  CHIMERA_MEGA = 7,
  MEPHISTO = 8,
  FAUST = 9,
  FROST = 10,
  ASSAULT = 11,
  BLITZ = 12,
  CHIMERA = 13,
  CHIMERA_ASSAULT = 14, // actually just infected enemy
  CUTLASS = 15,
  GLADIUS = 16,
  BASILISK = 17,
  BERIAL = 18,
  BAEL = 19,
  EM020 = 20,
  ECHIDNA = 21,
  CREDO = 22,
  AGNUS = 23,
  EM024 = 24,
  SAVIOR = 25,
  SAVIOR2 = 26,
  AGNUS_HUMAN = 27,
  EM028 = 28,
  SANCTUS_M11 = 29,
  SANCTUS_M20 = 30,
  AGNUS_AGAIN = 31,
  EM32 = 32,
  SAVIOR_AGAIN = 33,
  EM34 = 34,
  COMBAT_ADJUDICATOR = 35,
  KYRIE = 36,
};

class uEnemy_Old {
public:
    uintptr_t vtable;  // 0x0
    int flags;         // 0x4
    uEnemy_Old* nextEnemy; // 0x8
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
static_assert(sizeof(uEnemy_Old) == 0x1e004);

class uEnemy : public uActor {
    
};

class uEnemySomething
{
public:
	char pad_0000[4992]; //0x0000
	Vector3f m_spawn_coords; //0x1380
	char pad_138_c[8]; //0x138C
	uint8_t n00004023; //0x1394
	char pad_1395[3]; //0x1395
	int32_t m_enemy_spawn_effect_something; //0x1398
	char pad_139_c[12]; //0x139C
	uint32_t unk_type_something; //0x13A8
	uint32_t n00003_fef; //0x13AC
	uint32_t n00003_f_f0; //0x13B0
	uint32_t n00003_f_f1; //0x13B4
	uint32_t n00003_f_f2; //0x13B8
	uint32_t n00003_f_f3; //0x13BC
	Matrix4x4 n00003_f_f4; //0x13C0
	uint32_t n00003_f_f5; //0x1400
	uint8_t n00003_f_f6; //0x1404
	uint8_t n00004033; //0x1405
	uint8_t n00004037; //0x1406
	char pad_1407[1]; //0x1407
	float n00003_f_f7; //0x1408
	uint32_t n00003_f_f8; //0x140C
	uint32_t n00003_f_f9; //0x1410
	uint32_t n00003_ffa; //0x1414
	uint32_t n0000403_d; //0x1418 Ptr
	char pad_141_c[24]; //0x141C
	float n00004000; //0x1434
	uint8_t n00004001; //0x1438
	char pad_1439[3]; //0x1439
	uint32_t n00004002; //0x143C
	uint32_t n00004003; //0x1440
	uint32_t n00004004; //0x1444
	char pad_1448[228]; //0x1448
	bool n00003_c7_b; //0x152C
	bool n00003_fdd; //0x152D
	bool n00003_f_e0; //0x152E
	char pad_152_f[4409]; //0x152F
}; //Size: 0x2668
static_assert(sizeof(uEnemySomething) == 0x2668);