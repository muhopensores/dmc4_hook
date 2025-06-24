#include "PhotoMode.hpp"
#include "../sdk/Devil4.hpp"

bool PhotoMode::mod_enabled = false;

// Filter
static uintptr_t uDOFFilterCons             = 0x0091F680;
static uintptr_t uColorSpaceFilterCons      = 0x0091D850;
static uintptr_t uHazeFilterCons            = 0x00933120;
static uintptr_t uLevelCorrectFilterCons    = 0x0091EA50;
static uintptr_t uFishEyeFilterCons         = 0x00934E30;
static uintptr_t uToneMapFilterCons         = 0x0092C7E0;
static uintptr_t uVolumeNoiseFilterCons     = 0x00926A80;
static uintptr_t uTVNoiseFilterCons         = 0x00922290;
static uintptr_t uMotionBlurFilterCons      = 0x0092A180;
static uintptr_t uHSVFilterCons             = 0x0091CF40;
static uintptr_t uImagePlaneFilterCons      = 0x00923BC0;
static uintptr_t uContrastFilterCons        = 0x00936D00;
static uintptr_t uColorCorrectFilterCons    = 0x0093D9A0;
static uintptr_t uLightScatteringFilterCons = 0x0093F570;
static uintptr_t uToneCurveFilterCons       = 0x0091D430;
static uintptr_t uRadialBlurFilterCons      = 0x00928C60;
static uintptr_t uFeedbackBlurFilterCons    = 0x0092BE90;
static uintptr_t uColorFogFilterCons        = 0x009352B0;
static uintptr_t uCrossFadeFilterCons       = 0x00926450;
static uintptr_t uNoiseFogFilterCons        = 0x00927C20;
static uintptr_t uBlurFilterCons            = 0x00935F50;

// Light
static uintptr_t uPointLightCons          = 0x00A6AEA0;
static uintptr_t uSynchroSpotLightCons    = 0x00A705E0;
static uintptr_t uInfiniteLightCons       = 0x00A6AD70;
static uintptr_t uSynchroPointLightCons   = 0x00A704F0;
static uintptr_t uPointShadowLightCons    = 0x00AA0690;
static uintptr_t uPointCubeMapLightCons   = 0x00A6D140;
static uintptr_t uShadowLightCons         = 0x00B2C9A0;
static uintptr_t uCapsuleCubeMapLightCons = 0x00A6D7B0;
static uintptr_t uCuboidCubeMapLightCons  = 0x00A6DE30;
static uintptr_t uSpotLightCons           = 0x00A6B4B0;
static uintptr_t uBackLightCons           = 0x00A703D0;
static uintptr_t uCapsuleShadowLightCons  = 0x00AA14F0;
static uintptr_t uCubeMapLightCons        = 0x00A6CB60;
static uintptr_t uGridEnvLightCons        = 0x00ACB350;
static uintptr_t uHemiSphereLightCons     = 0x00A6C990;
static uintptr_t uCuboidShadowLightCons   = 0x00AA1B20;
static uintptr_t uSpotShadowLightCons     = 0x00AA0AA0;

enum LIGHT_ATTR : unsigned char {
    ATTR_FADEOUT = 1,
    ATTR_SH,
    ATTR_SUBTRACT  = 4,
    ATTR_PERPIXEL  = 8,
    ATTR_VIEWCOORD = 0x10,
    ATTR_DISP      = 0x20,
    ATTR_RANGE     = 0x40,
    ATTR_OOB_RANGE = 0x80
};

enum LIGHT_TYPE : unsigned char { TYPE_NONE, TYPE_INFINITE, TYPE_POINT, TYPE_SPOT, TYPE_AMBIENT, TYPE_CAPSULE, TYPE_CUBOID, MAX_TYPE };

struct uLight : cUnit {
    LIGHT_ATTR mAttr;
    LIGHT_TYPE mType;
    uint16_t mGroup;
    MtVector4 mColor;
};

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

void* call_constructor(void* cons_ptr) {
    void* ret_addr = nullptr;
    _asm {
		pushad
		call cons_ptr
		mov [ret_addr], eax
		popad
    }
    return ret_addr;
}

void sUnit_spawn(void* obj, int move_line) {
    constexpr uintptr_t spawn_call = 0x008DC540;
    uintptr_t sUnit_ptr = (uintptr_t)devil4_sdk::get_sUnit();
    _asm {
        pushad
		mov eax, sUnit_ptr
		mov esi, obj
		push move_line
		call spawn_call
        popad
    }
}

