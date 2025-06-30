//typedef unsigned char   undefined;
//typedef long long    longlong;
#ifndef UACTOR_HPP
#define UACTOR_HPP


#include "MtMath.hpp"
#include "ReClass_Internal.hpp"

// typedef unsigned long    ulong;
// typedef unsigned long long    ulonglong;
// typedef unsigned char    undefined1;
// typedef unsigned short    undefined2;
// typedef unsigned int    undefined4;
// typedef unsigned long long    undefined6;
// typedef unsigned long long    undefined8;


//typedef struct uActor uActor, *PuActor;
//
//typedef struct uDevil4Model uDevil4Model, *PuDevil4Model;
//
//typedef struct MtVector3 MtVector3, *PMtVector3;
//
////typedef struct LOCKONSPHERE_DAT uActor::LOCKONSPHERE_DAT, *PuActor::LOCKONSPHERE_DAT;
//
//typedef uchar uint8_t;
//
//typedef uint uintptr_t;
//
//typedef struct MtVector4 MtVector4, *PMtVector4;
//
//typedef struct uModel uModel, *PuModel;
//
//typedef struct cWorkRate cWorkRate, *PcWorkRate;
//
//typedef struct MtSphere MtSphere, *PMtSphere;
//
//typedef struct cFreeze cFreeze, *PcFreeze;
//
//typedef struct uCoord uCoord, *PuCoord;
//
//typedef struct MtMatrix MtMatrix, *PMtMatrix;
//
////typedef struct uModel::RenderInfo uModel::RenderInfo, *PuModel::RenderInfo;
//
//typedef struct MtEaseCurve MtEaseCurve, *PMtEaseCurve;
//
////typedef struct uModel::Motion uModel::Motion, *PuModel::Motion;
//
////typedef struct uModel::MaterialGroup uModel::MaterialGroup, *PuModel::MaterialGroup;
//
//typedef struct cShape cShape, *PcShape;
//
//typedef struct MtFloat3 MtFloat3, *PMtFloat3;
//
//typedef struct cUnit cUnit, *PcUnit;
//
////typedef struct uModel::MPARAM_WORK uModel::MPARAM_WORK, *PuModel::MPARAM_WORK;
//
////typedef struct cShape::WeightNode cShape::WeightNode, *PcShape::WeightNode;

//struct uCoord: cUnit {
//    uintptr_t mParent;
//    uint32_t ParentJoint;
//    uint32_t mOrder;
//    uint32_t padding24[3];
//    MtVector3 mPos;
//    MtVector4 mQuat;
//    MtVector3 mScale;
//    MtMatrix mLmat;
//    MtMatrix mWmat;
//};
//static_assert(sizeof(uCoord) == 0xe0);

class uModel;
class uDevil4Model;
class uActor;

struct WeightNode {
    void *vtable;
    uint mpShape;
    uint mpWeight;
    float mSpeed;
    float mInterRate;
    float mFrame;
    float mFrameMax;
    ushort mAttr;
    ushort mState;
};

struct cShape {
    void *vtable;
    uintptr_t mpMap;
    uintptr_t mpPrevMap;
    uintptr_t mKeyWeight;
    uintptr_t mpShape;
    uint mTarget[16];
    ushort mAttr;
    ushort mState;
    WeightNode mWeightList[4];
    uintptr_t mpModel;
    uint mPause;
};
static_assert(sizeof(cShape) == 0xe0);

class kEmJumpData /* size : 0x00000020 */
{
public:
    // void kEmJumpData(const kEmJumpData *) /* public */;
    // void kEmJumpData(void) /* public */;
    // kEmJumpData *operator=(const kEmJumpData *) /* public */;
    /*****************************************/
    /*****************************************/
    int jointNo;      // 0x00000000
    float radius;     // 0x00000004
    char pad_08[8];   // 0x00000008
    MtVector3 offset; // 0x00000010
};
static_assert(sizeof(kEmJumpData) == 0x20);

class kEmJumpDataArr {
public:
    class kEmJumpData enemyStepSphere[30];
}; // Size: 0x3C0
static_assert(sizeof(kEmJumpDataArr) == 0x3C0);

class UModelJoint { // redef, was in AfterImage.cpp
public:
    void* vtable;            // 0x00
    byte mAttr;              // 0x04
    byte mParentIndex;       // 0x05
    byte mType;              // 0x06
    byte mNo;                // 0x07
    int mSymmetryIndex;      // 0x08
    void* mpConstraint;      // 0x0C
    struct MtFloat3 mOffset; // 0x10
    float mLength;           // 0x1C
    MtVector4 mQuat;         // 0x20
    MtVector3 mScale;        // 0x30
    MtVector3 mTrans;        // 0x3C
    MtMatrix mWmat;          // 0x48
};
static_assert(sizeof(UModelJoint) == 0x90);

