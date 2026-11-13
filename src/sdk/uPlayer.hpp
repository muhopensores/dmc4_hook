#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP
#endif
#include "sdk/uPlayerDante.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uActor.hpp"
#include <cstddef>

class CharacterSettingsTwo;
class CharacterSettingsThree;

struct kAtckDefTbl {
    uint32_t atckAttr;  // 0x0000
    uint32_t atckId;    // 0x0004
    uint32_t atckLevel; // 0x0008
    uint32_t atckInfo;  // 0x000C
    union {
        uint32_t buffer; // 0x0010
        struct {
            uint8_t atckCommand;   // 0x0010
            uint8_t atckCommandNo; // 0x0011
            uint8_t atckCondition; // 0x0012
            uint8_t ukn;           // 0x0013
        };
    } command;
    uint32_t atckConditionWp;    // 0x0014
    uint32_t atckConditionStyle; // 0x0018
    uint32_t ukn;                // 0x001C (aerial lock)
    uint32_t atckAs;             // 0x0020
    uint32_t cancelId[5];        // 0x0024, 0x0028, 0x002C, 0x0030, 0x0034
}; // Size: 0x38
static_assert(sizeof(kAtckDefTbl) == 0x38);

class MotionData {
public:
    char pad_0[0x4];
    uint32_t hitboxStartTime; // 0x4
    char pad_8[0xc];
    uint32_t passThroughEnemiesIdk; // 0x14
    char pad_18[0x24];
    uint32_t sfxTiming; // 0x3c
    char pad_40[0x10];
    uint32_t sfxToPlay; // 0x50
}; // Size: 0x54
static_assert(sizeof(MotionData) == 0x54);

class InnerMotionPtr {
public:
    MotionData motionData[100]; // 0x0
}; // Size: 0x20d0
static_assert(sizeof(InnerMotionPtr) == 0x20d0);

class MotionPtr {
public:
    char pad_0[0x4];
    char motionString1[30]; // 0x4
    char pad_22[0x17e];
    class InnerMotionPtr* innerMotionPtr1; // 0x1a0
}; // Size: 0x1a4
static_assert(sizeof(MotionPtr) == 0x1a4);

class uPlayer_old : public uCoord {
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
    uPlayer_old* player_ptr; // 0x2a4
    char pad_2a8[0x38];
    int m_joint_array_size;      // 0x2e0
    UModelJointArr* joint_array; // 0x2e4
    char pad_2e8[0x4c];
    uint16_t animID; // 0x334
    char pad_336[0x12];
    float animFrame;    // 0x348
    float animFrameMax; // 0x34c
    char pad_350[0x176];
    uint8_t charAt4C6; // 0x4c6
    char pad_4C7[0x571];
    MotionPtr* motionPtr1; // 0xa38
    MotionPtr* motionPtr2; // 0xa3c
    char pad_a40[0x2b4];
    float speedUnsettable; // 0xCF4
    char pad_CF8[0x16C];
    int playerID;          // 0xE64
    char pad_E68[0x3C];
    uint8_t damageIDMaybe; // 0xEA4
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
    float stickDirection;    // 0x13C4
    char pad_13C8[0x3D];
    uint8_t padID;           // 0x1405
    char pad_1406[0x06];
    uint8_t inputHold[4];    // 0x140C
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
    uint32_t cancels2; // 0x1554
    char pad_1558[0xc];
    uint32_t moveID2; // 0x1564
    char pad_1568[0x4c];
    uDamage_Old damageStruct; // 0x15b4
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
    char pad_1ff8[0x4];
    uEnemy_Old* lockOnTargetPtr0; // 0x1ffc
    uEnemy_Old* lockOnTargetPtr1; // 0x2000
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
    uEnemy_Old* lockOnTargetPtr2; // 0x3080
    uEnemy_Old* lockOnTargetPtr3; // 0x3084
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
    char pad_cdc0[0x38]; // 0xcdc0
    void* stand; // 0xcdf8
    char pad_cdfc[0x7ED4];
    float releaseTimer; // 0x14cd0
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
    char pad_14df8[0x360];
    cCollisionGroup* legsCol; // 0x15158
    cCollisionGroup* bodyCol; // 0x1515C
    cCollisionGroup* airCol;  // 0x15160
    cCollisionGroup* iFrameCol; // 0x15164
    char pad_15168[0x8C]; // 0x15168
    float disasterGauge; // 0x151F4
    float mPdMeteorMoveSpd; // 0x151F8
    float mPdMeteorMoveSpdMax; // 0x151fc
    float mPdMeteorRotSpd; // 0x151200
    float mPdMeteorRotBlendSpd; // 0x151204
    char pad_15208[0x98];
    int dreadnaught; // 0x152A0
    char pad_152A4[0x7C4];
    uEnemy_Old* lockOnTargetPtr4; // 0x15a68
}; // Size: 0x15a6c
static_assert(sizeof(uPlayer_old) == 0x15a6c);