//enum PHOTO_MODE_ENUM : unsigned long {
//    PM_INF_LIGHT,
//    PM_SPOT_LIGHT,
//    PM_POINT_LIGHT,
//    PM_HEMI_LIGHT,
//    PM_BACK_LIGHT,
//    PM_DOF_FILTER,
//    PM_TONE_MAP_FILTER,
//    PM_COLOR_CORR_FILTER,
//    PM_COLOR_SPACE_FILTER,
//    PM_LVL_CORR_FILTER,
//    PM_CONTRAST_FILTER,
//    PM_HSV_FILTER
//};
//
#define PM_INF_LIGHT 0x00EAE3E8
#define PM_SPOT_LIGHT 0x00EAE4C8
#define PM_POINT_LIGHT 0x00EAE4A8
#define PM_HEMI_LIGHT 0x00EAE408
#define PM_BACK_LIGHT 0x00EAE428
#define PM_DOF_FILTER 0x00EADAA0
#define PM_TONE_MAP_FILTER 0x00EADAE0
#define PM_COLOR_CORR_FILTER 0x00EADA20
#define PM_COLOR_SPACE_FILTER 0x00EADBC0
#define PM_LVL_CORR_FILTER 0x00EAD9C0
#define PM_CONTRAST_FILTER 0x00EAD980
#define PM_HSV_FILTER 0x00EAD9A0

void spawn(void* fptr) {
    void* obj = call_constructor(fptr);
    //obj_vec.push_back(std::pair<void*, PHOTO_MODE_ENUM>(obj, mode));
    sUnit_spawn(obj, 17);
}

