#ifndef UCOLLISIONMGR_HPP
#define UCOLLISIONMGR_HPP



#include "MtMath.hpp"
#include "uActor.hpp"
#include "uDamage.hpp"

typedef unsigned int uint;
struct cCollisionGroup;
class uCollisionMgr;

struct kAttackStatus {
    char mAsName[16];
    int mAttackID; /* Created by retype action */
    float mDamageValue;
    int mAttackLv;
    int mAttackLvI;
    int mAttackLvB;
    int mRangeType;
    int mHitStopTimer;
    int mDamageType;
    int mDamageTypeI;
    int mDamageTypeB;
    int mHitmarkAngle;
    int mHitSE;
    float mStylishPoint;
    float mStylishTimer;
    float mDTAdd;
    int mAttackFlag;
    int mBlownAngleType;
    int mElementType;
};

struct kDefendStatus {
    char mAsName[16];
    float mResist[3];
    short mMaxInterrupt[5];
    short mMaxBlown[5];
};

enum cCollPrimType: unsigned int {
    PRIM_TYPE_SPHERE   = 0,
    PRIM_TYPE_BOX      = 1,
    PRIM_TYPE_CYLINDER = 2,
    PRIM_TYPE_CAPSULE  = 3,
    MAX_PRIM_TYPE      = 4
};

struct kCollPrim {
    cCollPrimType mType; // 0x0
    int mBone0; // 0x4
    int mBone1; // 0x8
    float mRadius; // 0xC
    MtVector3 mPos0; // 0x10
    MtVector3 mPos1; // 0x20
    uint mAttr; // 0x30
    uint mId; // 0x34
    float mShrink; // 0x38
    int mDummy; // 0x3C
};
static_assert(sizeof(kCollPrim) == 0x40);

struct cCollision {
    bool mHit; // 0x0
    char pad_01[0x3]; // 0x1
    kCollPrim* mpCollPrim; // 0x4
    float mScale; // 0x8
    int paddingC; // 0xC
    MtAABB mAABB; // 0x10
    bool mAABBHit; // 0x30
    char pad_0x31[3]; // 0x31
    cCollisionGroup* mpCollisionGroup; // 0x34
    char padding38[8]; // 0x38
    MtMatrix mMat0; // 0x40
    MtMatrix mMat1; // 0x80
    MtMatrix mOldMat0; // 0xC0
    MtMatrix* mpRefMat0; // 0x100
    MtMatrix* mpRefMat1; // 0x104
    bool mUseOld; // 0x108
    char padding109[7]; // 0x109
};
static_assert(sizeof(cCollision) == 0x110);

struct cCollisionGroup {
    uint mSeqNo; // 0x0
    uint mKind; // 0x4
    uint mAttr; // 0x8
    uint mVsAttr; // 0xC
    bool mRide; // 0x10
    bool mFrdAtck; // 0x11
    char padding12[0xC]; // 0x12
    MtVector3 mHitPos; // 0x20
    MtVector3 mShrinkHitPos; // 0x30
    cCollision* mpHitCollision; // 0x40
    char padding44[0xC]; // 0x44
    MtVector3 mPushPos; // 0x50
    cCollision* mpPushCollision; // 0x60
    bool mNoThrough; // 0x64
    int mCreatedFromIdx;
    bool mClear;
    MtAABB mAABB;
    bool mAABBHit;
    int mNum;
    uCollisionMgr* mpCollisionMgr;
    cCollision** mppCollision;
    uintptr_t mpCollGroup;
    kAttackStatus mAttackStatus; // 0xa4?
    kAttackStatus* mpAttackStatusSrc;
    kDefendStatus mDefendStatus;
    cCollisionGroup* mpNext[4];
    MtMatrix* mpRefMat;
    float mTimer;
    float mOffset;
    float mLife;
};

