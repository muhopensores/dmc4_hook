#pragma once

enum LIGHT_ATTR : unsigned char {
    ATTR_FADEOUT   = 1,
    ATTR_SH        = 2,
    ATTR_SUBTRACT  = 4,
    ATTR_PERPIXEL  = 8,
    ATTR_VIEWCOORD = 0x10,
    ATTR_DISP      = 0x20,
    ATTR_RANGE     = 0x40,
    ATTR_OOB_RANGE = 0x80
};

enum LIGHT_TYPE : unsigned char {
    TYPE_NONE,     // 0x0
    TYPE_INFINITE, // 0x1
    TYPE_POINT,    // 0x2
    TYPE_SPOT,     // 0x3
    TYPE_AMBIENT,  // 0x4
    TYPE_CAPSULE,  // 0x5
    TYPE_CUBOID,   // 0x6
    MAX_TYPE       // 0x7
};

struct uLight : cUnit {
    LIGHT_ATTR mAttr; // 0x18
    uint8_t int8_19;  // 0x19
    uint8_t int8_1a;  // 0x1a
    uint8_t int8_1b;  // 0x1b
    LIGHT_TYPE mType; // 0x1c
    MtVector4 mColor; // 0x20
};
static_assert(sizeof(uLight) == 0x30);

struct uInfiniteLight : uLight {
    MtVector3 mDir;
};

struct uSpotLight : uLight {
    MtVector3 mPos;
    MtVector3 mDir;
    MtVector3 mTargetPos;
    MtVector3 mWPos;
    MtVector3 mWDir;
    float mStart;
    float mEnd;
    float mCone;
    float mSpread;
    bool mTargetMode;
    uCoord* mpParent;
    uCoord* mpTarget;
    int mParentNo; // joint
    int mTargetNo; // joint
};

struct uPointLight : uLight {
    MtVector3 mPos;
    MtVector3 mWPos;
    float mStart;
    float mEnd;
    uCoord* mpParent;
    int mParentNo;
};

struct uHemiSphereLight : uLight {
    MtVector3 mRevColor;
    MtVector3 mDir;
};

struct uBackLight : uLight {
    struct SHFACTOR {
        MtVector4 r[3];
        MtVector4 g[3];
        MtVector4 b[3];
    } mFactor;
};
#if 0
struct uFilter : cUnit {
    uint32_t mPass;
    uint32_t mPriority;
};
static_assert(sizeof(uFilter) == 0x20);

enum DOF_TYPE: unsigned long {
    DOF_DETAIL = 0,
    DOF_SIMPLE = 1,
    DOF_SIMPLE2 = 2
};

enum BLUR_TYPE: unsigned long {
    BLUR_GAUSSIAN = 1,
    BLUR_MASKGAUSSIAN = 2
};

struct uDOFFilter : uFilter {
    uint32_t mBlurCount;
    DOF_TYPE mType;
    BLUR_TYPE mBlurType;
    bool mSimple;
    bool mbGSDOF;
    MtVector3 mGradateColor;
    float mBlurSize;
    uintptr_t mpTempTexture[2];
    float mAperture;
    float mFocalLength;
    float mLowCoCScale;
    float mCocScale;
    float mCoCBias;
    float mFarBlurLimit;
    float mNearBlurLimit;
    float mNear;
    float mFar;
    float mFocal;
    uintptr_t mpTempAccTexture;
    uintptr_t mpDiaphragm;
    int pad;
};
static_assert(sizeof(uDOFFilter) == 0x80);

