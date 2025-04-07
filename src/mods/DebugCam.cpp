#include "DebugCam.hpp"
#include "../sdk/ReClass.hpp"
#include "../sdk/uActor.hpp"
#include "../sdk/Devil4.hpp"

typedef uActorMain::uFreeCamera uFreeCamera;
typedef uActorMain::uCamera uCamera;
bool DebugCam::mod_enabled{false};
//constexpr uintptr_t freecam_cons { 0x9197A0 };
void* (__stdcall*freecam_cons)() = (void*(__stdcall*)())0x9197A0;
uintptr_t DebugCam::jmp_ret1 { NULL };

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

sCamera* get_sCamera() {
    uintptr_t sMain = 0x00E5574C;
    sCamera* ptr    = *(sCamera**)(*(uintptr_t*)sMain + 0x10358);
    return ptr;
}

void* get_sMouse() {
    uintptr_t sMain = 0x00E5574C;
    void* ptr    = (void*)(*(uintptr_t*)sMain + 0x10384);
    return ptr;
}

void set_viewport(uint32_t index, REGION_MODE mode, uintptr_t CameraPtr) {
    sCamera* sCam = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    viewport->mpCamera = (void*)CameraPtr;
    viewport->mMode = mode;
    viewport->mAttr = 0x17;
}

sCamera_ViewPort* get_viewport(uint32_t index) {
    sCamera* sCam              = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    return viewport;
}

void __stdcall freecam_mouse_input(uCamera* camera) {
    uintptr_t sMouse = (uintptr_t)get_sMouse();
    int x_diff = *(int*)(*(uintptr_t*)sMouse + 0x2C);
    int y_diff = *(int*)(*(uintptr_t*)sMouse + 0x30);
    Vector3f cam_pos = *(Vector3f*)&camera->mCameraPos;
    Vector3f cam_up = *(Vector3f*)&camera->mCameraUp;
    Vector3f cam_lookat = *(Vector3f*)&camera->mTargetPos;

    glm::mat4 viewMatrix = glm::lookAt(cam_pos, cam_lookat, cam_up);
    glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), (float)(y_diff * 0.001), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), (float)(x_diff * 0.001), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotateZ    = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    if (short input = VkKeyScan('Q'); devil4_sdk::internal_kb_check(input & 0xFF))
        rotateZ = glm::rotate(glm::mat4(1.0f), -0.005f, glm::vec3(0.0f, 0.0f, 1.0f));
    if (short input = VkKeyScan('E'); devil4_sdk::internal_kb_check(input & 0xFF))
        rotateZ = glm::rotate(glm::mat4(1.0f), 0.005f, glm::vec3(0.0f, 0.0f, 1.0f));
    viewMatrix = rotateZ * viewMatrix;
    viewMatrix = glm::translate(viewMatrix, cam_pos);
    viewMatrix = viewMatrix * rotateY;
    viewMatrix = glm::translate(viewMatrix, -cam_pos);
    viewMatrix = rotateX * viewMatrix;
    glm::mat4 translateMat(1.0f);
    float speed = 1.0f;
    if (devil4_sdk::internal_kb_check(VK_SHIFT))
        speed = 10.0f;
    if (short input = VkKeyScan('W'); devil4_sdk::internal_kb_check(input & 0xFF))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, speed));
    if (short input = VkKeyScan('S'); devil4_sdk::internal_kb_check(input & 0xFF))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -speed));
    if (short input = VkKeyScan('A'); devil4_sdk::internal_kb_check(input & 0xFF))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(speed, 0.0f, 0.0f));
    if (short input = VkKeyScan('D'); devil4_sdk::internal_kb_check(input & 0xFF))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(-speed, 0.0f, 0.0f));
    if (devil4_sdk::internal_kb_check(VK_SPACE))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -speed, 0.0f));
    if (devil4_sdk::internal_kb_check(VK_CONTROL))
        translateMat *= glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, speed, 0.0f));
    viewMatrix = translateMat * viewMatrix;
    glm::vec4 target = glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, -700.0f, 1.0f);
    glm::vec4 up = glm::inverse(viewMatrix) * glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    glm::vec4 pos = glm::inverse(viewMatrix) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    up = glm::normalize(up - pos);
    camera->mCameraPos.x = pos.x;
    camera->mCameraPos.y = pos.y;
    camera->mCameraPos.z = pos.z;
    camera->mTargetPos.x = target.x;
    camera->mTargetPos.y = target.y;
    camera->mTargetPos.z = target.z;
    camera->mCameraUp.x = up.x;
    camera->mCameraUp.y = up.y;
    camera->mCameraUp.z = up.z;
}

naked void detour1(void) {
    _asm {
        pushad
        push esi
        call freecam_mouse_input
        popad
    //originalcode:
        mov eax,[esi+0x000000C0]
        jmp [DebugCam::jmp_ret1]
    }
}

