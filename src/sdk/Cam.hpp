#pragma once
#include <cstdint>

#ifndef UACTOR_HPP
#include "uActor.hpp"
#endif

#ifndef MTMATH_HPP
#include "MtMath.hpp"
#endif

struct sCamera_ViewPort {
    void* vtable;
    void* mpCamera;
    void* mpTestCamera;
    uint32_t mAttr;
    uint8_t mActive;
    uint8_t mNo;
    uint8_t mSceneNo;
    uint8_t mMode;
    MtRect mRegion;
    char padding0[12];
    MtVector4 mFrustum[6];
    MtColor mClearColor;
    float mClearZ;
    uint32_t mClearStencil;
    void* mpRenderTarget;
    void* mpDepthStencil;
    void* mpSubPixelMask;
    char padding1[8];
    MtMatrix mViewMat;
    MtMatrix mProjMat;
    MtMatrix mPrevViewMat;
    MtMatrix mPrevProjMat;
    float mFogStart;
    float mFogEnd;
    uint32_t mFogPlaneType;
    float mFogPlaneWidth;
    float mFogPlaneDist;
    float mFogAttentuation;
    char padding2[8];
    MtVector4 mFogPlaneDir;
    MtVector4 mFogColor;
    MtVector4 UknVecs1[2];
    MtVector4 UknVecs2[2];
    MtVector4 mTransFogFactor[2];
    MtVector4 mTransFogColor[2];
    MtMatrix mTransViewMat[2];
    MtMatrix mTransProjMat[2];
    MtMatrix mTransViewProjMat[2];
    MtMatrix mTransPrevViewMat[2];
    MtMatrix mTransPrevProjMat[2];
    MtMatrix mTransPrevViewProjMat[2];
    MtVector4 mTransEyePos[2];
};
static_assert(sizeof(sCamera_ViewPort) == 0x590);

struct sCamera : CSystem {
    float mSubPixelOfsX;
    float mSubPixelOfsY;
    float mViewSubFrame;
    float mWorldSubFrame;
    sCamera_ViewPort viewports[8];
    MtRect ScreenRect;
    float SceneSize[2];
    uint32_t mLayoutMode;
    uint32_t mPause;
};

class uCamera: public cUnit {
public:
    float mFarPlane;
    float mNearPlane;
    float mAspect;
    float mFov;
    long padding28[2];
    MtVector3 mCameraPos;
    MtVector3 mCameraUp;
    MtVector3 mTargetPos;
    MtVector4 mFrustum[6];
};
static_assert(sizeof(uCamera) == 0xc0);

struct VIB_REQUEST_DATA {
    int32_t prio;
    int padding[3];
    MtVector4 vibVec;
};
static_assert(sizeof(VIB_REQUEST_DATA) == 0x20);

class uDevilCamera : public uCamera {
public:
    float mRoll;
    int padding[3];
    MtVector4 mFrustumAppli[6];
    MtVector4 mFrustumVertex[8];
    MtMatrix mViewMat;
    MtMatrix mProjMat;
    MtMatrix mInverseViewMat;
    uint32_t mVibUnitCreateNum;
    uint32_t mVibRequestNum;
    int padding278[2];
    VIB_REQUEST_DATA mVibRequest[8];
    bool mVibEndFlag;
    schar mTypeStickAxisY;
    bool mTypeStickSixAxis;
    bool mPlayerJumpFlag;
    bool mPlayerJumpBeginFlag;
    char padding385[11];
    MtVector3 mPlayerJumpPoint;
};
static_assert(sizeof(uDevilCamera) == 0x3a0);

class uFreeCamera: public uCamera {
public:
    uCoord* mpParent;
    long mParentNo;
    uCoord* mpTarget;
    long mTargetNo;
    long mControlPad;
    char paddingd4[12];
    MtVector3 mControlSpeed;
};

class cCameraBase {
public:
    void* vtable;
    int padding4[3];
    MtVector3 mCameraPos;
    MtVector3 mTargetPos;
    MtVector3 mCameraUp;
    float mNearPlane;
    float mFarPlane;
    float mAspect;
    float mFov;
    float mRoll;
    int padding54[3];
};
static_assert(sizeof(cCameraBase) == 0x60);