class UModelJointArr {
public:
    class UModelJoint joint[200]; // 0x0000
};                                // Size: 0x2A30
static_assert(sizeof(UModelJointArr) == 0x7080);


class uModel;
struct MPARAM_WORK {
    uintptr_t pparam;
    float cur_frame;
    uintptr_t pcur_param;
    float weight;
};
static_assert(sizeof(MPARAM_WORK)==0x10);

struct Motion {
    void* vtable;
    ushort mMotionNo;
    ushort mReserved;
    ushort mState;
    ushort mAttr;
    float mInterFrame;
    float mInterCount;
    float mBlend;
    float mFrame;
    float mFrameMax;
    float mLoopFrame;
    float mSpeed;
    float mInterRate;
    float mNextFrame;
    struct MtEaseCurve mInterCurve;
    int padding38[2];
    struct MtVector4 mPrevQuat;
    struct MtVector4 mBaseQuat;
    struct MtVector4 mNullQuat;
    struct MtVector3 mPrevTrans;
    struct MtVector3 mBaseTrans;
    struct MtVector3 mNullTrans;
    struct MPARAM_WORK mTransParam;
    struct MPARAM_WORK unusedQuatParam;
    uintptr_t mpSeqInfo[2];
    int mSequence[2];
    int mPrevSequence[2];
    uintptr_t mJoint;
    int paddingdc;
};
static_assert(sizeof(Motion) == 0xE0);

struct RenderInfo {
    void* vtable;
    uint mRenderMode;
    uint mVFCullLevel;
    int mZPrepassDist;
    int mMotionBlurDist;
    uint mPriorityBias;
    uint mLOD;
    float mMotionBlurLimit;
    uint mSOEnable;
    uintptr_t mpModel;
    int padding28[2];
};
static_assert(sizeof(RenderInfo) == 0x30);

struct MaterialGroup {
    void* vtable;
    uModel* owner;
    uint index;
};
static_assert(sizeof(MaterialGroup) == 0xC);

class uModel: public uCoord {
public:
    uint8_t mContinueCount;
    uint8_t padding0[15];
    struct MtMatrix mPrevWmat;
    uint mLightGroup;
    uint mShadowCastGroup;
    uint mShadowRecvGroup;
    uint mPartsDisp[16];
    uint SkinRemap[64];
    uint SkinRemapNum;
    struct RenderInfo Render;
    MtSphere mBoundSphere;//bsphere
    MtAABB mBoundingBox;
    uint mJointNum;
    UModelJoint* mpJoint;
    uchar* mJointTable;
    uint padding2ec;
    struct MtVector4 mBasePlane;
    float mWaistOffset;
    float mWaistAdjustFactor;
    float mWaistAdjust;
    struct MtEaseCurve mWaistAdjustCurve;
    uint8_t mAnkleAdjust;
    uint8_t mFootAdjust;
    short padding316;
    float mWaistAdjustSpeed;
    float mHeelHeight;
    float mRotInverseWeight;
    uint mBlendNum;
    char padding328[8];
    struct Motion MotionArray[8];
    uintptr_t mpRModel;
    char paddinga34[4];
    uintptr_t mpMotionListArr[16];
    uintptr_t mpMaterial;
    uint MaterialNum;
    struct MaterialGroup mMaterialGroupArr[32]; /* Created by retype action */
    uint mDebugFlags;
    cShape mShape; /* Created by retype action */
    char paddingCE4[12];
};
static_assert(sizeof(uModel) == 0xcf0);

struct cFreeze {
    void* vtable;
    uint8_t mSchedulerMode;
    uint8_t mEnable;
    uint16_t padding6;
    uintptr_t mpTexBase;
    uintptr_t mpTexNormal;
    uintptr_t mpTexMask;
    uint32_t padding14[3];
    MtVector3 mSphereCenter;
    float mSphereBandwith;
    float mIceBias;
    float mFresnelBias;
    float mFresnelFactor;
    float mSpecularPow;
    float mEnvmapPower;
    float mNormalBias;
    float mNormalWrap;
    MtVector3 mTransmit;
    uint32_t mFog;
    float mSphereRadiusMax;
    float mSphereRadiusMin;
    float mIceTransparencyMax;
    float mIceTransparencyMin;
    float mFreezeTimerMax;
    float mFreezeTimerNow;
    float mSphereRadius;
    float mIceTransparency;
    uint32_t padding84[3];
};

struct cWorkRate {
    void* vtable;
    float mWorkRate;
    uint32_t mType;
    float HalfRate;
    uDevil4Model* mpModel;
};