struct uToneMapFilter : uFilter {
    uint32_t EnableToneMap : 1;
    uint32_t EnableManual : 1;
    uint32_t EnableMaxLum : 1;
    uint32_t EnableLogLum : 1;
    uint32_t EnableBloom : 1;
    uint32_t EnableGhost : 1;
    uint32_t EnableStar : 1;
    uint32_t EnableBlueShift : 1;
    uint32_t EnableBloomHSV : 1;
    uint32_t EnableGhostHSV : 1;
    uint32_t EnableStarHSV : 1;
    uint32_t EnableHiBright : 1;
    uint32_t pad : 12;
    float mManualLuminance;
    float mManualMaxLuminance;
    struct MtColor {
        unsigned char r;
        unsigned char g;
        unsigned char b;
    } mClearColor;
    float mMiddleGray; // 0x30
    float mElapsedTime;
    float mMaxLimit;
    float mMinLimit;
    float mBloomThreshold; // 0x40
    float mBloomOffset;
    float mBloomDispersion;
    int pad4C;
    MtVector4 mBloomColor;
    bool mBloomLuminanceThreshold;
    float mStarThreshold;
    float mStarOffset;
    int pad6C;
    MtVector4 mStarColor;
    bool mStarLuminanceThreshold; // 0x80
    float mGhostThreshold;
    int pad88[2];
    MtVector4 mGhostColor;
    float mGhostOffset;
    bool mGhostLuminanceThreshold;
    bool mCompatibleMode; // A5
    bool mToneMapActive;
    uint32_t mAdaptedLuminance;
    uint32_t mPerFrame;
    uint32_t mCountFrame;
    int padB4[3];
    MtVector3 mInfiniteDir;    // C0
    uintptr_t mpRTScaledScene; // D0
    uintptr_t mpRTAverage[4];  // D4
    uintptr_t mpRTAdaptedLum[2]; // E4
    uintptr_t mpRTBrightPass;  // EC
    uintptr_t mpRTStar;
    uintptr_t mpRTGhost;
    uintptr_t mpRTGlareTemp;
    uintptr_t mpRTGlareTemp2;
    uintptr_t mpGhostMask; // 100
    uint32_t ClearAllRT;   // 108
    uint32_t mGlareType;
    struct GlareDef {
        uint32_t mGlareType;
        float mBloomLuminance;
        float mGhostLuminance;
        float mGhostDistortion;
        float mStarLuminance;
        float mStarInclination;
        float mStarLength;
        float mChromaticAberration;
        float mAfterImageSensitivity;
        float mAfterImageRatio;
        struct StarDef {
            uint32_t mStarType;
            uint32_t mNumStarLines;
            uint32_t nPasses[8];
            float fSampleLength[8];
            float fAttenuation[8];
            float fInclination[8];
            bool mRotation;
            MtVector4 mChromaticAberrationColor[8];
        } mStarDef;
    } mGlareDef;
    struct GhostDef {
        MtVector4 mSampleWeights[8];
        float mSampleScales[8];
    } mGhostDef;
    char pad2F0[32];
};
static_assert(sizeof(uToneMapFilter) == 0x300);

//struct MtHermiteCurve {
//    float x[8];
//    float y[8];
//};

struct uColorCorrectFilter : uFilter {
    struct Corrector {
        bool mEnable;
        uint32_t type;
        uint32_t pad[2];
        MtVector3 mFactor;
        MtVector3 mFactor2;
    } mCorrector[16];
    // 320
    MtMatrix mMatrix;
    MtHermiteCurve ToneCurveR;
    MtHermiteCurve ToneCurveG;
    MtHermiteCurve ToneCurveB;
    MtVector3 mGamma;
    uintptr_t mpToneMap;
    char pad[12];
};
static_assert(sizeof(uColorCorrectFilter) == 0x440);

struct uColorSpaceFilter : uFilter {
    MtVector4 mAngle;
    MtVector4 mScale;
    MtMatrix mMatrix;
};
static_assert(sizeof(uColorSpaceFilter) == 0x80);

struct uLevelCorrectFilter : uFilter {
    MtVector4 mInBlack;
    MtVector4 mInWhite;
    MtVector4 mInputGamma;
    MtVector4 mOutBlack;
    MtVector4 mOutWhite;
};
static_assert(sizeof(uLevelCorrectFilter) == 0x70);

struct uContrastFilter : uFilter {
    float mContrast;
};
static_assert(sizeof(uContrastFilter) == 0x24);

struct uHSVFilter : uFilter {
    float mShiftHue;
    float mShiftSaturation;
    float mShiftValue;
};
static_assert(sizeof(uHSVFilter) == 0x2C);
#endif