// The DX9 uPlayer, from the IDB's uPlayer::uPlayer (0x3270; the base is the SDK's DX9 uActor, 0x1370). Nested
// types keep the IDB's names; pointers to types the SDK doesn't have are void* with the IDB type in a comment.
// Offsets were checked against uPlayer::uPlayer 0x7A5B30 and the readers it names (static_asserts below).
class uPlayer : public uActor {
public:
    enum ACT_STAT : int32_t {
        AST_NONE = 0,
        AST_STAND = 1,
        AST_JUMP = 2,
        AST_ATCK = 4,
        AST_POSE = 8,
        AST_GUARD = 16,
        AST_RUN = 32,
        AST_FLY = 64,
    };
    enum STYLE_KIND : int32_t {
        STYLE_KIND_SHORT = 0,
        STYLE_KIND_LONG = 1,
        STYLE_KIND_ESCAPE = 2,
        STYLE_KIND_GUARD = 3,
        STYLE_KIND_YAMATO = 4,
        STYLE_KIND_MAX = 5,
        STYLE_KIND_NONE = -1,
    };
    enum STYLE_STATUS : int32_t {
        STYLE_ST_NONE = 0,
        STYLE_ST_GUARD = 1,
        STYLE_ST_TIME = 2,
        STYLE_ST_SPLIT = 3,
    };
    enum STYLE_LEVEL : int32_t {
        STYLE_LEVEL1 = 0,
        STYLE_LEVEL2 = 1,
        STYLE_LEVEL3 = 2,
        STYLE_LEVEL_NUM = 3,
    };
    enum STYLE_SIDE : int32_t {
        STYLE_SIDE_LEFT = 0,
        STYLE_SIDE_RIGHT = 1,
        STYLE_SIDE_NONE = 2,
    };
    enum DMAGE_ID : int32_t {
        DMG_ID_DUMMY = 0,
        DMG_ID_HIT = 1,
        DMG_ID_ZURI = 2,
        DMG_ID_BUT = 3,
        DMG_ID_ICE = 4,
        DMG_ID_KIRI = 5,
        DMG_ID_HITIME = 6,
        DMG_ID_WIND_PRESSURE_S = 7,
        DMG_ID_WIND_PRESSURE_M = 8,
        DMG_ID_WIND_PRESSURE_L = 9,
        DMG_ID_COMBO = 10,
        DMG_ID_AIR_COMBO = 11,
        DMG_ID_BLITZ_ELECTRIGGER = 12,
        DMG_ID_AGNUS_ABSORB = 13,
        DMG_ID_AGNUS_ABSORB_HIGH = 14,
        DMG_ID_AGNUS_ABSORB_HIGH_AIR = 15,
        DMG_ID_BAEL_EAT = 16,
        DMG_ID_BAEL_EAT_RELEASE = 17,
        DMG_ID_BAEL_EAT_RELEASE_DT = 18,
        DMG_ID_BAEL_EAT_DEAD = 19,
        DMG_ID_BAEL_BAIT_ICE = 20,
        DMG_ID_GRAB_RELEASE = 21,
        DMG_ID_GRAB_RELEASE_POSE = 22,
        DMG_ID_GRAB_DEAD = 23,
        DMG_ID_BUT_DEAD = 24,
        DMG_ID_FALL_DEAD = 25,
        DMG_ID_UKEMI = 26,
    };
    enum class uDamage__UDAMAGE_TYPE : int32_t {
        PLAYER = 0,
        SHELL = 1,
        ENEMY = 2,
        ENEMY_SHELL = 3,
        SET = 4,
        STAGE = 5,
        BUTOU = 6,
        BOSS_DANTE = 7,
        OTHER = 8,
    };
    enum RUN_MODE : int32_t {
        RUN_MODE_NONE = 0,
        RUN_MODE_WALK = 1,
        RUN_MODE_RUN = 2,
    };
    enum JUMP_TYPE : int32_t {
        JUMP_TYPE_F = 0,
        JUMP_TYPE_L = 1,
        JUMP_TYPE_B = 2,
        JUMP_TYPE_R = 3,
        JUMP_TYPE_V = 4,
        JUMP_TYPE_MAX = 5,
        JUMP_TYPE_FALL = 6,
        JUMP_TYPE_ESCAPE_FALL = 7,
        JUMP_TYPE_ATCK_AFTER = 8,
    };
    enum JUMP_MODE : int32_t {
        JUMP_MODE_NORM_SINGLE_LO = 0,
        JUMP_MODE_NORM_SINGLE_HI = 1,
        JUMP_MODE_NORM_W_LO = 2,
        JUMP_MODE_NORM_W_HI = 3,
        JUMP_MODE_NORM_WALL_LO = 4,
        JUMP_MODE_NORM_WALL_HI = 5,
        JUMP_MODE_NORM_EMJUMP_LO = 6,
        JUMP_MODE_NORM_EMJUMP_HI = 7,
        JUMP_MODE_LOCK_SINGLE_LO = 8,
        JUMP_MODE_LOCK_SINGLE_HI = 9,
        JUMP_MODE_WALLHIKE = 10,
        JUMP_MODE_CATAPULT = 11,
        JUMP_MODE_MAX = 12,
    };
    enum MOT_TYPE : int32_t {
        MOT_BASE = 0,
        MOT_BLEND1 = 1,
        MOT_BLEND2 = 2,
        MOT_BLEND3 = 3,
        MOT_BLEND4 = 4,
        MOT_BLEND5 = 5,
        MOT_BLEND6 = 6,
        MOT_BLEND7 = 7,
        MOT_TYPE_NUM = 8,
    };
    enum EFCT_TYPE : int32_t {
        EFCT_TYPE_ERASE_ACT_CHG = 0,
        EFCT_TYPE_ERASE_RESET = 1,
        EFCT_TYPE_FINISH_ACT_CHG = 2,
        EFCT_TYPE_NOT_ERASE = 3,
    };
    enum LAND_EFCT_TYPE : int32_t {
        LE_LEFT_FRONT_MAN = 0,
        LE_RIGHT_FRONT_MAN = 1,
    };
    struct kAnlg {
        uint16_t x; // 0x0
        uint16_t y; // 0x2
        float len; // 0x4
        float radian; // 0x8
        uint32_t dgtlOn; // 0xc
        uint32_t dgtlTrg; // 0x10
        uint32_t dgtlRel; // 0x14
    };
    static_assert(sizeof(kAnlg) == 0x18);
    struct cPeripheral {
        char pad_0[0x4];
        uint32_t mPadBtnOn; // 0x4
        uint32_t mPadBtnTrg; // 0x8
        uint32_t mPadBtnRel; // 0xc
        float mPadBtnPress[15]; // 0x10
        kAnlg mAnlgL; // 0x4c
        kAnlg mAnlgR; // 0x64
        kAnlg mHoldAnlgL; // 0x7c
        bool mIsHold; // 0x94
        char pad_95[0x3];
    };
    static_assert(sizeof(cPeripheral) == 0x98);
    struct cInterface {
        char pad_0[0x4];
        uint32_t mBtnOn; // 0x4
        uint32_t mBtnTrg; // 0x8
        uint32_t mBtnRel; // 0xc
        uint32_t mBtnOld; // 0x10
        uint32_t pow; // 0x14
        char pad_18[0x34];
    };
    static_assert(sizeof(cInterface) == 0x4c);
    struct kParam {
        bool mIsMove; // 0x0
        char pad_1[0x3];
        float mNow; // 0x4
        float mTarget; // 0x8
        float mMax; // 0xc
    };
    static_assert(sizeof(kParam) == 0x10);
    struct cProcSpdCtrl {
        char pad_0[0x4];
        uActor* mpParent; // 0x4
        void*  /* cWorkRate * */ mpWorkRate; // 0x8
        kParam mParam[2]; // 0xc
        float mBaseDeltaTime; // 0x2c
    };
    static_assert(sizeof(cProcSpdCtrl) == 0x30);
    struct cHitSlowCtrl {
        char pad_0[0x4];
        bool mIsMove; // 0x4
        char pad_5[0x3];
        float mTimer; // 0x8
        float mRate; // 0xc
    };
    static_assert(sizeof(cHitSlowCtrl) == 0x10);
    struct kBdyAction {
        int32_t no; // 0x0
        int32_t phase; // 0x4
    };
    static_assert(sizeof(kBdyAction) == 0x8);
    struct kStingerJumpParam {
        bool mIsValid; // 0x0
        bool mIsEventJump; // 0x1
        char pad_2[0x2];
        float mSpdZ; // 0x4
        float mSpdY; // 0x8
        float mAddY; // 0xc
        float mAngle; // 0x10
        char pad_14[0xc];
        MtVector3 mSetPos; // 0x20
    };
    static_assert(sizeof(kStingerJumpParam) == 0x30);
    struct kStyleParam {
        STYLE_KIND kind; // 0x0
        STYLE_STATUS status; // 0x4
        STYLE_LEVEL level; // 0x8
        STYLE_SIDE slide; // 0xc
    };
    static_assert(sizeof(kStyleParam) == 0x10);
    struct RnoFlags {
        uint32_t mRno0 : 8;
        uint32_t mRno1 : 8;
        uint32_t mRno2 : 8;
        uint32_t mRno3 : 8;
    };
    static_assert(sizeof(RnoFlags) == 0x4);
    struct cUnit {
        void* vtable_ptr; // 0x0
        uint32_t mBeFlag : 3;
        uint32_t mMoveLine : 7;
        uint32_t mUnitAttr : 6;
        uint8_t mTransMode; // 0x8
        uint8_t mTransView; // 0x9
        cUnit* mp_next_unit; // 0xc
        cUnit* mp_prev_unit; // 0x10
        float m_delta_time; // 0x14
        RnoFlags mRno; // 0x18
    };
    static_assert(sizeof(cUnit) == 0x1c);
    struct uDamage__uDamage {
        cUnit cUnitBase; // 0x0
        float mHitPoint; // 0x1c
        float mMaxHitPoint; // 0x20
        int32_t mPointCorrect; // 0x24
        int32_t mLvCorrect; // 0x28
        float mDamageCorrect; // 0x2c
        float mDamageValue; // 0x30
        float mActualDamageValue; // 0x34
        int32_t mInterrupt[5]; // 0x38
        int32_t mBlown[5]; // 0x4c
        int32_t mMaxInterrupt[5]; // 0x60
        int32_t mMaxBlown[5]; // 0x74
        uint32_t mState; // 0x88
        uint32_t mEffect; // 0x8c
        bool mIsDead; // 0x90
        char pad_91[0x3];
        uintptr_t mpSoundRequest; // 0x94
        uint32_t mGuardType; // 0x98
        bool mResetInterruptReq; // 0x9c
        bool mResetBlownReq; // 0x9d
        bool mIsEmToEm; // 0x9e
        char pad_9f[0x1];
        void*  /* kAttackStatus * */ mpLastAttackStatus; // 0xa0
        int32_t mEffectReduceCount; // 0xa4
        float mEffectReduceTimer; // 0xa8
        char pad_ac[0x4];
        float mTimer; // 0xb0
        float mStateTimer; // 0xb4
        void*  /* kAttackStatus * */ mpAttackStatus; // 0xb8
        void*  /* kDefendStatus * */ mpDefendStatus; // 0xbc
        uActor* mpAttackActor; // 0xc0
        uActor* mpDefendActor; // 0xc4
        uModel* mpModel; // 0xc8
        uDamage__UDAMAGE_TYPE mType; // 0xcc
        uint32_t mComboRank; // 0xd0
        uintptr_t mpDefendStatusData; // 0xd4
    };
    static_assert(sizeof(uDamage__uDamage) == 0xd8);
    struct kEffectElementMessageParam {
        uint32_t mUParam; // 0x0
        int32_t mSParam; // 0x4
        float mFParam[4]; // 0x8
    };
    static_assert(sizeof(kEffectElementMessageParam) == 0x18);
    struct kDamageParam {
        DMAGE_ID type; // 0x0
        uint8_t element; // 0x4
        char pad_5[0x3];
        uActor* pActor; // 0x8
        void*  /* kAttackStatus * */ pAtckStat; // 0xc
        float ang; // 0x10
        int32_t bloodType; // 0x14
        DMAGE_ID typeReserve; // 0x18
        float pushTimer; // 0x1c
        float pushSpd; // 0x20
        float pushAng; // 0x24
        DMAGE_ID pushType; // 0x28
        float iceRestrainTimer; // 0x2c
        bool isIceDmgNoFall; // 0x30
        char pad_31[0x3];
        int32_t level; // 0x34
        uDamage__uDamage mgr; // 0x38
        kEffectElementMessageParam effElemParam; // 0x110
        bool isBaelEatLand; // 0x128
        char pad_129[0x3];
    };
    static_assert(sizeof(kDamageParam) == 0x12c);
    struct kJumpParam {
        cResource* rsrc; // 0x0
        uint32_t data; // 0x4
        JUMP_TYPE type; // 0x8
        JUMP_MODE mode; // 0xc
        float moveAng; // 0x10
    };
    static_assert(sizeof(kJumpParam) == 0x14);
    struct kPoseParam {
        int32_t dir; // 0x0
        int32_t oldDir; // 0x4
        float ang; // 0x8
        int32_t stat; // 0xc
        float holdTimer; // 0x10
        int32_t wpnId; // 0x14
        uint32_t motTbl; // 0x18
        float blendWeight; // 0x1c
        bool isUpSide; // 0x20
        float maxRad; // 0x24
        float minRad; // 0x28
        bool isPoseBeginSeCall; // 0x2c
        bool isHoldGun; // 0x2d
        uint8_t pad2[2]; // 0x2e
    };
    static_assert(sizeof(kPoseParam) == 0x30);
    struct kCameraAngle {
        float now; // 0x0
        float raw; // 0x4
        bool isHold; // 0x8
        char pad_9[0x3];
        float holdAng; // 0xc
    };
    static_assert(sizeof(kCameraAngle) == 0x10);
    struct cRotateCtrl {
        void* vtable_ptr; // 0x0
        bool mIsMove; // 0x4
        char pad_5[0x3];
        float mTarget; // 0x8
        float mNow; // 0xc
        float mMax; // 0x10
    };
    static_assert(sizeof(cRotateCtrl) == 0x14);
    struct kPushCollInfo {
        void*  /* cCollision * */ pColl; // 0x0
        char pad_4[0xc];
        MtVector3 hitPos; // 0x10
        MtVector3 targetPos; // 0x20
    };
    static_assert(sizeof(kPushCollInfo) == 0x30);
    struct cLegIkCtrl {
        char pad_0[0x4];
        bool mIsEnable; // 0x4
        char pad_5[0x3];
        uPlayer* mpParent; // 0x8
        int32_t mId; // 0xc
        char pad_10[0x1f0];
        bool mLand; // 0x200
        char pad_201[0x3];
        uint32_t mJointNo[3]; // 0x204
    };
    static_assert(sizeof(cLegIkCtrl) == 0x210);
    struct cFootprintCtrl {
        char pad_0[0x4];
        bool mIsEnable; // 0x4
        char pad_5[0x3];
        uModel* mpParent; // 0x8
        int32_t mJointNo; // 0xc
        float mOffsetY; // 0x10
        uint32_t mScrAttr; // 0x14
        int32_t mType; // 0x18
        bool mLand; // 0x1c
        char pad_1d[0x3];
    };
    static_assert(sizeof(cFootprintCtrl) == 0x20);
    struct cSECtrl {
        char pad_0[0x17c];
    };
    static_assert(sizeof(cSECtrl) == 0x17c);
    struct kMotContInfo {
        uint8_t atck; // 0x0
        uint8_t camVib; // 0x1
        uint8_t free0; // 0x2
        uint8_t free1; // 0x3
        uint8_t free2; // 0x4
        uint8_t padVib; // 0x5
    };
    static_assert(sizeof(kMotContInfo) == 0x6);
    struct cWeaponCtrl {
        char pad_0[0x4];
        uPlayer* mpParent; // 0x4
        void*  /* uPlWeapon * */ mpWeapon[7]; // 0x8
        uintptr_t mpShortChgTbl; // 0x24
        uintptr_t mpLongChgTbl; // 0x28
        uint32_t mShortId; // 0x2c
        uint32_t mLongId; // 0x30
        uint32_t mGripId; // 0x34
        uint32_t mEnableShortId; // 0x38
        uint32_t mEnableLongId; // 0x3c
        bool mIsChangeEnable; // 0x40
        bool mIsChangedShort; // 0x41
        bool mIsChangedLong; // 0x42
        char pad_43[0x1];
    };
    static_assert(sizeof(cWeaponCtrl) == 0x44);
    struct cCharTblMgr {
        uintptr_t member_0x0; // 0x0
        uintptr_t mpRsrc; // 0x4
        void*  /* uModel::uModel::Motion * */ mpMtn; // 0x8
        void*  /* uchar * */ mpDataTop; // 0xc
        void*  /* uchar * */ mpDataCrnt; // 0x10
        bool mIsDone; // 0x14
        uint8_t info[12]; // 0x15
        char pad_21[0x3];
    };
    static_assert(sizeof(cCharTblMgr) == 0x24);
    struct cParamTblCtrl {
        char pad_0[0x4];
        cResource* mpRsrc; // 0x4
        void*  /* float * */ mpData; // 0x8
    };
    static_assert(sizeof(cParamTblCtrl) == 0xc);
    struct kWindParam {
        float blend; // 0x0
        float gravity; // 0x4
        char pad_8[0x8];
        MtVector3 wind; // 0x10
    };
    static_assert(sizeof(kWindParam) == 0x20);
    struct kDevilTrg {
        float now; // 0x0
        float max; // 0x4
        bool isOnNow; // 0x8
        bool isOnOld; // 0x9
        char pad_a[0x2];
        float keepTimer; // 0xc
    };
    static_assert(sizeof(kDevilTrg) == 0x10);
    struct auto_structs__MtPlane {
        MtFloat3 normal; // 0x0
        float dist; // 0xc
    };
    static_assert(sizeof(auto_structs__MtPlane) == 0x10);
    struct auto_structs__uknCollisionInfo {
        uint32_t pSbc; // 0x0
        int32_t PartsNum; // 0x4
        uint32_t pPartsInfo; // 0x8
        void*  /* rCollision::Triangle * */ pTriangle; // 0xc
        void*  /* rCollision::rCollision::Vertex * */ pVertex; // 0x10
        char pad_14[0xc];
        MtVector3 member_0x20; // 0x20
        MtVector3 member_0x30; // 0x30
        MtVector3 member_0x40; // 0x40
        auto_structs__MtPlane hit_plane; // 0x50
        MtVector3 hit_pos; // 0x60
        MtVector3 hit_normal; // 0x70
        float member_0x80; // 0x80
        char pad_84[0xc];
    };
    static_assert(sizeof(auto_structs__uknCollisionInfo) == 0x90);
    struct kConstInfo {
        uActor* mpParent; // 0x0
        int32_t mJoint; // 0x4
        bool mIsAngReverse; // 0x8
        char pad_9[0x3];
        int32_t mType; // 0xc
    };
    static_assert(sizeof(kConstInfo) == 0x10);
    struct cConstraint {
        uintptr_t vtable; // 0x0
        float mBlendWeight; // 0x4
        uint8_t mRno[4]; // 0x8
        float mBlendVel; // 0xc
        float mBlend; // 0x10
        float mRate; // 0x14
        bool mIgnoreRate; // 0x18
        uint8_t mEnable; // 0x19
        char pad_1a[0x2];
        uint32_t mID; // 0x1c
    };
    static_assert(sizeof(cConstraint) == 0x20);
    struct cCnsRandom {
        cConstraint member_0x0; // 0x0
        float mTimerT; // 0x20
        float mAmpTX; // 0x24
        float mAmpTY; // 0x28
        float mAmpTZ; // 0x2c
        float mDecayT; // 0x30
        float mTimerR; // 0x34
        float mAmpR; // 0x38
        float mDecayR; // 0x3c
        uint32_t mMode; // 0x40
    };
    static_assert(sizeof(cCnsRandom) == 0x44);
    struct kSPlCmd {
        uint8_t mStep; // 0x0
        uint8_t mStatus; // 0x1
        float mActiveF; // 0x4
        float mTimerF; // 0x8
        uint8_t mCnt; // 0xc
        uint8_t mCmdFlag; // 0xd
        float mTame; // 0x10
    };
    static_assert(sizeof(kSPlCmd) == 0x14);
    struct kDashParam {
        float timer; // 0x0
        bool isDash; // 0x4
        bool isEfctSet; // 0x5
        char pad_6[0x2];
        float angBuff[20]; // 0x8
        int32_t angBuffCnt; // 0x58
        int32_t sePhase; // 0x5c
    };
    static_assert(sizeof(kDashParam) == 0x60);
    struct cMotionCtrl {
        uintptr_t vtable; // 0x0
        uint8_t mIsMotionSetReq; // 0x4
        uint8_t mIsBlendRateSetReq; // 0x5
        uint8_t mIsSppedSetReq; // 0x6
        char pad_7[0x1];
        uActor* mpParent; // 0x8
        MOT_TYPE mType; // 0xc
        void*  /* uModel::uModel::Motion * */ mpMotion; // 0x10
        uint32_t mMotionNo; // 0x14
        uint32_t mMotionNoOld; // 0x18
        float mInterpolateTime; // 0x1c
        float mStartFrame; // 0x20
        float mSpeed; // 0x24
        int16_t mAttribute; // 0x28
        int16_t mAddAttribute; // 0x2a
        int16_t mSubAttribute; // 0x2c
        char pad_2e[0x2];
        float mBlendRate; // 0x30
        uint8_t mIsHitStop; // 0x34
        uint8_t mIsInterpolate; // 0x35
        char pad_36[0x2];
        float mInterpolateCnt; // 0x38
    };
    static_assert(sizeof(cMotionCtrl) == 0x3c);
    struct kMotCharTblSetParam {
        bool isEnable; // 0x0
        char pad_1[0x3];
        uint32_t bank; // 0x4
        uint32_t idx; // 0x8
    };
    static_assert(sizeof(kMotCharTblSetParam) == 0xc);
    struct kEfctCtrl {
        uintptr_t ptr; // 0x0
        EFCT_TYPE type; // 0x4
        int32_t id; // 0x8
    };
    static_assert(sizeof(kEfctCtrl) == 0xc);
    struct kLockOnParam {
        bool chgEnable; // 0x0
        char pad_1[0x3];
        float keepTimer; // 0x4
        bool isLockOn; // 0x8
        char pad_9[0x3];
        uint32_t mLockOnMode; // 0xc
        void*  /* cLockonTarget * */ pTarget; // 0x10
        uActor* pParent; // 0x14
        void*  /* cLockonTarget * */ pInterTarget; // 0x18
        uActor* pInterParent; // 0x1c
        void*  /* cLockonTarget * */ pInterLongTarget; // 0x20
        uActor* pInterLongParent; // 0x24
        char pad_28[0x8];
        MtVector4 quat; // 0x30
        MtVector3 targetPos; // 0x40
    };
    static_assert(sizeof(kLockOnParam) == 0x50);
    struct cMotCancel {
        char pad_0[0x4];
        int32_t mStat; // 0x4
        bool mIsReserve; // 0x8
        char pad_9[0x1b];
        uPlayer* mpParent; // 0x24
        uintptr_t mpCheckFunc; // 0x28
    };
    static_assert(sizeof(cMotCancel) == 0x2c);
    struct kAutomaticCombo {
        int32_t mPhase; // 0x0
        int32_t mArea; // 0x4
        bool mIsJump; // 0x8
        char pad_9[0x3];
        int32_t mWpnId; // 0xc
        uintptr_t mpAtMeleeTbl; // 0x10
    };
    static_assert(sizeof(kAutomaticCombo) == 0x14);
    struct kShotRepeatParam {
        int32_t atckId; // 0x0
        float interval; // 0x4
        float timer; // 0x8
        int32_t shotCnt; // 0xc
        int32_t repeatCnt; // 0x10
    };
    static_assert(sizeof(kShotRepeatParam) == 0x14);
    struct kShotChargeParam {
        float chargeFrame[3]; // 0x0
        float maxChargeFrm; // 0xc
        float timer; // 0x10
        float noCountTimer; // 0x14
    };
    static_assert(sizeof(kShotChargeParam) == 0x18);

