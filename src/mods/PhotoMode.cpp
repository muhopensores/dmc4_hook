#include "PhotoMode.hpp"
#include "DebugCam.hpp"
#include "CameraSettings.hpp"
#include "../sdk/Devil4.hpp"
#include <Mods.hpp>
#include "utility/Hash.hpp"
#include "../sdk/World2Screen.hpp"

bool PhotoMode::mod_enabled = false;
bool PhotoMode::photo_mode_open = false;

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

static void* call_constructor(void* cons_ptr) {
    void* ret_addr = nullptr;
    _asm {
		pushad
		call cons_ptr
		mov [ret_addr], eax
		popad
    }
    return ret_addr;
}

static void sUnit_spawn(void* obj, int move_line) {
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

// yea we should make these public somewhere

#include "../sdk/uActor.hpp"

typedef uActorMain::uFreeCamera uFreeCamera;
typedef uActorMain::uCamera uCamera;
static void* (__stdcall*freecam_cons)() = (void*(__stdcall*)())0x9197A0;

struct MtRect {
    uint32_t l;
    uint32_t t;
    uint32_t r;
    uint32_t b;
};

struct MtColor {
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t a;
};

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
    struct uActorMain::MtVector4 mFrustum[6];
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
    struct uActorMain::MtVector4 mFogPlaneDir;
    struct uActorMain::MtVector4 mFogColor;
    struct uActorMain::MtVector4 UknVecs1[2];
    struct uActorMain::MtVector4 UknVecs2[2];
    struct uActorMain::MtVector4 mTransFogFactor[2];
    struct uActorMain::MtVector4 mTransFogColor[2];
    MtMatrix mTransViewMat[2];
    MtMatrix mTransProjMat[2];
    MtMatrix mTransViewProjMat[2];
    MtMatrix mTransPrevViewMat[2];
    MtMatrix mTransPrevProjMat[2];
    MtMatrix mTransPrevViewProjMat[2];
    struct uActorMain::MtVector4 mTransEyePos[2];
};
static_assert(sizeof(sCamera_ViewPort) == 0x590);

struct sCamera: CSystem {
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

static sCamera* get_sCamera() {
    uintptr_t sMain = 0x00E5574C;
    sCamera* ptr    = *(sCamera**)(*(uintptr_t*)sMain + 0x10358);
    return ptr;
}

static void set_viewport(uint32_t index, REGION_MODE mode, uintptr_t CameraPtr) {
    sCamera* sCam = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    viewport->mpCamera = (void*)CameraPtr;
    viewport->mMode = mode;
    viewport->mAttr = 0x17;
}

//

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

static void spawn(void* fptr) {
    void* obj = call_constructor(fptr);
    //obj_vec.push_back(std::pair<void*, PHOTO_MODE_ENUM>(obj, mode));
    sUnit_spawn(obj, 17);
}

static void SetGameSpeeds(float speed) {
    sWorkRate* wr = devil4_sdk::get_work_rate();
    if (!wr) { return; }
    wr->enemy_speed = speed;
    wr->player_speed = speed;
    wr->room_speed = speed;
}

static sCamera_ViewPort* get_viewport(uint32_t index) {
    sCamera* sCam              = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    return viewport;
}

static void ToggleGameplayCam(bool enable) {
    uFreeCamera* cam = (uFreeCamera*)freecam_cons();
    /*if (freecamMouseControls)*/ cam->mControlPad = 0;
    devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)cam, 0x17);
    sCamera_ViewPort* first_vp = get_viewport(0);
    set_viewport(1, REGION_FULLSCREEN, (uintptr_t)cam);
    cam->uCameraBase.mCameraPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraPos;
    cam->uCameraBase.mTargetPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mTargetPos;
    cam->uCameraBase.mCameraUp = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraUp;

    sCamera_ViewPort* vp = get_viewport(0);
    vp->mActive = enable;
}

static void DisablePlayerInputs(bool enable) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    player->pad_1e80[2] = enable;
}

void PhotoMode::on_gui_frame(int display) {
    ImGui::Checkbox(_("Spawn Filters And Lights"), &PhotoMode::mod_enabled);
    if (PhotoMode::mod_enabled) {
        ImGui::Indent(lineIndent);
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

        sUnit* s_unit = (sUnit*)devil4_sdk::get_sUnit();
        MoveLine* moveline = &s_unit->mMoveLine[17];
        cUnit* obj = moveline->mTop;
        while (obj != nullptr) {
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
        }
        ImGui::Unindent(lineIndent);
    }
    if (ImGui::Checkbox(_("Photo Mode"), &PhotoMode::photo_mode_open)) {
        if (photo_mode_open) {
            SetGameSpeeds(0.0f);
        } else {
            SetGameSpeeds(1.0f);
            DebugCam::toggle_gameplay_cam = true;
            ToggleGameplayCam(DebugCam::toggle_gameplay_cam);
        }
        DebugCam::disable_player_inputs = photo_mode_open;
        DisablePlayerInputs(DebugCam::disable_player_inputs);
    }
}

