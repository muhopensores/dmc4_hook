#pragma once

#include "cUnit.hpp"
#include "MtMath.hpp"
#include <cstdint>

enum CcType : uint32_t //NOLINT
{
    TYPE_UNUSED = 0,
    TYPE_HUE = 1,
    TYPE_CONTRAST = 2,
    TYPE_CONTRAST2 = 3,
    TYPE_CHROMA = 4,
    TYPE_CHROMA2 = 5,
    TYPE_BRIGHTNESS = 6,
    TYPE_BRIGHTNESS2 = 7,
    TYPE_SCALE = 8,
    TYPE_SCALE2 = 9,
    TYPE_INPUTLEVEL = 10,
    TYPE_INPUTLEVEL2 = 11,
    TYPE_OUTPUTLEVEL = 12,
    TYPE_OUTPUTLEVEL2 = 13,
    TYPE_SEPIA = 14,
    TYPE_SEPIA2 = 15,
    TYPE_NEGA = 16,
    TYPE_NEGA2 = 17,
    TYPE_GRAYSCALE = 18,
    TYPE_RREPLACE = 19,
    TYPE_GREPLACE = 20,
    TYPE_BREPLACE = 21,
    TYPE_SEPIA3 = 22
}; //0x0008

enum DOF_TYPE : unsigned long { DOF_DETAIL = 0, DOF_SIMPLE = 1, DOF_SIMPLE2 = 2 };

enum BLUR_TYPE : unsigned long { BLUR_GAUSSIAN = 1, BLUR_MASKGAUSSIAN = 2 };

#pragma pack(push, r2, 4)

struct uFilter : cUnit {
    uint32_t mPass;
    uint32_t mPriority;
};
static_assert(sizeof(uFilter) == 0x20);

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
    MtVector3 mInfiniteDir;      // C0
    uintptr_t mpRTScaledScene;   // D0
    uintptr_t mpRTAverage[4];    // D4
    uintptr_t mpRTAdaptedLum[2]; // E4
    uintptr_t mpRTBrightPass;    // EC
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

// struct MtHermiteCurve {
//     float x[8];
//     float y[8];
// };

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
#pragma pack(pop, r2)

class uTvNoiseFilter : public uFilter {
public:
    Vector3f m_noisy_uv_offset;      // 0x0020
    char pad_002_c[8];               // 0x002C
    float m_noise_power;             // 0x0034
    float m_noise_power_croma;       // 0x0038
    float m_scale_y;                 // 0x003C
    float m_scale_cr_cb;             // 0x0040
    uint32_t m_scanline_size;        // 0x0044
    float m_scanline_alpha;          // 0x0048
    uint32_t noise_texture_ptr;      // 0x004C
    uint32_t mp_tv_mask_texture_ptr; // 0x0050
    bool m_enable_blank_scan;        // 0x0054
    char pad_0055[3];                // 0x0055
    float m_blank_ofs_y;             // 0x0058
    float m_blank_speed;             // 0x005C
    uint8_t m_blank_alpha;           // 0x0060
    char pad_0061[3];                // 0x0061
    float m_blank_size;              // 0x0064
    float m_shock_noise_freq;        // 0x0068
    float m_h_sync_noise_amplitude;  // 0x006C
    float m_v_sync_noise_amplitude;  // 0x0070
    float m_shock_detail_reduction;  // 0x0074
    float m_h_sync_blank_width;      // 0x0078
    float m_v_sync_blank_width;      // 0x007C
    Vector4 m_v_sync_blank_color;    // 0x0080
    uint32_t m_shock_noise_time;     // 0x0090
    float m_h_sync_noise_value;      // 0x0094
    float m_v_sync_noise_value;      // 0x0098
    float m_h_sync_lag;              // 0x009C
    float m_v_sync_lag;              // 0x00A0
    float m_detail_reduction;        // 0x00A4
    float m_h_sync_noise_offset;     // 0x00A8
    char pad_00_ac[1992];            // 0x00AC
}; // Size: 0x0874
static_assert(sizeof(uTvNoiseFilter) == 0x0874);