class cCameraPlayer : public cCameraBase {
    schar mType;
    uint mRno;
    uint mNextRno; /* Created by retype action */
    CResource* mpCam;
    MtVector3 mLockonTargetPos;
    MtVector3 mLockonTargetBasePos;
    MtVector3 mLockonTargetCenterPos;
    uintptr_t mpLockOnTargetOld; /* Created by retype action */
    float mLockOnAllowance;
    int mLockOnTargetID;
    int mScreenOutTimer;
    float mZoom1vs1PlusFov;
    float mZoom1vs1Timer;
    int paddingB8[2];
    MtVector3 mCorrectBasePos;
    float mHoseiY;
    float mRotX;
    float mLen;
    float mLenBattle;
    float mLenNow;
    float mFovDefault;
    float mFovBattle;
    int paddingEC;
    MtVector3 mInitialPos[16];
    uint mInitialPosNum;
    int padding1F4[3];
    MtVector3 mCamDir;
    MtVector3 mEndTar;
    MtVector3 mEndPos;
    MtVector3 mOldPos;
    MtVector3 mOldTar;
    MtVector3 mOldPlPos;
    float mRotY;
    float mRotYResetEnd;
    float mAddRotY;
    char mAddRotYFlag;
    float mAddRotYValue;
    float mAddYResetEnd;
    float mAddRotX;
    schar mAddRotXFlag;
    float mAddRotXStep;
    schar mRotYResetFlag;
    schar mRotYResetTimer;
    int padding[2];
    MtVector3 mLockOffPlPos;
    bool mLockOffPlMoveFlag;
    undefined field86_0x2a1;
    undefined field87_0x2a2;
    undefined field88_0x2a3;
    float mXZMoveDist[10];
    int mXZMoveTimer;
    bool mPlMoveXZFlag;
    bool mPlAirFlag;
    bool mFlexLengthFlag;
    bool mOldFlexLengthFlag;
    bool mFlexLengthOffFlag;
    undefined field96_0x2d5;
    undefined field97_0x2d6;
    undefined field98_0x2d7;
    float mPlusFlexLength;
    bool mThrowStart;    /* Created by retype action */
    bool mThrow;               /* Created by retype action */
    bool mThrowEnd;      /* Created by retype action */
    bool mThrowType;           /* Created by retype action */
    bool mThrowSameLockOnFlag; /* Created by retype action */
    bool mThrowStartLockOnFlag;
    int padding2E2[3];
    MtVector3 mThrowTargetPos;
    MtVector3 mThrowTargetBasePos;
    float mThrowEndTimer;
    uActor* mpThrowTarget;
    float mThrowTimer[3];
    float mThrowParam[2];
    bool mBusterType;
    float mPlRotY;
    float mCamAngleRangeY;
    float mCamHoseiAngleY;
    float mHalfLifePos;
    float mHalfLifeTar;
    float mLockOffHalfLifePos;
    float mLockOffHalfLifeTar;
    float mLockOnHalfLifePos;
    float mLockOnHalfLifeTar;
    float mLockOffTimer;
    float mShiftLockOffTimer;
    float mThrowEndHalfLifePos;
    float mThrowEndHalfLifeTar;
    bool mPassWallFlag;
    bool mPushWallRotXFlag; /* Created by retype action */
    bool mLockOnValidFlag;
    bool m1vs1ValidFlag;
    bool mAlwaysZoomFlag;
    float mAlwaysZoomFovMax;
    bool mBlitzExAttackNew; /* Created by retype action */
    bool mBlitzExAttackOld; /* Created by retype action */
    float mExAttackTimer[2];
    float mExAttackParam[2];
    int padding384[3];
    MtVector3 mExAttackPos[4];
    bool mExAttackFlag[2];
    float mAngleRangeY;
    float mAngleHoseiY;
    float mOffLenAngleHoseiY;
    float mOnLenAngleHoseiY;
    bool mAngleHoseiYFlag;
    float mTimeOffAngleHoseiY;
    int mSensorGSwitch;
    int mSensorGTimer;
    int mSensorRotYTimer;
    short mStickX;
    short mSensorZSwitch;
    int mSensorResetTimer;
    float mSensorZ[5];
    bool mWideTvMode;
    char padding415[11];
};
static_assert(sizeof(cCameraPlayer) == 0x420);

struct cCamAreaConnect {
    void* vtable;
    unsigned char no;   /* Created by retype action */
    unsigned char time; /* Created by retype action */
    undefined field6_0x6;
    undefined field7_0x7;
};