enum PUSH_POS_MODE: unsigned int {
    PUSH_POS_BOTTOM  = 0,
    PUSH_POS_SURFACE = 1,
    PUSH_POS_P0      = 2,
    PUSH_POS_P1      = 3
};

enum PUSH_MODE : unsigned int {
    CHARACTER_MODE,
    DIRECT_MODE
};

enum COLLISION_MGR_ID {
    COLLMGR_ID_DEFAULT=0,
    COLLMGR_ID_EM021=1
};


class uCollisionMgr: public cUnit {
public:
    bool mEnable;
    bool mDamage;
    bool mPsh;
    bool mCapsuleLand;
    bool mLand;
    bool mNoLand;
    bool mMoveLand;
    bool mWall;
    bool mCeilling;
    bool mHitCap;
    bool mMoveWall;
    bool mNoLandTimerEnable;
    float mNoLandTimer;
    float mNoLandTime;
    bool mObjLand;
    bool mOldLand;
    bool mOldMoveLand;
    bool mOldWall;
    bool mOldCeilling;
    bool mCheckWallEnable;
    bool mCheckGroundEnable;
    char padding33[9];
    bool mFall;
    bool mIsBoss;
    bool mIsDevilTrigger;
    uint8_t field26_0x3f;
    uint mDevilTriggerMode;
    float mDamageValueCorrect;
    bool mPushStraight;
    undefined field30_0x49;
    undefined field31_0x4a;
    undefined field32_0x4b;
    uint padding4c;
    MtVector3 mPushStraightDir;
    cLockonTarget* mpPushStraightLockOn;
    uint padding64[3];
    MtVector3 mCorrectPos;
    MtMatrix mCorrectMat;
    MtVector3 mAttackCorrectPos;
    uint mWeightType;
    uint mModelID;
    uActor* mpReportActor;
    uModel* mpDstModel; /* Created by retype action */
    uModel* mpSrcModel;
    uintptr_t mpModel;
    uModel* mppIgnoreModel[16];
    int mIgnoreModelNum;
    uint mPushType;
    uintptr_t mpPushModel;
    uint padding134[3];
    MtCapsule mPushCap;
    PUSH_POS_MODE mPushPosMode;
    float mPushCapShrinkTimer;
    undefined field54_0x178;
    undefined field55_0x179;
    undefined field56_0x17a;
    undefined field57_0x17b;
    undefined field58_0x17c;
    undefined field59_0x17d;
    undefined field60_0x17e;
    undefined field61_0x17f;
    MtLineSegment mPushLineSgXZ;
    MtLineSegment mPushLineSgY;
    MtMatrix mPushNewMat;
    MtMatrix mPushOldMat;
    MtMatrix mPushNewMatThrough;
    bool mPushUseOld; /* Created by retype action */
    uintptr_t mpCollisionIdxData;
    uintptr_t mpCollisionShape;
    uintptr_t mpAttackStatusData;
    uintptr_t mpDefendStatusData;
    bool mInDamageMessage;
    uint mMotType;
    uint mSeqTrgOn;
    uint mSeqTrgOff;
    bool mClearReq;
    bool mClear;
    bool mPushMode;
    COLLISION_MGR_ID mCollisionMgrID;
    int mUniqueID;
    uint mVsAttrPlAtk; /* Created by retype action */
    uint mVsAttrPlDmg;
    uint mVsAttrPlPsh;
    uint mVsAttrPlGrb; /* Created by retype action */
    uint mVsAttrPlFrdAtk;
    uint mVsAttrPsAtk; /* Created by retype action */
    uint mVsAttrPsDmg;
    uint mVsAttrPsPsh;
    uint mVsAttrPsGrb; /* Created by retype action */
    uint mVsAttrPsFrdAtk;
    uint mVsAttrEmAtk; /* Created by retype action */
    uint mVsAttrEmDmg;
    uint mVsAttrEmPsh;
    uint mVsAttrEmGrb;    /* Created by retype action */
    uint mVsAttrEmFrdAtk; /* Created by retype action */
    uint mVsAttrEsAtk;    /* Created by retype action */
    uint mVsAttrEsDmg;
    uint mVsAttrEsPsh;
    uint mVsAttrEsGrb;    /* Created by retype action */
    uint mVsAttrEsFrdAtk; /* Created by retype action */
    uint mVsAttrEm2Atk;   /* Created by retype action */
    uint mVsAttrEm2Dmg;
    uint mVsAttrEm2Psh;
    uint mVsAttrEm2Grb;
    uint mVsAttrEm2FrdAtk; /* Created by retype action */
    uint mVsAttrEs2Atk;    /* Created by retype action */
    uint mVsAttrEs2Dmg;
    uint mVsAttrEs2Psh;
    uint mVsAttrEs2Grb;    /* Created by retype action */
    uint mVsAttrEs2FrdAtk; /* Created by retype action */
    uint mVsAttrSetAtk;    /* Created by retype action */
    uint mVsAttrSetDmg;
    uint mVsAttrSetPsh;
    uint mVsAttrSetGrb;    /* Created by retype action */
    uint mVsAttrSetFrdAtk; /* Created by retype action */
    uint mVsAttrStgAtk;    /* Created by retype action */
    uint mVsAttrStgDmg;
    uint mVsAttrStgPsh; /* Created by retype action */
    uint mVsAttrStgGrb; /* Created by retype action */
    bool mIsCheckPush;
    bool mIsStingerStop;  /* Created by retype action */
    bool mIsLuciferStick; /* Created by retype action */
    PUSH_MODE mMode;
    uint mCollisionGroupNum;
    cCollisionGroup* mppCollisionGroup[32];
    uint padding3d8[6];
};
static_assert(sizeof(uCollisionMgr) == 0x3f0);