    cPeripheral mPeripheral; // 0x1370
    cInterface mInterface; // 0x1408
    cProcSpdCtrl mProcSpdCtrl; // 0x1454
    cHitSlowCtrl mHitSlowCtrl; // 0x1484
    uint32_t mPlayerID; // 0x1494
    char pad_1498[0x8];
    MtVector3 mPosOld; // 0x14a0
    MtMatrix mCorrectMtrx; // 0x14b0
    int32_t mActionNo; // 0x14f0
    int32_t mActionNoOld; // 0x14f4
    kBdyAction mBdyAction[2]; // 0x14f8
    bool mIsRotateUpdate; // 0x1508
    bool mIsInterfaceSetting; // 0x1509
    bool mIsEnableExceptionInput; // 0x150a
    bool mIsInvincible; // 0x150b
    bool mEnableScrHit; // 0x150c
    bool mEnableScrHitForce; // 0x150d
    bool mEnableObjHit; // 0x150e
    bool mEnableObjHitForce; // 0x150f
    bool mIsCatchCMsgOnCatapult; // 0x1510
    bool mIsCathcCMsgEventStingerJump; // 0x1511
    bool mIsCatchCMsgEmSync; // 0x1512
    char pad_1513[0xd];
    kStingerJumpParam mSJumpParam; // 0x1520
    uint32_t ActLimit; // 0x1550
    ACT_STAT mActStat; // 0x1554
    char pad_1558[0x4];
    void*  /* uPlayer::kActionTbl * */ mpActionTbl[2]; // 0x155c
    int32_t mAtckId; // 0x1564
    int32_t mOldAtckId; // 0x1568
    kStyleParam mStyle; // 0x156c
    kDamageParam mDamage; // 0x157c
    char pad_16a8[0x4];
    RUN_MODE mRunMode; // 0x16ac
    kJumpParam mJump; // 0x16b0
    kPoseParam mPose; // 0x16c4
    kCameraAngle mCamAngl; // 0x16f4
    cRotateCtrl mRotate; // 0x1704
    cRotateCtrl mPoseAngle; // 0x1718
    char pad_172c[0x4];
    kPushCollInfo mPshCollInfo; // 0x1730
    int32_t mYoyakuAtckId; // 0x1760
    bool mActFlag[4]; // 0x1764
    bool mActFlagU[4]; // 0x1768
    int16_t mActCnt[4]; // 0x176c
    int16_t mActCntU[4]; // 0x1774
    float mActTimer[4]; // 0x177c
    float mActTimerU[4]; // 0x178c
    float mSamePushTimer; // 0x179c
    cLegIkCtrl mLegIk[2]; // 0x17a0
    cFootprintCtrl mFootPrnt[2]; // 0x1bc0
    cSECtrl mSeCtrl; // 0x1c00
    kMotContInfo mMotCont; // 0x1d7c
    kMotContInfo mMotContOld; // 0x1d82
    cWeaponCtrl mWeaponCtrl; // 0x1d88
    char pad_1dcc[0x8];
    cCharTblMgr mCharTblMgr[2]; // 0x1dd4
    float mMoveVel; // 0x1e1c
    float mMoveAcc; // 0x1e20
    float mMoveAngle; // 0x1e24
    char pad_1e28[0x8];
    MtVector3 mAddSpd; // 0x1e30
    bool mIsAddSpeed; // 0x1e40
    char pad_1e41[0xf];
    MtVector3 mMoveVector; // 0x1e50
    float mMovementXZ; // 0x1e60
    float mMovementAngY; // 0x1e64
    float mHitStopTimer; // 0x1e68
    float mDevilTrgTimer; // 0x1e6c
    float mWeaponChangeTimer[2]; // 0x1e70
    bool mWeaponChangeRsrvReverse[2]; // 0x1e78
    uint8_t mAtckLongCnt; // 0x1e7a
    uint8_t mAtckShortCnt; // 0x1e7b
    uint8_t mAtckStylCnt; // 0x1e7c
    int8_t mJumpAtckCnt; // 0x1e7d
    char pad_1e7e[0x4];
    bool mIsDead; // 0x1e82
    char pad_1e83[0x5];
    bool mIsCheckSerch; // 0x1e88
    bool mIsNoCliff; // 0x1e89
    char pad_1e8a[0x2];
    uCollisionMgr* mCollMgr; // 0x1e8c
    float mMotionBlendWeight; // 0x1e90
    cParamTblCtrl mParamTbl[2]; // 0x1e94
    cParamTblCtrl mClothParam; // 0x1eac
    cParamTblCtrl mPadVibParam; // 0x1eb8
    char pad_1ec4[0xc];
    kWindParam mWindParam[2]; // 0x1ed0
    float mRendaTimer[4]; // 0x1f10
    uint8_t mRendaLevel; // 0x1f20
    uint8_t mRendaTblNo; // 0x1f21
    char pad_1f22[0x2];
    kDevilTrg mDevilTrg; // 0x1f24
    float mProvokeTimer; // 0x1f34
    char pad_1f38[0x8];
    MtVector3 mUpVec; // 0x1f40
    bool mIsCancelCehckEnable; // 0x1f50
    char pad_1f51[0x3];
    void*  /* cLockonTarget * */ mpAtckTarget; // 0x1f54
    char pad_1f58[0x8];
    auto_structs__uknCollisionInfo mGroundPoly; // 0x1f60
    bool isWallFront; // 0x1ff0
    char pad_1ff1[0x3];
    float mEmJumpTimer; // 0x1ff4
    float mWJumpTimer; // 0x1ff8
    uActor* mpLockTargetParent; // 0x1ffc
    uintptr_t mpLockTarget; // 0x2000
    bool mIsDevilTriggerTrg; // 0x2004
    bool mIsEmJumpEnable; // 0x2005
    char pad_2006[0x2];
    uint32_t mScrClsnRslt; // 0x2008
    char pad_200c[0x4];
    auto_structs__uknCollisionInfo mScrClsnTriInfo; // 0x2010
    bool mUseHolyWaterReq; // 0x20a0
    char pad_20a1[0x3];
    float mHolyTimer; // 0x20a4
    float mReviveTimer; // 0x20a8
    float mFallHeight; // 0x20ac
    bool mCheckIK; // 0x20b0
    char pad_20b1[0x3];
    float mFrameCntr; // 0x20b4
    bool mIsExceptCalcPos; // 0x20b8
    char pad_20b9[0x7];
    uDevil4Model* mpFaceMdl; // 0x20c0
    uDevil4Model* mpCoatMdl; // 0x20c4
    bool mIsFlipCancel; // 0x20c8
    bool mIsWpChangeCancel; // 0x20c9
    bool mIsGrabbedEnemy; // 0x20ca
    char pad_20cb[0x1];
    kConstInfo mPosConstInfo; // 0x20cc
    cCnsRandom mCnsRandomUp; // 0x20dc
    cCnsRandom mCnsRandomDown; // 0x2120
    float mDownMaxSpd; // 0x2164
    bool mIsSetupFrame; // 0x2168
    bool mIsFootWorkMtnKeep; // 0x2169
    bool mIsEnableAutoDT; // 0x216a
    bool mIsReviveReq; // 0x216b
    char pad_216c[0x4];
    MtVector3 mLastLandPos; // 0x2170
    bool mIsSetLandEfct; // 0x2180
    bool mIsEnableEnemyPsh; // 0x2181
    char pad_2182[0xe];
    kSPlCmd mPlCmd[85]; // 0x2190
    kDashParam mDash; // 0x2834
    cMotionCtrl mMotionCtrl[7]; // 0x2894
    bool mMotionSetClsnNoClear; // 0x2a38
    char pad_2a39[0x3];
    kMotCharTblSetParam mMotionSetCharTblParam[2]; // 0x2a3c
    bool mMotionFrameEnd[7]; // 0x2a54
    char pad_2a5b[0x1];
    kEfctCtrl mEfctCtrl[128]; // 0x2a5c
    int32_t mEfctControlLevel; // 0x305c
    LAND_EFCT_TYPE mLandEfctType; // 0x3060
    char pad_3064[0xc];
    kLockOnParam mLockOn; // 0x3070
    cMotCancel mMotCncl[8]; // 0x30c0
    kAutomaticCombo mAutoCombo; // 0x3220
    bool mIsAutomaticEnable; // 0x3234
    bool mIsAutoCombo; // 0x3235
    bool mIsEnableAtckForceCancel; // 0x3236
    bool mIsEmDanteReverse; // 0x3237
    kShotRepeatParam mShotRepeatParam; // 0x3238
    kShotChargeParam mShotChargeParam; // 0x324c
    char pad_3264[0xc];
};
static_assert(sizeof(uPlayer) == 0x3270);
static_assert(offsetof(uPlayer, mPeripheral) == 0x1370);
static_assert(offsetof(uPlayer, mInterface) == 0x1408);
static_assert(offsetof(uPlayer, mProcSpdCtrl) == 0x1454);
static_assert(offsetof(uPlayer, mHitSlowCtrl) == 0x1484);
static_assert(offsetof(uPlayer, mPlayerID) == 0x1494);
static_assert(offsetof(uPlayer, mPosOld) == 0x14a0);
static_assert(offsetof(uPlayer, mCorrectMtrx) == 0x14b0);
static_assert(offsetof(uPlayer, mActionNo) == 0x14f0);
static_assert(offsetof(uPlayer, mActionNoOld) == 0x14f4);
static_assert(offsetof(uPlayer, mBdyAction) == 0x14f8);
static_assert(offsetof(uPlayer, mIsRotateUpdate) == 0x1508);
static_assert(offsetof(uPlayer, mIsInterfaceSetting) == 0x1509);
static_assert(offsetof(uPlayer, mIsEnableExceptionInput) == 0x150a);
static_assert(offsetof(uPlayer, mIsInvincible) == 0x150b);
static_assert(offsetof(uPlayer, mEnableScrHit) == 0x150c);
static_assert(offsetof(uPlayer, mEnableScrHitForce) == 0x150d);
static_assert(offsetof(uPlayer, mEnableObjHit) == 0x150e);
static_assert(offsetof(uPlayer, mEnableObjHitForce) == 0x150f);
static_assert(offsetof(uPlayer, mIsCatchCMsgOnCatapult) == 0x1510);
static_assert(offsetof(uPlayer, mIsCathcCMsgEventStingerJump) == 0x1511);
static_assert(offsetof(uPlayer, mIsCatchCMsgEmSync) == 0x1512);
static_assert(offsetof(uPlayer, mSJumpParam) == 0x1520);
static_assert(offsetof(uPlayer, ActLimit) == 0x1550);
static_assert(offsetof(uPlayer, mActStat) == 0x1554);
static_assert(offsetof(uPlayer, mpActionTbl) == 0x155c);
static_assert(offsetof(uPlayer, mAtckId) == 0x1564);
static_assert(offsetof(uPlayer, mOldAtckId) == 0x1568);
static_assert(offsetof(uPlayer, mStyle) == 0x156c);
static_assert(offsetof(uPlayer, mDamage) == 0x157c);
static_assert(offsetof(uPlayer, mRunMode) == 0x16ac);
static_assert(offsetof(uPlayer, mJump) == 0x16b0);
static_assert(offsetof(uPlayer, mPose) == 0x16c4);
static_assert(offsetof(uPlayer, mCamAngl) == 0x16f4);
static_assert(offsetof(uPlayer, mRotate) == 0x1704);
static_assert(offsetof(uPlayer, mPoseAngle) == 0x1718);
static_assert(offsetof(uPlayer, mPshCollInfo) == 0x1730);
static_assert(offsetof(uPlayer, mYoyakuAtckId) == 0x1760);
static_assert(offsetof(uPlayer, mActFlag) == 0x1764);
static_assert(offsetof(uPlayer, mActFlagU) == 0x1768);
static_assert(offsetof(uPlayer, mActCnt) == 0x176c);
static_assert(offsetof(uPlayer, mActCntU) == 0x1774);
static_assert(offsetof(uPlayer, mActTimer) == 0x177c);
static_assert(offsetof(uPlayer, mActTimerU) == 0x178c);
static_assert(offsetof(uPlayer, mSamePushTimer) == 0x179c);
static_assert(offsetof(uPlayer, mLegIk) == 0x17a0);
static_assert(offsetof(uPlayer, mFootPrnt) == 0x1bc0);
static_assert(offsetof(uPlayer, mSeCtrl) == 0x1c00);
static_assert(offsetof(uPlayer, mMotCont) == 0x1d7c);
static_assert(offsetof(uPlayer, mMotContOld) == 0x1d82);
static_assert(offsetof(uPlayer, mWeaponCtrl) == 0x1d88);
static_assert(offsetof(uPlayer, mCharTblMgr) == 0x1dd4);
static_assert(offsetof(uPlayer, mMoveVel) == 0x1e1c);
static_assert(offsetof(uPlayer, mMoveAcc) == 0x1e20);
static_assert(offsetof(uPlayer, mMoveAngle) == 0x1e24);
static_assert(offsetof(uPlayer, mAddSpd) == 0x1e30);
static_assert(offsetof(uPlayer, mIsAddSpeed) == 0x1e40);
static_assert(offsetof(uPlayer, mMoveVector) == 0x1e50);
static_assert(offsetof(uPlayer, mMovementXZ) == 0x1e60);
static_assert(offsetof(uPlayer, mMovementAngY) == 0x1e64);
static_assert(offsetof(uPlayer, mHitStopTimer) == 0x1e68);
static_assert(offsetof(uPlayer, mDevilTrgTimer) == 0x1e6c);
static_assert(offsetof(uPlayer, mWeaponChangeTimer) == 0x1e70);
static_assert(offsetof(uPlayer, mWeaponChangeRsrvReverse) == 0x1e78);
static_assert(offsetof(uPlayer, mAtckLongCnt) == 0x1e7a);
static_assert(offsetof(uPlayer, mAtckShortCnt) == 0x1e7b);
static_assert(offsetof(uPlayer, mAtckStylCnt) == 0x1e7c);
static_assert(offsetof(uPlayer, mJumpAtckCnt) == 0x1e7d);
static_assert(offsetof(uPlayer, mIsDead) == 0x1e82);
static_assert(offsetof(uPlayer, mIsCheckSerch) == 0x1e88);
static_assert(offsetof(uPlayer, mIsNoCliff) == 0x1e89);
static_assert(offsetof(uPlayer, mCollMgr) == 0x1e8c);
static_assert(offsetof(uPlayer, mMotionBlendWeight) == 0x1e90);
static_assert(offsetof(uPlayer, mParamTbl) == 0x1e94);
static_assert(offsetof(uPlayer, mClothParam) == 0x1eac);
static_assert(offsetof(uPlayer, mPadVibParam) == 0x1eb8);
static_assert(offsetof(uPlayer, mWindParam) == 0x1ed0);
static_assert(offsetof(uPlayer, mRendaTimer) == 0x1f10);
static_assert(offsetof(uPlayer, mRendaLevel) == 0x1f20);
static_assert(offsetof(uPlayer, mRendaTblNo) == 0x1f21);
static_assert(offsetof(uPlayer, mDevilTrg) == 0x1f24);
static_assert(offsetof(uPlayer, mProvokeTimer) == 0x1f34);
static_assert(offsetof(uPlayer, mUpVec) == 0x1f40);
static_assert(offsetof(uPlayer, mIsCancelCehckEnable) == 0x1f50);
static_assert(offsetof(uPlayer, mpAtckTarget) == 0x1f54);
static_assert(offsetof(uPlayer, mGroundPoly) == 0x1f60);
static_assert(offsetof(uPlayer, isWallFront) == 0x1ff0);
static_assert(offsetof(uPlayer, mEmJumpTimer) == 0x1ff4);
static_assert(offsetof(uPlayer, mWJumpTimer) == 0x1ff8);
static_assert(offsetof(uPlayer, mpLockTargetParent) == 0x1ffc);
static_assert(offsetof(uPlayer, mpLockTarget) == 0x2000);
static_assert(offsetof(uPlayer, mIsDevilTriggerTrg) == 0x2004);
static_assert(offsetof(uPlayer, mIsEmJumpEnable) == 0x2005);
static_assert(offsetof(uPlayer, mScrClsnRslt) == 0x2008);
static_assert(offsetof(uPlayer, mScrClsnTriInfo) == 0x2010);
static_assert(offsetof(uPlayer, mUseHolyWaterReq) == 0x20a0);
static_assert(offsetof(uPlayer, mHolyTimer) == 0x20a4);
static_assert(offsetof(uPlayer, mReviveTimer) == 0x20a8);
static_assert(offsetof(uPlayer, mFallHeight) == 0x20ac);
static_assert(offsetof(uPlayer, mCheckIK) == 0x20b0);
static_assert(offsetof(uPlayer, mFrameCntr) == 0x20b4);
static_assert(offsetof(uPlayer, mIsExceptCalcPos) == 0x20b8);
static_assert(offsetof(uPlayer, mpFaceMdl) == 0x20c0);
static_assert(offsetof(uPlayer, mpCoatMdl) == 0x20c4);
static_assert(offsetof(uPlayer, mIsFlipCancel) == 0x20c8);
static_assert(offsetof(uPlayer, mIsWpChangeCancel) == 0x20c9);
static_assert(offsetof(uPlayer, mIsGrabbedEnemy) == 0x20ca);
static_assert(offsetof(uPlayer, mPosConstInfo) == 0x20cc);
static_assert(offsetof(uPlayer, mCnsRandomUp) == 0x20dc);
static_assert(offsetof(uPlayer, mCnsRandomDown) == 0x2120);
static_assert(offsetof(uPlayer, mDownMaxSpd) == 0x2164);
static_assert(offsetof(uPlayer, mIsSetupFrame) == 0x2168);
static_assert(offsetof(uPlayer, mIsFootWorkMtnKeep) == 0x2169);
static_assert(offsetof(uPlayer, mIsEnableAutoDT) == 0x216a);
static_assert(offsetof(uPlayer, mIsReviveReq) == 0x216b);
static_assert(offsetof(uPlayer, mLastLandPos) == 0x2170);
static_assert(offsetof(uPlayer, mIsSetLandEfct) == 0x2180);
static_assert(offsetof(uPlayer, mIsEnableEnemyPsh) == 0x2181);
static_assert(offsetof(uPlayer, mPlCmd) == 0x2190);
static_assert(offsetof(uPlayer, mDash) == 0x2834);
static_assert(offsetof(uPlayer, mMotionCtrl) == 0x2894);
static_assert(offsetof(uPlayer, mMotionSetClsnNoClear) == 0x2a38);
static_assert(offsetof(uPlayer, mMotionSetCharTblParam) == 0x2a3c);
static_assert(offsetof(uPlayer, mMotionFrameEnd) == 0x2a54);
static_assert(offsetof(uPlayer, mEfctCtrl) == 0x2a5c);
static_assert(offsetof(uPlayer, mEfctControlLevel) == 0x305c);
static_assert(offsetof(uPlayer, mLandEfctType) == 0x3060);
static_assert(offsetof(uPlayer, mLockOn) == 0x3070);
static_assert(offsetof(uPlayer, mMotCncl) == 0x30c0);
static_assert(offsetof(uPlayer, mAutoCombo) == 0x3220);
static_assert(offsetof(uPlayer, mIsAutomaticEnable) == 0x3234);
static_assert(offsetof(uPlayer, mIsAutoCombo) == 0x3235);
static_assert(offsetof(uPlayer, mIsEnableAtckForceCancel) == 0x3236);
static_assert(offsetof(uPlayer, mIsEmDanteReverse) == 0x3237);
static_assert(offsetof(uPlayer, mShotRepeatParam) == 0x3238);
static_assert(offsetof(uPlayer, mShotChargeParam) == 0x324c);