static int selectedLightIndex = -1;
static bool isManipulatingLight = false;
static ImGuizmo::OPERATION currentLightGizmoOperation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE currentLightGizmoMode = ImGuizmo::WORLD;

static void ImGuizmoManipulators() {
    w2s::ImGuizmoSetup();
    float view[16], projection[16];
    if (!w2s::GetImGuizmoMatrices(view, projection)) {
        return;
    }
    
    sUnit* s_unit = (sUnit*)devil4_sdk::get_sUnit();
    MoveLine* moveline = &s_unit->mMoveLine[17];
    cUnit* obj = moveline->mTop;
    int lightIndex = 0;
    
    while (obj != nullptr) {
        bool del_flag = obj->flags.bits.mBeFlag & BEFLAG_DELETE;
        bool pre_del_flag = obj->flags.bits.mBeFlag & BEFLAG_PRE_DELETE;
        
        if (del_flag != 0 || pre_del_flag != 0) {
            uintptr_t objectType = (uintptr_t)call_constructor(*(void**)((*(uintptr_t**)obj) + 4));
            
            glm::vec3 lightPos;
            bool hasPosition = false;
            ImU32 lightColor = IM_COL32(255, 255, 0, 200); // yellow
            
            switch (objectType) {
                case PM_SPOT_LIGHT: {
                    uSpotLight* spot_light = (uSpotLight*)obj;
                    lightPos = glm::vec3(spot_light->mWPos.x, spot_light->mWPos.y, spot_light->mWPos.z);
                    lightColor = IM_COL32(255, 255, 0, 200); // yellow
                    hasPosition = true;
                    
                    glm::vec3 newPos;
                    if (w2s::DrawImGuizmoManipulator(lightPos, newPos, lightIndex, selectedLightIndex, isManipulatingLight,
                                        currentLightGizmoOperation, currentLightGizmoMode, view, projection,
                                        lightColor, 8.0f, "SpotLight")) {
                        spot_light->mPos.x = newPos.x;
                        spot_light->mPos.y = newPos.y;
                        spot_light->mPos.z = newPos.z;
                    }
                    break;
                }
                
                case PM_POINT_LIGHT: {
                    uPointLight* point_light = (uPointLight*)obj;
                    lightPos = glm::vec3(point_light->mWPos.x, point_light->mWPos.y, point_light->mWPos.z);
                    lightColor = IM_COL32(255, 200, 100, 200); // orange
                    hasPosition = true;
                    
                    glm::vec3 newPos;
                    if (w2s::DrawImGuizmoManipulator(lightPos, newPos, lightIndex, selectedLightIndex, isManipulatingLight,
                                        currentLightGizmoOperation, currentLightGizmoMode, view, projection,
                                        lightColor, 8.0f, "PointLight")) {
                        point_light->mPos.x = newPos.x;
                        point_light->mPos.y = newPos.y;
                        point_light->mPos.z = newPos.z;
                    }
                    break;
                }
            }
        }
        
        lightIndex++;
        obj = obj->mp_next_unit;
    }
    ImGuizmo::MODE currentImGuizmode = ImGuizmo::WORLD; // force world
    w2s::ImGuizmoKeyboardShortcuts(currentLightGizmoOperation, currentImGuizmode);
    w2s::ImGuizmoDeselection(selectedLightIndex);
}

void DisplayLightGuizmoControls() {
    ImGui::Text("Light Manipulator Controls:");
    ImGui::Text("G - Translate, R - Rotate, S - Scale");
    
    if (ImGui::RadioButton("Translate", currentLightGizmoOperation == ImGuizmo::TRANSLATE)) {
        currentLightGizmoOperation = ImGuizmo::TRANSLATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Rotate", currentLightGizmoOperation == ImGuizmo::ROTATE)) {
        currentLightGizmoOperation = ImGuizmo::ROTATE;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Scale", currentLightGizmoOperation == ImGuizmo::SCALE)) {
        currentLightGizmoOperation = ImGuizmo::SCALE;
    }
    
    if (selectedLightIndex >= 0) {
        ImGui::Text("Selected Light Index: %d", selectedLightIndex);
    } else {
        ImGui::Text("No light selected");
    }
}