struct cCameraRect3D {
    undefined** vftable;
    uint mVertexNum;
    undefined field2_0x8;
    undefined field3_0x9;
    undefined field4_0xa;
    undefined field5_0xb;
    undefined field6_0xc;
    undefined field7_0xd;
    undefined field8_0xe;
    undefined field9_0xf;
    struct MtVector3 mVertex[6];
    float mHeight;
    undefined field12_0x74;
    undefined field13_0x75;
    undefined field14_0x76;
    undefined field15_0x77;
    undefined field16_0x78;
    undefined field17_0x79;
    undefined field18_0x7a;
    undefined field19_0x7b;
    undefined field20_0x7c;
    undefined field21_0x7d;
    undefined field22_0x7e;
    undefined field23_0x7f;
};

struct cCamAreaData {
    void* vtable;
    ushort cam_type;  /* Created by retype action */
    uchar fly_cam;    /* Created by retype action */
    uchar snatch_cam; /* Created by retype action */
    uchar reserve_cam;
    uchar box_num; /* Created by retype action */
    undefined field9_0xa;
    undefined field10_0xb;
    cCamAreaConnect ConnectData[16];
    uchar priority;
    undefined field13_0x8d;
    undefined field14_0x8e;
    undefined field15_0x8f;
};

class cCameraRailBase : public cCameraBase {
public:
    cCamAreaData mCommonData;
    uint mBoxNum;                  /* Created by retype action */
    cCameraRect3D** mppBox; /* Created by retype action */
    uDevilCamera* mpcam;
    bool mOffFlag;
    float mFogStart;
    float mFogEnd;
    float mFlogPlaneWidth;
    float mFogPlaneDist;
    MtVector3 mFogPlaneDir;
    MtVector4 mFogColor;
    float mBattleZoomRate;
    float mDist;
    float mHalfLifePos;
    float mHalfLifeTar;
    float mLockonPer;
    float m2PanTimer;
    float m2PanSetTime;
    float m2PanHalfLifeTar;
    bool mLockOnOldFlag;
    undefined field24_0x151;
    undefined field25_0x152;
    undefined field26_0x153;
    undefined field27_0x154;
    undefined field28_0x155;
    undefined field29_0x156;
    undefined field30_0x157;
    undefined field31_0x158;
    undefined field32_0x159;
    undefined field33_0x15a;
    undefined field34_0x15b;
    undefined field35_0x15c;
    undefined field36_0x15d;
    undefined field37_0x15e;
    undefined field38_0x15f;
};

class uCameraCtrl : public uDevilCamera {
public:
    CResource* mpResource;
    uchar mNowArea;
    uchar mOldArea;         /* Created by retype action */
    uchar mBeforeNohitArea; /* Created by retype action */
    uchar field5_0x3a7;
    int mCheckedHitBoxNo;
    float mFogStart;
    float mFogEnd;
    float mFogPlaneWidth;
    float mFogPlaneDist;
    undefined field11_0x3bc;
    undefined field12_0x3bd;
    undefined field13_0x3be;
    undefined field14_0x3bf;
    MtVector3 mFogPlaneDir;
    MtVector4 mFogColor;
    float mFogStartTmp;
    float mFogEndTmp;
    float mFogPlaneWidthTmp;
    float mFogPlaneDistTmp;
    MtVector3 mFogPlaneDirTmp;
    MtVector4 mFogColorTmp;
    float mOldFov;
    float mOldRoll;
    undefined field25_0x418;
    undefined field26_0x419;
    undefined field27_0x41a;
    undefined field28_0x41b;
    undefined field29_0x41c;
    undefined field30_0x41d;
    undefined field31_0x41e;
    undefined field32_0x41f;
    MtVector3 mOldCameraPos;
    MtVector3 mOldTargetPos;
    int mTransitionFrame;
    float mTransitionTimer;
    float mOldFogStart;
    float mOldFogEnd;
    float mOldFogPlaneWidth;
    float mOldFogPlaneDist;
    undefined field41_0x458;
    undefined field42_0x459;
    undefined field43_0x45a;
    undefined field44_0x45b;
    undefined field45_0x45c;
    undefined field46_0x45d;
    undefined field47_0x45e;
    undefined field48_0x45f;
    MtVector3 mOldFogPlaneDir;
    MtVector4 mOldFogColor;
    bool mFirstAreaChange;
    undefined field52_0x481;
    undefined field53_0x482;
    undefined field54_0x483;
    uint mActStatus;
    uint mActStatusExt;
    bool mFlgSetActStatus;
    undefined field58_0x48d;
    undefined field59_0x48e;
    undefined field60_0x48f;
    cCameraBase* mpNowCamera; /* Created by retype action */
    cCameraRailBase* mpCamRailBase;
    cCameraPlayer* mpCamPlayer;
    bool mDelaySet;
    undefined field65_0x49d;
    undefined field66_0x49e;
    undefined field67_0x49f;
};