class uDevil4Model: public uModel {
public:
    struct cWorkRate mWorkRate;
    uint mCameraTransparencyType;
    uint mCameraTransparencyEnable;
    uint field4_0xd0c;
    struct MtSphere mCameraSphere;
    uint mSphereJointNo;
    float mAlphaMin;
    float mAlphaMax;
    float mDistanceMin;
    float mDistanceMax;
    uint mEnableClip;
    uint mClipNormalType;
    uint mClipPosType;
    MtVector3 mLocalPlanePos;
    MtVector3 mWorldPlanePos;
    MtVector3 mLocalPlaneNormal;
    MtVector3 mWolrdPlaneNormal;
    MtVector4 mWorldPlaneVector;
    cFreeze mFreeze;
    float mTransparency;
    uint pade24[3];
    MtVector3 mLMF;
    uint mMaterialType;
    uint8_t mZWrite;
    uint8_t mZThrough;
    uint mAlphaRef;
    uint MaterialHandlerNum;
    uintptr_t mppMaterialHandlerArray;
    uint mJointHandlerNum;
    uintptr_t mppJointHandlerArray;
    uint mPartsHandlerNum;
    uintptr_t mppPartsHandlerArray;
    char paddinge64[12];
    MtLineSegment mSegment;
    float* mpPrimTransparency;
    uint mPrimTransparencyNum;
    char paddinge98[8];
};
static_assert(sizeof(uDevil4Model) == 0xea0);

struct LOCKONSPHERE_DAT {
    struct MtSphere Sphere;
    uint jointNum;
    char padding[12];
    MtVector3 offsetpos;
};

struct NextRno {
    uint mNextRno0 : 8;
    uint mNextRno1 : 8;
    uint mNextRno2 : 8;
    uint mNextRno3 : 8;
};

class uActor: public uDevil4Model{ /* Actor class */
public:
    struct NextRno mNextRno;
    uint mActorType; /* Created by retype action */
    uint mActorStatus;
    uint mPullType;
    uint mAttackType;
    float mAttackResponse;
    float mAttackResponseTimer;
    uActor* mpAttackSender;
    MtVector3 mVel;
    MtVector3 mAcc;
    MtVector3 mCullingSphere;
    uint muALockonSphereNum;
    char paddingef4[12];
    LOCKONSPHERE_DAT muALockOnSphere[16];
    bool mUseUniqueID;
    bool mActorHitStop;
    bool mActorHitStopEnable;
    uintptr_t mpSoundRequest;
    bool mEnemyDeadSEMute;
    char padding1209[7];
    float mRotY;
    uint mIDQueue[16];
    uint8_t mIDQueueStart;
    uint8_t mIDQueueValue;
    short padding1256;
    uint field27_0x1258[32];
    CResource* mpRefResource;
    uint mbActorTrans;
    MtVector3 mDamagePos;
    MtVector3 mDamageVec;
    float mDamageAngleYF;
    float mDamageAngleYB;
    float mDamageAngleYFB;
    float mDamageAngleYFBLR;
    uint mDamageFBLR;
    uint mDamageFB;
    kEmJumpData* mpEmJumpData;
    uint mEmJumpEnableMaskFlag;
    bool mEmJumpEnableFlag;
    char padding1321[15];
    MtVector4 mNowFrameQuat;
    MtVector4 mOldFrameQuat;
    MtVector3 mNowFramePos;
    MtVector3 mOldFramePos;
};
static_assert(sizeof(uActor) == 0x1370);

enum LOCKON_MODEL_TYPE : unsigned int {
    MODEL_TYPE_EM          = 0,
    MODEL_TYPE_BOSS        = 1,
    MODEL_TYPE_EM_SHL      = 2,
    MODEL_TYPE_OBJ         = 3,
    MODEL_TYPE_SNATCH      = 4,
    MODEL_TYPE_SNATCH_EV   = 5,
    MODEL_TYPE_EM035       = 6,
    MODEL_TYPE_HUGE_STATUE = 7
};

class cLockonTarget {
    void* vtable;
    cLockonTarget* mpNext;
    uActor* mpModel;
    void* mpDmgTbl;
    MtVector3 mPos;
    LOCKON_MODEL_TYPE mType;
    float mPILength;
    float mMaxHP;
    float mHp;
    float mRange;
    bool mIsIn;
    bool mIsTarget;
    bool mIsCatch;
    bool mIsCameraIn;
    bool mIsNoTarget;
    bool mIsInterTarget;
    bool mIsNoInterTarget;
    int padding;
    MtVector3 mNearPos;
};
static_assert(sizeof(cLockonTarget) == 0x50);
#endif