void PhotoMode::on_frame(fmilliseconds& dt) {
    if (photo_mode_open) {
        uPlayer* player = devil4_sdk::get_local_player();
        ImVec2 ScreenSize = ImGui::GetIO().DisplaySize;
        static float HUDCooldown = 0.0f;
        if (HUDCooldown > 0.0f) {
            /*if (HUDCooldown <= 1.0f) {
                unhide hud here
            }*/
            HUDCooldown -= 1.0f;
        }
        if (player && HUDCooldown <= 0.0f) {
            ImGui::Begin("Photo Mode##UI", &photo_mode_open);
            ImGuizmoManipulators();
            DisplayLightGuizmoControls();
            static bool pauseGame = true;
            ImGui::SetWindowPos(ImVec2(ScreenSize.x * 0.6f, ScreenSize.y * 0.5f), ImGuiCond_Once);
            ImGui::SetWindowSize(ImVec2(ScreenSize.x * 0.4f, ScreenSize.y * 0.5f), ImGuiCond_Once);

            if (ImGui::Button("Hide Photo Mode UI")) {
                HUDCooldown = 300.0f;
                /*hide hud here*/
            }
            ImGui::SameLine();
            help_marker("Hide UI for a few seconds so you have time to take a screenshot");

            static float speedSlider = 0.0f;
            if (ImGui::Checkbox(_("Pause Game"), &pauseGame)) {
                if (pauseGame) {
                    speedSlider = 0.0f;
                }
                else {
                    speedSlider = 1.0f;
                }
            }
            if (pauseGame) {
                speedSlider = 0.0f;
                SetGameSpeeds(speedSlider);
            }
            else {
                speedSlider = 1.0f;
                SetGameSpeeds(speedSlider);
            }
            ImGui::SameLine();
            if (!pauseGame) ImGui::BeginDisabled();
            if (ImGui::Button(_("Frame Advance"))) {
                SetGameSpeeds(1.0f);
            }
            if (!pauseGame) ImGui::EndDisabled();
            ImGui::SameLine();

            ImGui::PushItemWidth(sameLineItemWidth);

            if (ImGui::SliderFloat(_("Speed"), &speedSlider, 0.0f, 2.0f)) {
                SetGameSpeeds(speedSlider);
            }
            ImGui::SameLine();
            ImGui::SliderFloat(_("Player Anim Frame"), &player->animFrame, 0.0f, player->animFrameMax);

            ImGui::PopItemWidth();

            if (ImGui::BeginTabBar("PhotoTabBar", ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll)) {
                if (ImGui::BeginTabItem(_("Camera"))) {

                    ImGui::SeparatorText(_("Camera"));
                    g_framework->get_mods().get()->on_draw_ui("CameraSettings"_hash, 3);
                    ImGui::SameLine();
                    g_framework->get_mods().get()->on_draw_ui("DebugCam"_hash, 2);

                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(_("Lights/Filters"))) {
                    ImGui::SeparatorText(_("Lights"));

                    if (ImGui::Button("Infinite Light"))
                        spawn((void*)uInfiniteLightCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Spot Light"))
                        spawn((void*)uSpotLightCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Point Light"))
                        spawn((void*)uPointLightCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Hemi Light"))
                        spawn((void*)uHemiSphereLightCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Back Light"))
                        spawn((void*)uPointLightCons);

                    ImGui::SeparatorText(_("Filters"));

                    if (ImGui::Button("DOF Filter"))
                        spawn((void*)uDOFFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Tone Map"))
                        spawn((void*)uToneMapFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Color Correct"))
                        spawn((void*)uColorCorrectFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Color Space"))
                        spawn((void*)uColorSpaceFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Level Correct"))
                        spawn((void*)uLevelCorrectFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("Contrast"))
                        spawn((void*)uContrastFilterCons);
                    ImGui::SameLine();
                    if (ImGui::Button("HSV"))
                        spawn((void*)uHSVFilterCons);

                    sUnit* s_unit = (sUnit*)devil4_sdk::get_sUnit();
                    MoveLine* moveline = &s_unit->mMoveLine[17];
                    cUnit* obj = moveline->mTop;
                    int numOfObjs = 0;
                    if (obj != nullptr) ImGui::SeparatorText(_("Added Objects"));
                    while (obj != nullptr) {
                        bool del_flag     = obj->flags.bits.mBeFlag & BEFLAG_DELETE;
                        bool pre_del_flag = obj->flags.bits.mBeFlag & BEFLAG_PRE_DELETE;
                        if (del_flag != 0 || pre_del_flag != 0) {
                            ImGui::BeginGroup();
                            ImGui::PushItemWidth(sameLineItemWidth);
                            ImGui::PushID((uintptr_t)obj);

                            switch ((uintptr_t)call_constructor(*(void**)((*(uintptr_t**)obj) + 4))) {
                            case PM_INF_LIGHT: {
                                uInfiniteLight* inf_light = (uInfiniteLight*)obj;
                                ImGui::Text("Infinite Light");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("Color"), (float*)&inf_light->mColor);
                                ImGui::InputFloat3(_("Dir"), (float*)&inf_light->mDir);
                                break;
                            }

                            case PM_SPOT_LIGHT: {
                                uSpotLight* spot_light = (uSpotLight*)obj;
                                spot_light->mpTarget   = (uCoord*)devil4_sdk::get_local_player();
                                spot_light->mTargetNo  = 0;
                                ImGui::Text("Spot Light");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
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
                                ImGui::Text("Point Light");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("Color"), (float*)&point_light->mColor);
                                ImGui::InputFloat3(_("Pos"), (float*)&point_light->mPos);
                                ImGui::InputFloat3(_("World Pos"), (float*)&point_light->mWPos);
                                ImGui::InputFloat(_("Start"), &point_light->mStart);
                                ImGui::InputFloat(_("End"), &point_light->mEnd);
                                break;
                            }

                            case PM_HEMI_LIGHT: {
                                uHemiSphereLight* hemi_light = (uHemiSphereLight*)obj;
                                ImGui::Text("Hemi Light");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("Color"), (float*)&hemi_light->mColor);
                                ImGui::InputFloat3(_("RevColor"), (float*)&hemi_light->mRevColor);
                                ImGui::InputFloat3(_("Dir"), (float*)&hemi_light->mDir);
                                break;
                            }

                            case PM_BACK_LIGHT: {
                                uBackLight* back_light = (uBackLight*)obj;
                                ImGui::Text("Back Light");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("Color"), (float*)&back_light->mColor);
                                break;
                            }

                            case PM_DOF_FILTER: {
                                uDOFFilter* dof_filter = (uDOFFilter*)obj;
                                ImGui::Text("DOF Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
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
                                ImGui::Text("Tone Map Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                // uToneMapFilter* tone_filter = (uToneMapFilter*)obj;
                                break;
                            }
    
                            case PM_COLOR_CORR_FILTER: {
                                ImGui::Text("Colour Correct Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                break;
                            }

                            case PM_COLOR_SPACE_FILTER: {
                                uColorSpaceFilter* cs_filter = (uColorSpaceFilter*)obj;
                                ImGui::Text("Colour Space Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("Angle"), (float*)&cs_filter->mAngle);
                                ImGui::InputFloat4(_("Scale"), (float*)&cs_filter->mScale);
                                break;
                            }

                            case PM_LVL_CORR_FILTER: {
                                uLevelCorrectFilter* lvl_filter = (uLevelCorrectFilter*)obj;
                                ImGui::Text("Level Correct Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat4(_("In Black"), (float*)&lvl_filter->mInBlack);
                                ImGui::InputFloat4(_("In White"), (float*)&lvl_filter->mInWhite);
                                ImGui::InputFloat4(_("Input Gamma"), (float*)&lvl_filter->mInputGamma);
                                ImGui::InputFloat4(_("Out Black"), (float*)&lvl_filter->mOutBlack);
                                ImGui::InputFloat4(_("Out White"), (float*)&lvl_filter->mOutWhite);
                                break;
                            }

                            case PM_CONTRAST_FILTER: { 
                                uContrastFilter* contr_filter = (uContrastFilter*)obj;
                                ImGui::Text("Contrast Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat(_("Contrast"), &contr_filter->mContrast);
                                break;
                            }

                            case PM_HSV_FILTER: {
                                uHSVFilter* hsv_filter = (uHSVFilter*)obj;
                                ImGui::Text("HSV Filter");
                                if (ImGui::Button("Remove")) obj->flags.bits.mBeFlag = 3;
                                ImGui::InputFloat(_("Shift Hue"), &hsv_filter->mShiftHue);
                                ImGui::InputFloat(_("Shift Saturation"), &hsv_filter->mShiftSaturation);
                                ImGui::InputFloat(_("Shift Value"), &hsv_filter->mShiftValue);
                                break;
                            }
                            }
                            ImGui::PopItemWidth();
                            ImGui::PopID();
                            ImGui::EndGroup();
                        }
                        numOfObjs++;
                        obj = obj->mp_next_unit;
                        if (numOfObjs % 3 != 0) {
                            ImGui::SameLine();
                        }
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
    }
}