enum UDAMAGE_TYPE {
    PLAYER      = 0,
    SHELL       = 1,
    ENEMY       = 2,
    ENEMY_SHELL = 3,
    SET         = 4,
    STAGE       = 5,
    BUTOU       = 6,
    BOSS_DANTE  = 7,
    OTHER       = 8
};

enum GUARD_TYPE {
    GT_NO_GUARD,
    GT_NORMAL,
    GT_DEFEND_FAIL,
    GT_ATTACK_FAIL,
    GT_ATTACK_FAIL2,
    GT_BOTH_FAIL
};

class uDamage_New : public cUnit{
public:
    float mHitPoint;
    float mMaxHitPoint;
    int mPointCorrect;
    int mLvCorrect;
    float mDamageCorrect;
    float mDamageValue;
    float mActualDamageValue;
    int mInterrupt[5];
    int mBlown[5];
    int mMaxInterrupt[5];
    int mMaxBlown[5];
    uint mState;
    uint mEffect;
    bool mIsDead;
    undefined field15_0x8d;
    undefined field16_0x8e;
    undefined field17_0x8f;
    uintptr_t mpSoundRequest;
    GUARD_TYPE mGuardType; /* Created by retype action */
    bool mResetInterruptReq;
    bool mResetBlownReq;
    bool mIsEmToEm;
    undefined field23_0x9b;
    kAttackStatus* mpLastAttackStatus;
    int mEffectReduceCount; /* Created by retype action */
    float mEffectReduceTimer;
    undefined field27_0xa8;
    undefined field28_0xa9;
    undefined field29_0xaa;
    undefined field30_0xab;
    float mTimer;
    float mStateTimer;
    kAttackStatus* mpAttackStatus;
    kDefendStatus* mpDefendStatus; /* Created by retype action */
    uActor* mpAttackActor;         /* Created by retype action */
    uActor* mpDefendActor;         /* Created by retype action */
    uModel* mpModel;               /* Created by retype action */
    enum UDAMAGE_TYPE mType;              /* Created by retype action */
    uint mComboRank;                      /* Created by retype action */
    uintptr_t mpDefendStatusData;         /* Created by retype action */
};
#endif