void PhotoMode::on_gui_frame(int display) {
    ImGui::Checkbox(_("Photo Mode"), &PhotoMode::mod_enabled);
    if (PhotoMode::mod_enabled) {
        if (ImGui::Button("Infinite Light"))
            spawn((void*)uInfiniteLightCons);
        ImGui::SameLine();
        if (ImGui::Button("Spot Light"))
            spawn((void*)uSpotLightCons);

        if (ImGui::Button("Point Light"))
            spawn((void*)uPointLightCons);
        ImGui::SameLine();
        if (ImGui::Button("Hemi Light"))
            spawn((void*)uHemiSphereLightCons);

        if (ImGui::Button("Back Light"))
            spawn((void*)uPointLightCons);
        ImGui::SameLine();
        if (ImGui::Button("DOF Filter"))
            spawn((void*)uDOFFilterCons);

        if (ImGui::Button("Tone Map"))
            spawn((void*)uToneMapFilterCons);
        ImGui::SameLine();
        if (ImGui::Button("Color Correct"))
            spawn((void*)uColorCorrectFilterCons);

        if (ImGui::Button("Color Space"))
            spawn((void*)uColorSpaceFilterCons);
        ImGui::SameLine();
        if (ImGui::Button("Level Correct"))
            spawn((void*)uLevelCorrectFilterCons);
        
        if (ImGui::Button("Contrast"))
            spawn((void*)uContrastFilterCons);
        ImGui::SameLine();
        if (ImGui::Button("HSV"))
            spawn((void*)uHSVFilterCons);

        New_sUnit* s_unit = (New_sUnit*)devil4_sdk::get_sUnit();
        MoveLine* moveline = &s_unit->mMoveLine[17];
        cUnit* obj = moveline->mTop;
        if (obj != nullptr)
            do {
                bool del_flag     = obj->flags.bits.mBeFlag & BEFLAG_DELETE;
                bool pre_del_flag = obj->flags.bits.mBeFlag & BEFLAG_PRE_DELETE;
                if (del_flag != 0 || pre_del_flag != 0) {
                    ImGui::PushID((uintptr_t)obj);
                    ImGui::PushItemWidth(sameLineItemWidth);
                    switch ((uintptr_t)call_constructor(*(void**)((*(uintptr_t**)obj) + 4))) {
                    case PM_INF_LIGHT: {
                        uInfiniteLight* inf_light = (uInfiniteLight*)obj;
                        ImGui::InputFloat4(_("Color"), (float*)&inf_light->mColor);
                        ImGui::InputFloat3(_("Dir"), (float*)&inf_light->mDir);
                        break;
                    }

                    case PM_SPOT_LIGHT: {
                        uSpotLight* spot_light = (uSpotLight*)obj;
                        spot_light->mpTarget   = (uCoord*)devil4_sdk::get_local_player();
                        spot_light->mTargetNo  = 0;
                        ImGui::InputFloat4(_("Color"), (float*)&spot_light->mColor);
                        ImGui::InputFloat3(_("Pos"), (float*)&spot_light->mPos);
                        ImGui::InputFloat3(_("Dir"), (float*)&spot_light->mDir);
                        ImGui::InputFloat3(_("Target Pos"), (float*)&spot_light->mTargetPos);
                        ImGui::InputFloat3(_("World Pos"), (float*)&spot_light->mWPos);
                        ImGui::InputFloat3(_("World Dir"), (float*)&spot_light->mWDir);
                        ImGui::InputFloat(_("Start"), &spot_light->mStart);
                        ImGui::InputFloat(_("End"), &spot_light->mEnd);
                        ImGui::InputFloat(_("Cone"), &spot_light->mCone);
                        ImGui::InputFloat(_("Spread"), &spot_light->mSpread);
                        ImGui::Checkbox(_("Target Mode"), &spot_light->mTargetMode);
                        ImGui::InputFloat(_("mStart"), &spot_light->mStart);
                        break;
                    }

                    case PM_POINT_LIGHT: {
                        uPointLight* point_light = (uPointLight*)obj;
                        ImGui::InputFloat4(_("Color"), (float*)&point_light->mColor);
                        ImGui::InputFloat3(_("Pos"), (float*)&point_light->mPos);
                        ImGui::InputFloat3(_("World Pos"), (float*)&point_light->mWPos);
                        ImGui::InputFloat(_("Start"), &point_light->mStart);
                        ImGui::InputFloat(_("End"), &point_light->mEnd);
                        break;
                    }

                    case PM_HEMI_LIGHT: {
                        uHemiSphereLight* hemi_light = (uHemiSphereLight*)obj;
                        ImGui::InputFloat4(_("Color"), (float*)&hemi_light->mColor);
                        ImGui::InputFloat3(_("RevColor"), (float*)&hemi_light->mRevColor);
                        ImGui::InputFloat3(_("Dir"), (float*)&hemi_light->mDir);
                        break;
                    }

                    case PM_BACK_LIGHT: {
                        uBackLight* back_light = (uBackLight*)obj;
                        ImGui::InputFloat4(_("Color"), (float*)&back_light->mColor);
                        break;
                    }

                    case PM_DOF_FILTER: {
                        uDOFFilter* dof_filter = (uDOFFilter*)obj;
                        ImGui::InputInt(_("Blur count"), (int*)&dof_filter->mBlurCount);
                        ImGui::InputInt(_("Type"), (int*)&dof_filter->mType);
                        ImGui::InputInt(_("Blur type"), (int*)&dof_filter->mBlurType);
                        ImGui::InputFloat3(_("Gradate Color"), (float*)&dof_filter->mGradateColor);
                        ImGui::InputFloat(_("Blur Size"), &dof_filter->mBlurSize);
                        ImGui::InputFloat(_("Aperture"), &dof_filter->mAperture);
                        ImGui::InputFloat(_("Focal Length"), &dof_filter->mFocalLength);
                        ImGui::InputFloat(_("Low CoC Scale"), &dof_filter->mLowCoCScale);
                        ImGui::InputFloat(_("CoC Scale"), &dof_filter->mCocScale);
                        ImGui::InputFloat(_("CoC Bias"), &dof_filter->mCoCBias);
                        ImGui::InputFloat(_("Far Blur Limit"), &dof_filter->mFarBlurLimit);
                        ImGui::InputFloat(_("Near Blur Limit"), &dof_filter->mNearBlurLimit);
                        ImGui::InputFloat(_("Near"), &dof_filter->mNear);
                        ImGui::InputFloat(_("Far"), &dof_filter->mFar);
                        ImGui::InputFloat(_("Focal"), &dof_filter->mFocal);
                        break;
                    }

                    case PM_TONE_MAP_FILTER: {
                        // uToneMapFilter* tone_filter = (uToneMapFilter*)obj;
                        break;
                    }
                
                    case PM_COLOR_CORR_FILTER: {
                        break;
                    }

                    case PM_COLOR_SPACE_FILTER: {
                        uColorSpaceFilter* cs_filter = (uColorSpaceFilter*)obj;
                        ImGui::InputFloat4(_("Angle"), (float*)&cs_filter->mAngle);
                        ImGui::InputFloat4(_("Scale"), (float*)&cs_filter->mScale);
                        break;
                    }

                    case PM_LVL_CORR_FILTER: {
                        uLevelCorrectFilter* lvl_filter = (uLevelCorrectFilter*)obj;
                        ImGui::InputFloat4(_("In Black"), (float*)&lvl_filter->mInBlack);
                        ImGui::InputFloat4(_("In White"), (float*)&lvl_filter->mInWhite);
                        ImGui::InputFloat4(_("Input Gamma"), (float*)&lvl_filter->mInputGamma);
                        ImGui::InputFloat4(_("Out Black"), (float*)&lvl_filter->mOutBlack);
                        ImGui::InputFloat4(_("Out White"), (float*)&lvl_filter->mOutWhite);
                        break;
                    }

                    case PM_CONTRAST_FILTER: { 
                        uContrastFilter* contr_filter = (uContrastFilter*)obj;
                        ImGui::InputFloat(_("Contrast"), &contr_filter->mContrast);
                        break;
                    }

                    case PM_HSV_FILTER: {
                        uHSVFilter* hsv_filter = (uHSVFilter*)obj;
                        ImGui::InputFloat(_("Shift Hue"), &hsv_filter->mShiftHue);
                        ImGui::InputFloat(_("Shift Saturation"), &hsv_filter->mShiftSaturation);
                        ImGui::InputFloat(_("Shift Value"), &hsv_filter->mShiftValue);
                        break;
                    }
                    }
                    if (ImGui::Button("Remove")) {
                        obj->flags.bits.mBeFlag = 3;
                    }
                    ImGui::PopItemWidth();
                    ImGui::PopID();
                }
                obj = obj->mp_next_unit;
            } while (obj != nullptr);
    }
}