std::optional<std::string> DebugCam::on_initialize() {
    if (!install_hook_offset(0x519E60, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init DebugCam mod\n");
        return "Failed to init DebugCam mod";
    }
    return Mod::on_initialize();
}

struct ViewportState {
    int selectedViewport = 1;
    REGION_MODE selectedViewportOption = REGION_FULLSCREEN;
    int selectedViewportOptionComboIndex = 0;
};

ViewportState states[2];

void DebugCam::on_gui_frame() {
    int numOfViewports = IM_ARRAYSIZE(states);
    for (int i = 0; i < numOfViewports; i++) {
        ImGui::PushID(i);
        ViewportState& state = states[i];
        
        if (i == 0) {
            ImGui::Text("Gameplay Cam");
            ImGui::SetNextItemWidth(sameLineItemWidth);
            if (ImGui::Combo("## Location", &state.selectedViewportOptionComboIndex, validViewportsComboStr)) {
                sCamera_ViewPort* first_vp = get_viewport(0);
                first_vp->mMode = state.selectedViewportOptionComboIndex;
            }
            ImGui::SameLine();
            help_marker("This is set automatically when setting the extra cam\nIf you want to override it, set this last");
            if (ImGui::Button("Reset")) {
                sCamera_ViewPort* first_vp = get_viewport(0);
                state.selectedViewportOptionComboIndex = REGION_FULLSCREEN;
                first_vp->mMode = state.selectedViewportOptionComboIndex;
            }
        }
        else {
            ImGui::Text("Free Cam %i:", i);
            float buttonSize = 50.0f;
            float spacing = 5.0f;

            struct RegionButton {
                REGION_MODE mode;
                const char* label;
            };

            RegionButton regionButtons[3][3] = {
                {
                    { REGION_TOPLEFT, "Top\nLeft" },
                    { REGION_TOP, "Top" },
                    { REGION_TOPRIGHT, "Top\nRight" }
                },
                {
                    { REGION_LEFT, "Left" },
                    { REGION_FULLSCREEN, "Full" },
                    { REGION_RIGHT, "Right" }
                },
                {
                    { REGION_BOTTOMLEFT, "Bottom\nLeft" },
                    { REGION_BOTTOM, "Bottom" },
                    { REGION_BOTTOMRIGHT, "Bottom\nRight" }
                }
            };

            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    RegionButton& rb = regionButtons[row][col];
                    ImGui::PushStyleColor(ImGuiCol_Button, 
                        (state.selectedViewportOption == rb.mode) ? 
                        ImVec4(0.4f, 0.6f, 0.8f, 1.0f) : ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
                    if (ImGui::Button(rb.label, ImVec2(buttonSize, buttonSize))) {
                        state.selectedViewportOption = rb.mode;
                        state.selectedViewportOptionComboIndex = state.selectedViewportOption;
                        ViewportState& gameplayViewportOption = states[0];
                        switch(state.selectedViewportOption) {
                            case REGION_LEFT:
                                gameplayViewportOption.selectedViewportOption = REGION_RIGHT;
                                break;
                            case REGION_RIGHT:
                                gameplayViewportOption.selectedViewportOption = REGION_LEFT;
                                break;
                            case REGION_TOP:
                                gameplayViewportOption.selectedViewportOption = REGION_BOTTOM;
                                break;
                            case REGION_BOTTOM:
                                gameplayViewportOption.selectedViewportOption = REGION_TOP;
                                break;
                            case REGION_TOPLEFT:
                                gameplayViewportOption.selectedViewportOption = REGION_BOTTOMRIGHT;
                                break;
                            case REGION_TOPRIGHT:
                                gameplayViewportOption.selectedViewportOption = REGION_BOTTOMLEFT;
                                break;
                            case REGION_BOTTOMLEFT:
                                gameplayViewportOption.selectedViewportOption = REGION_TOPRIGHT;
                                break;
                            case REGION_BOTTOMRIGHT:
                                gameplayViewportOption.selectedViewportOption = REGION_TOPLEFT;
                                break;
                            case REGION_FULLSCREEN:
                                gameplayViewportOption.selectedViewportOption = REGION_TOPRIGHT;
                                break;
                            default:
                                gameplayViewportOption.selectedViewportOption = REGION_FULLSCREEN;
                                break;
                        }
                        gameplayViewportOption.selectedViewportOptionComboIndex = gameplayViewportOption.selectedViewportOption;
            
                        // make a new cam every click because idk how to track active cams
                        uFreeCamera* cam = (uFreeCamera*)freecam_cons();
                        devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)cam, 0x17);
                        set_viewport(state.selectedViewport, state.selectedViewportOption, (uintptr_t)cam);

                        // update gameplay cam
                        sCamera_ViewPort* first_vp = get_viewport(0);
                        first_vp->mMode = states[0].selectedViewportOption;
                    }
                    ImGui::PopStyleColor();
                    if (col < 2) ImGui::SameLine(0, spacing);
                }
            }
        }
        ImGui::PopID();
    }
}

// void DebugCam::on_frame(fmilliseconds& dt) {}

//void DebugCam::on_config_load(const utility::Config& cfg) {
//    mod_enabled = cfg.get<bool>("debug_cam").value_or(false);
//};
//
//void DebugCam::on_config_save(utility::Config& cfg) {
//    cfg.set<bool>("debug_cam", mod_enabled);
//};