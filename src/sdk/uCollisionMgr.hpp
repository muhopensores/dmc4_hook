#ifndef UCOLLISIONMGR_HPP
#define UCOLLISIONMGR_HPP



#include "MtMath.hpp"
#include "uActor.hpp"

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
    cCollPrimType mType;
    int mBone0;
    int mBone1;
    float mRadius;
    MtVector3 mPos0;
    MtVector3 mPos1;
    uint mAttr;
    uint mId;
    float mShrink;
    int mDummy;
};
static_assert(sizeof(kCollPrim) == 0x40);

struct cCollision {
    bool mHit;
    kCollPrim* mpCollPrim;
    float mScale;
    int paddingC;
    MtAABB mAABB;
    bool mAABBHit;
    cCollisionGroup* mpCollisionGroup;
    uint32_t padding38[2];
    MtMatrix mMat0;
    MtMatrix mMat1;
    MtMatrix mOldMat0;
    MtMatrix* mpRefMat0;
    MtMatrix* mpRefMat1;
    bool mUseOld;
    char padding109[7];
};
//static_assert(sizeof(cCollision) == 0x110);

struct cCollisionGroup {
    uint mSeqNo;
    uint mKind;
    uint mAttr;
    uint mVsAttr;
    bool mRide;
    bool mFrdAtck;
    int padding12[3];
    MtVector3 mHitPos;
    MtVector3 mShrinkHitPos;
    cCollision* mpHitCollision;
    int padding44[3];
    MtVector3 mPushPos;
    cCollision* mpPushCollision;
    bool mNoThrough;
    int mCreatedFromIdx;
    bool mClear;
    MtAABB mAABB;
    bool mAABBHit;
    int mNum;
    uCollisionMgr* mpCollisionMgr;
    cCollision** mppCollision;
    uintptr_t mpCollGroup;
    kAttackStatus mAttackStatus;
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
#endif