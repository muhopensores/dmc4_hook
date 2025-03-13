//typedef unsigned char   undefined;
//typedef long long    longlong;
namespace uActorMain {
    typedef short sword;
    typedef unsigned char uchar;
    typedef unsigned int uint;
    // typedef unsigned long    ulong;
    // typedef unsigned long long    ulonglong;
    // typedef unsigned char    undefined1;
    // typedef unsigned short    undefined2;
    // typedef unsigned int    undefined4;
    // typedef unsigned long long    undefined6;
    // typedef unsigned long long    undefined8;
    typedef unsigned short ushort;
    typedef unsigned short word;

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

    struct MtFloat3 {
        float x;
        float y;
        float z;
    };

    struct MtSphere {
        struct MtFloat3 pos;
        float r;
    };

    struct cWorkRate {
        void *vtable;
        float mWorkRate;
        uint mType;
        float HalfRate;
        struct uDevil4Model *mpModel;
    };

    struct MtVector3 { /* 10 */
        float x;
        float y;
        float z;
        uint32_t padding;
    };
    static_assert(sizeof(MtVector3) == 0x10);

    struct MtEaseCurve {
        float p1;
        float p2;
    };

    struct MtVector4 { /* MtFramework vector */
        float x;
        float y;
        float z;
        float w;
    };
    static_assert(sizeof(MtVector4)==0x10);

    struct MtMatrix { /* MtFramework matrix */
        struct MtVector4 vectors[4];
    };
    static_assert(sizeof(MtMatrix) == 0x40);

    struct MtCapsule {
        struct MtVector3 p0;
        struct MtVector3 p1;
        float r;
        uint32_t padding[3];
    };

    struct cUnit {
        uintptr_t* vtable_ptr;
        uint8_t uknFlag1;
        uint8_t uknFlag2;
        uint8_t mTransMode;
        uint8_t mTransView;
        struct cUnit *mp_next_unit;
        struct cUnit *mp_prev_unit;
        float m_delta_time;
        uint8_t reserved_state_flags[4];
    };
    static_assert(sizeof(cUnit) == 0x18);

    struct uCoord {
        struct cUnit cUnitBase;
        uintptr_t mParent;
        uint32_t ParentJoint;
        uint32_t mOrder;
        uint32_t padding24[3];
        struct MtVector3 mPos;
        struct MtVector4 mQuat;
        struct MtVector3 mScale;
        struct MtMatrix mLmat;
        struct MtMatrix mWmat;
    };
    static_assert(sizeof(uCoord) == 0xe0);

    namespace cShape {
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
            struct cShape::WeightNode mWeightList[4];
            uintptr_t mpModel;
            uint mPause;
        };
        static_assert(sizeof(cShape) == 0xe0);
    };

    namespace uModelMain {
        struct uModel;
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
            struct uModelMain::uModel* owner;
            uint index;
        };
        static_assert(sizeof(MaterialGroup) == 0xC);

        struct uModel {
            struct uCoord uCoordBase;
            uint8_t UknChainFlag;
            uint8_t padding0[15];
            struct MtMatrix mPrevWmat;
            uint mLightGroup;
            uint mShadowCastGroup;
            uint mShadowRecvGroup;
            uint mPartsDisp[16];
            uint SkinRemap[64];
            uint SkinRemapNum;
            struct RenderInfo Render;
            struct MtVector4 uknJointVector1;
            struct MtVector3 uknJointVector2;
            struct MtVector3 uknJointVector3;
            uint mJointNum;
            uintptr_t mpJoint;
            uintptr_t mJointTable;
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
            struct cShape::cShape mShape; /* Created by retype action */
            char paddingce4[12];
        };
        static_assert(sizeof(MaterialGroup) * 32 == 0x180);
    };


    struct cFreeze {
        void *vtable;
        uint8_t mSchedulerMode;
        uint8_t mEnable;
        ushort padding6;
        uintptr_t mpTexBase;
        uintptr_t mpTexNormal;
        uintptr_t mpTexMask;
        uint padding14[3];
        struct MtVector3 mSphereCenter;
        float mSphereBandwith;
        float mIceBias;
        float mFresnelBias;
        float mFresnelFactor;
        float mSpecularPow;
        float mEnvmapPower;
        float mNormalBias;
        float mNormalWrap;
        struct MtVector3 mTransmit;
        uint mFog;
        float mSphereRadiusMax;
        float mSphereRadiusMin;
        float mIceTransparencyMax;
        float mIceTransparencyMin;
        float mFreezeTimerMax;
        float mFreezeTimerNow;
        float mSphereRadius;
        float mIceTransparency;
        uint padding84[3];
    };

    struct uDevil4Model {
        struct uModelMain::uModel uModelBase;
        struct cWorkRate mWorkRate;
        uint mCameraTransparencyType;
        uint mCameraTransparencyEnable;
        int paddingd0c;
        struct MtSphere mCameraSphere;
        uint mSphereJointNo;
        float mAlphaMin;
        float mAlphaMax;
        float mDistanceMin;
        float mDistanceMax;
        uint mEnableClip;
        uint mClipNormalType;
        uint mClipPosType;
        struct MtVector3 mLocalPlanePos;
        struct MtVector3 mWorldPlanePos;
        struct MtVector3 mLocalPlaneNormal;
        struct MtVector3 mWolrdPlaneNormal;
        struct MtVector4 mWorldPlaneVector;
        struct cFreeze mFreeze;
        float mTransparency;
        char padding24[12];
        struct MtVector3 mLMF;
        uint mMaterialType;
        uint8_t mZWrite;
        uint8_t mZThrough;
        short padding46;
        uint mAlphaRef;
        uint MaterialHandlerNum;
        uintptr_t MaterialHandlerArr;
        uint JointHandlerNum;
        uintptr_t JointHandlerArr;
        uint PartHandlerNum;
        uintptr_t PartHandlerArr;
        char padding64[12];
        struct MtVector3 UknCameraPos1;
        struct MtVector3 UknCameraPos2;
        uintptr_t MeshCullParamsPtr;
        uint MeshCount;
        char paddinge98[8];
    };

    namespace uActor {
        struct LOCKONSPHERE_DAT {
            struct MtSphere Sphere;
            uint jointNum;
            char padding14[12];
            struct MtVector3 offsetpos;
        };

        struct uActor { /* Actor class */
            struct uDevil4Model uActorBase;
            uint UknActorFlag1;
            uint mActorType; /* Created by retype action */
            uint mActorStatus;
            uint UknActorFlag2;
            uint TargetSignal;
            float uknTargetFloat;
            float uknTargetFloat1;
            struct uActor *mpTarget;
            struct MtVector3 mVel;
            struct MtVector3 mAcc;
            struct MtVector3 CenterJointPos;
            uint muaLockonSphereNum;
            char paddingef4[12];
            struct uActor::LOCKONSPHERE_DAT LockonSpheres[16];
            uint8_t HitStopEndFlag;
            uint8_t isHitStop;
            short padding1202;
            uintptr_t mpSoundRequest;
            uint8_t UknEnemyDtSfxFlg;
            char padding1209[7];
            float mRotY;
            uint mIDQueue[16];
            uint8_t mIDQueueStart;
            uint8_t mIDQueueValue;
            short padding1256;
            uint field26_0x1258[32];
            uintptr_t ParentArchive;
            uint mTransparency;
            struct MtVector3 uknKnockbackVector1;
            struct MtVector3 uknKnockbackVector2;
            struct MtVector4 uknKnockbackVector3;
            uint UknKnockbackAngleSign1;
            uint UknKnockbackAngleSign2;
            uintptr_t JcHitboxParamTable;
            uint JcHitboxNum;
            uint isJumpCancellable;
            uint padding1324[3];
            struct MtVector4 CurrQuat;
            struct MtVector4 PrevQuat;
            struct MtVector3 CurrPos;
            struct MtVector3 PrevPos;
        };
        static_assert(sizeof(uActor) == 0x1370);
    }; // namespace uActor

    struct uCollisionMgr {
        struct cUnit base;
        byte mHit;
        byte mGrab;
        byte mDamage;
        byte mPsh;
        byte mLand;
        byte mTimedForceLand;
        byte mSetLand;
        byte mWall;
        byte mCeilling;
        byte padding21[2];
        byte mNoLandTimerEnable;
        float mNoLandTimer;
        float mNoLandTime;
        byte ForceLandFlag;
        byte mOldLand;
        byte mOldSetLand;
        byte mOldWall;
        byte mOldCeilling;
        byte mCheckWallEnable;
        byte mCheckGroundEnable;
        char padding33[9];
        byte mFall;
        byte NoLandTimerType;
        byte NoLandDTSwitch;
        byte field25_0x3f;
        byte NoLandTimerType2;
        float DamageValueCorrect;
        uint32_t EnemyCollisionToggle;
        uint32_t padding4c;
        struct MtVector3 LockonTargetPos;
        uintptr_t LockonTarget;
        uint32_t padding64[3];
        struct MtVector3 uknFixedVec1;
        struct MtMatrix GroundMat;
        struct MtVector3 uknFixedVec2;
        uint32_t mWeightType;
        uint32_t mModelID;
        struct uActorMain::uActor::uActor* mpReportActor;
        struct uActorMain::uActor::uActor* mpDstModel; /* Created by retype action */
        struct uActorMain::uActor::uActor* mpSrcModel;
        uintptr_t uknSrcModelPtr;
        uint32_t paddinge8[17];
        uint32_t mPushType;
        uintptr_t mpPushModel;
        uint32_t padding134[3];
        struct MtCapsule mPushCap;
        struct MtCapsule mPushCap1;
        struct MtVector3 mPushPos;
        struct MtVector3 mPushPosOld;
        struct MtMatrix field50_0x1c0;
        struct MtMatrix field51_0x200;
        char padding240[64];
        uint32_t uknToggle;
        uintptr_t mpCollisionIdxData;
        uintptr_t mpCollisionShape;
        uintptr_t mpAttackStatusData;
        uintptr_t mpDefendStatusData;
        uint32_t mInDamageMessage;
        uint32_t currentMotBuffer;
        uint32_t MotSeq1;
        uint32_t MotSeq2;
        byte HitCheckFlag;
        byte HitConfirm;
        byte mSelfCollision;
        byte padding2a7;
        struct uCollisionMgr* uknCollMgrPtr;
        uint32_t mMode;
        uint32_t mVsAttrPlAtk;
        uint32_t mVsAttrPlDmg;
        uint32_t mVsAttrPlPsh;
        uint32_t mVsAttrPlGrb;
        uint32_t mVsAttrPsAtk;
        uint32_t mVsAttrPsDmg;
        uint32_t mVsAttrPsPsh;
        uint32_t mVsAttrPsGrb;
        uint32_t mVsAttrEmAtk;
        uint32_t mVsAttrEmDmg;
        uint32_t mVsAttrEmPsh;
        uint32_t mVsAttrEmGrb;
        uint32_t mVsAttrEsAtk;
        uint32_t mVsAttrEsDmg;
        uint32_t mVsAttrEsPsh;
        uint32_t mVsAttrEsGrb;
        uint32_t mVsAttrSetAtk;
        uint32_t mVsAttrSetDmg;
        uint32_t mVsAttrSetPsh;
        uint32_t mVsAttrSetGrb;
        uint32_t mVsAttrStgAtk;
        uint32_t mVsAttrStgDmg;
        uint32_t mVsAttrStgPsh;
        uint32_t mVsAttrStgGrb;
        uint32_t UknVsAttrAtk1;
        uint32_t UknVsAttrDmg1;
        uint32_t UknVsAttrPsh1;
        uint32_t UknVsAttrGrb1;
        uint32_t UknVsAttrAtk2;
        uint32_t UknVsAttrDmg2;
        uint32_t UknVsAttrPsh2;
        uint32_t UknVsAttrGrb2;
        uint32_t UknVsAttrAtk3;
        uint32_t UknVsAttrDmg3;
        uint32_t UknVsAttrPsh3;
        uint32_t UknVsAttrGrb3;
        uint32_t UknVsAttrAtk4;
        uint32_t UknVsAttrDmg4;
        uint32_t UknVsAttrPsh4;
        uint32_t UknVsAttrGrb4;
        uint32_t UknCollisionToggle;
        uint32_t mCollisionGroupNum;
        uintptr_t mppCollisionGroup[32];
        uint32_t padding3d8[6];
    };
    //static_assert(sizeof(uCollisionMgr) == 0x3f0);

    struct uCamera {
        struct cUnit base;
        float mFarPlane;
        float mNearPlane;
        float mAspect;
        float mFov;
        long padding28[2];
        MtVector3 mCameraPos;
        MtVector3 mTargetPos;
        MtVector3 mCameraUp;
        MtVector4 mFrustum[6];
    };

    struct uFreeCamera {
        struct uCamera uCameraBase;
        struct uCoord* mpParent;
        long mParentNo;
        struct uCoord* mpTarget;
        long mTargetNo;
        long mControlPad;
        char paddingd4[12];
        MtVector3 mControlSpeed;
    };
};