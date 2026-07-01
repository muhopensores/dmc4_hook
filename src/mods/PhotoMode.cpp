#include "PhotoMode.hpp"
#include "DebugCam.hpp"
#include "CameraSettings.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/Cam.hpp"
#include "sdk/World2Screen.hpp"
#include "sdk/uFilter.hpp"
#include "sdk/sWorkRate.hpp"
#include "sdk/sUnit.hpp"
#include <Mods.hpp>
#include "utility/Hash.hpp"

#include "../sdk/uLight.hpp"

bool PhotoMode::mod_enabled = false;
float PhotoMode::HUDCooldown = 0.0f;
static bool spotLightsFollowPlayer = true;

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

static void* (__stdcall*freecam_cons)() = (void*(__stdcall*)())0x9197A0;

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

static constexpr uintptr_t PM_INF_LIGHT = 0x00EAE3E8;
static constexpr uintptr_t PM_SPOT_LIGHT = 0x00EAE4C8;
static constexpr uintptr_t PM_POINT_LIGHT = 0x00EAE4A8;
static constexpr uintptr_t PM_HEMI_LIGHT = 0x00EAE408;
static constexpr uintptr_t PM_BACK_LIGHT = 0x00EAE428;
static constexpr uintptr_t PM_DOF_FILTER = 0x00EADAA0;
static constexpr uintptr_t PM_TONE_MAP_FILTER = 0x00EADAE0;
static constexpr uintptr_t PM_COLOR_CORR_FILTER = 0x00EADA20;
static constexpr uintptr_t PM_COLOR_SPACE_FILTER = 0x00EADBC0;
static constexpr uintptr_t PM_LVL_CORR_FILTER = 0x00EAD9C0;
static constexpr uintptr_t PM_CONTRAST_FILTER = 0x00EAD980;
static constexpr uintptr_t PM_HSV_FILTER = 0x00EAD9A0;

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
    cam->mCameraPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraPos;
    cam->mTargetPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mTargetPos;
    cam->mCameraUp = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraUp;

    sCamera_ViewPort* vp = get_viewport(0);
    vp->mActive = enable;
}

static void DisablePlayerInputs(bool enable) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    player->pad_1e80[2] = enable;
}

void PhotoMode::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Photo Mode"), &PhotoMode::mod_enabled)) {
            if (mod_enabled) {
                SetGameSpeeds(0.0f);
            } else {
                SetGameSpeeds(1.0f);
                DebugCam::toggle_gameplay_cam = true;
                ToggleGameplayCam(DebugCam::toggle_gameplay_cam);
            }
            DebugCam::disable_player_inputs = mod_enabled;
            DisablePlayerInputs(DebugCam::disable_player_inputs);
        }
    }
}

static int selectedLightIndex = -1;
static bool isManipulatingLight = false;
static ImGuizmo::OPERATION currentLightGizmoOperation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE currentLightGizmoMode = ImGuizmo::WORLD;

static void ImGuizmoManipulators() {
    w2s::ImGuizmoSetup();
    float view[16], projection[16];
    if (!w2s::GetImGuizmoMatrices(view, projection)) { return; }
    sUnit* s_unit      = (sUnit*)devil4_sdk::get_sUnit();
    MoveLine* moveline = &s_unit->mMoveLine[17];
    cUnit* obj         = moveline->mTop;
    int lightIndex = 0;
    auto* drawList = ImGui::GetForegroundDrawList();
    while (obj != nullptr) {
        bool del_flag = obj->flags.bits.mBeFlag & BEFLAG_DELETE;
        bool pre_del_flag = obj->flags.bits.mBeFlag & BEFLAG_PRE_DELETE;
        if (!(del_flag || pre_del_flag)) {
            lightIndex++;
            obj = obj->mp_next_unit;
            continue;
        }
        uintptr_t objectType = (uintptr_t)call_constructor(*(void**)((*(uintptr_t**)obj) + 4));
        glm::vec3 lightPos;
        const char* objectName = nullptr;
        ImU32 lightColor       = IM_COL32(255, 255, 0, 200);

        switch (objectType) {
            case PM_SPOT_LIGHT: {
                uSpotLight* spot_light = (uSpotLight*)obj;
                lightPos = spot_light->mPos;
                objectName = _("SpotLight");
                glm::vec3 dir;
                if (spot_light->mTargetMode) {
                    dir = spot_light->mTargetPos;
                } else {
                    dir = spot_light->mDir;
                }
                if (glm::length(dir) < 0.0001f)
                    dir = glm::vec3(0, 0, 1);
                else
                    dir = glm::normalize(dir);

                glm::vec3 worldEnd = lightPos + dir * spot_light->mEnd;
                if (currentLightGizmoOperation == ImGuizmo::TRANSLATE) {
                    glm::vec3 newPos;
                    if (w2s::DrawImGuizmoManipulator(lightPos, newPos, lightIndex, selectedLightIndex, isManipulatingLight, ImGuizmo::TRANSLATE,
                            currentLightGizmoMode, view, projection, lightColor, 8.0f, objectName)) {
                        glm::vec3 delta = newPos - lightPos;
                        spot_light->mPos += delta;
                        lightPos = newPos;
                    }
                }

                else if (currentLightGizmoOperation == ImGuizmo::ROTATE && !spot_light->mTargetMode) {
                    glm::vec3 dirRot = spot_light->mDir;
                    if (glm::length(dirRot) < 0.0001f)
                        dirRot = glm::vec3(0, 0, 1);

                    glm::vec3 up(0, 1, 0);
                    if (fabs(glm::dot(dirRot, up)) > 0.99f)
                        up = glm::vec3(1, 0, 0);

                    glm::vec3 right = glm::normalize(glm::cross(up, dirRot));
                    up              = glm::normalize(glm::cross(dirRot, right));

                    float matrix[16] = {
                        right.x, right.y, right.z, 0,
                        up.x, up.y, up.z, 0,
                        dirRot.x, dirRot.y, dirRot.z, 0,
                        lightPos.x, lightPos.y, lightPos.z, 1
                    };
                    ImGuizmo::Manipulate(view, projection, ImGuizmo::ROTATE, currentLightGizmoMode, matrix);
                    glm::vec3 newDir(matrix[8], matrix[9], matrix[10]);
                    if (glm::length(newDir) > 0.0001f)
                        newDir = glm::normalize(newDir);

                    spot_light->mDir = newDir;
                }

                if (currentLightGizmoOperation == ImGuizmo::SCALE) {
                    glm::vec2 screenPos = w2s::WorldToScreen(lightPos);
                    if (w2s::IsVisibleOnScreen(screenPos)) {
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x + 20.0f, screenPos.y));
                        ImGui::SetNextWindowBgAlpha(0.6f);
                        ImGui::Begin("SpotLight Edit", nullptr,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
                        ImGui::SliderFloat(_("Start"), &spot_light->mStart, 0.f, 3000.f);
                        ImGui::SliderFloat(_("End"), &spot_light->mEnd, 0.f, 3000.f);
                        ImGui::SliderFloat(_("Cone"), &spot_light->mCone, 0.f, 360.f);
                        ImGui::SliderFloat(_("Spread"), &spot_light->mSpread, 0.f, 3000.0f);
                        ImGui::Checkbox(_("Target Mode"), &spot_light->mTargetMode);
                        ImGui::End();
                    }
                }

                if (spot_light->mTargetMode) {
                    uPlayer* player = devil4_sdk::get_local_player();
                    Vector3f playerPos       = player->mPos;
                    Vector3f offset = spot_light->mTargetPos;
                    Vector3f worldTarget;
                    if (spotLightsFollowPlayer)
                        worldTarget = playerPos;
                    else 
                        worldTarget = lightPos + offset;

                    glm::vec3 newWorldTarget = worldTarget;
                    int gizmoID = 100000 + lightIndex;
                    ImGuizmo::SetID(gizmoID);
                    if (w2s::DrawImGuizmoManipulator(worldTarget, newWorldTarget, gizmoID, selectedLightIndex, isManipulatingLight,
                            ImGuizmo::TRANSLATE, currentLightGizmoMode, view, projection, IM_COL32(0, 200, 255, 255), 6.0f, "Target")) {
                        glm::vec3 newOffset = newWorldTarget - lightPos;
                        spot_light->mTargetPos = newOffset;
                    }
                }

                glm::vec2 p0 = w2s::WorldToScreen(lightPos);

                glm::vec2 p1 = w2s::WorldToScreen(worldEnd);
                if (w2s::IsVisibleOnScreen(p0)) {
                    drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), IM_COL32(0, 255, 255, 200), 2.0f);
                    drawList->AddText(ImVec2(p0.x + 10.0f, p0.y - 10.0f), IM_COL32(255, 255, 0, 255), objectName);
                    if (w2s::IsVisibleOnScreen(p1)) {
                        drawList->AddCircleFilled(ImVec2(p1.x, p1.y), 4.0f, IM_COL32(0, 255, 255, 255));
                    }
                }
                break;
            }

            case PM_POINT_LIGHT: {
                uPointLight* point_light = (uPointLight*)obj;
                lightPos = point_light->mPos;
                objectName = _("PointLight");
                if (currentLightGizmoOperation == ImGuizmo::TRANSLATE) {
                    glm::vec3 newPos;
                    if (w2s::DrawImGuizmoManipulator(lightPos, newPos, lightIndex, selectedLightIndex, isManipulatingLight, ImGuizmo::TRANSLATE,
                            currentLightGizmoMode, view, projection, lightColor, 8.0f, objectName)) {
                        point_light->mPos = newPos;
                    }
                }

                if (currentLightGizmoOperation == ImGuizmo::SCALE) {
                    glm::vec2 screenPos = w2s::WorldToScreen(lightPos);
                    if (w2s::IsVisibleOnScreen(screenPos)) {
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x + 20.0f, screenPos.y));
                        ImGui::SetNextWindowBgAlpha(0.6f);
                        ImGui::Begin("PointLight Edit", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
                        ImGui::SliderFloat(_("Start"), &point_light->mStart, 0.f, 3000.f);
                        ImGui::SliderFloat(_("End"), &point_light->mEnd, 0.f, 3000.f);
                        ImGui::ColorEdit4(_("Color"), &point_light->mColor.x);
                        ImGui::End();
                    }
                }

                glm::vec2 p0 = w2s::WorldToScreen(lightPos);
                if (w2s::IsVisibleOnScreen(p0)) {
                    drawList->AddText(ImVec2(p0.x + 10.0f, p0.y - 10.0f), IM_COL32(255, 255, 255, 255), objectName);
                }

                break;
            }
            case PM_HEMI_LIGHT: {
                uHemiSphereLight* hemi_light = (uHemiSphereLight*)obj;

                lightPos   = glm::vec3(0.0f, 0.0f, 0.0f);
                objectName = _("HemiLight");

                if (currentLightGizmoOperation == ImGuizmo::ROTATE) {
                    glm::vec3 dirRot = hemi_light->mDir;

                    if (glm::length(dirRot) < 0.0001f)
                        dirRot = glm::vec3(0, 0, 1);
                    else
                        dirRot = glm::normalize(dirRot);

                    glm::vec3 up(0, 1, 0);
                    if (fabs(glm::dot(dirRot, up)) > 0.99f)
                        up = glm::vec3(1, 0, 0);

                    glm::vec3 right = glm::normalize(glm::cross(up, dirRot));
                    up              = glm::normalize(glm::cross(dirRot, right));

                    float matrix[16] = {
                        right.x, right.y, right.z, 0, up.x, up.y, up.z, 0, dirRot.x, dirRot.y, dirRot.z, 0, 0.0f, 0.0f, 0.0f, 1};

                    ImGuizmo::Manipulate(view, projection, ImGuizmo::ROTATE, currentLightGizmoMode, matrix);

                    glm::vec3 newDir(matrix[8], matrix[9], matrix[10]);

                    if (glm::length(newDir) > 0.0001f)
                        newDir = glm::normalize(newDir);

                    hemi_light->mDir = newDir;
                }

                if (currentLightGizmoOperation == ImGuizmo::SCALE) {
                    glm::vec2 screenPos = w2s::WorldToScreen(lightPos);
                    if (w2s::IsVisibleOnScreen(screenPos)) {
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x + 20.0f, screenPos.y));
                        ImGui::SetNextWindowBgAlpha(0.6f);
                        ImGui::Begin("PointLight Edit", nullptr,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
                        ImGui::ColorEdit4(_("Color"), &hemi_light->mColor.x);
                        ImGui::ColorEdit4(_("RevColor"), &hemi_light->mRevColor.x);
                        ImGui::End();
                    }
                }

                glm::vec3 dir = hemi_light->mDir;

                if (glm::length(dir) > 0.0001f)
                    dir = glm::normalize(dir);
                else
                    dir = glm::vec3(0, 0, 1);

                glm::vec3 worldEnd = dir * 500.0f;

                glm::vec2 p0 = w2s::WorldToScreen(glm::vec3(0, 0, 0));
                glm::vec2 p1 = w2s::WorldToScreen(worldEnd);

                if (w2s::IsVisibleOnScreen(p0)) {
                    drawList->AddLine(ImVec2(p0.x, p0.y), ImVec2(p1.x, p1.y), IM_COL32(0, 255, 0, 200), 2.0f);
                    drawList->AddText(ImVec2(p0.x + 10.0f, p0.y - 10.0f), IM_COL32(0, 255, 0, 255), objectName);
                    if (w2s::IsVisibleOnScreen(p1)) {
                        drawList->AddCircleFilled(ImVec2(p1.x, p1.y), 4.0f, IM_COL32(0, 255, 0, 255));
                    }
                }
                break;
            }
            case PM_BACK_LIGHT: {
                uBackLight* back_light = (uBackLight*)obj;
                objectName = _("BackLight");
                glm::vec2 p0 = w2s::WorldToScreen(glm::vec3(0, 0, 0));

                if (currentLightGizmoOperation == ImGuizmo::SCALE) {
                    glm::vec2 screenPos = w2s::WorldToScreen(lightPos);
                    if (w2s::IsVisibleOnScreen(screenPos)) {
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x + 20.0f, screenPos.y));
                        ImGui::SetNextWindowBgAlpha(0.6f);
                        ImGui::Begin("BackLight Edit", nullptr,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
                        ImGui::ColorEdit4(_("Color"), &back_light->mColor.x);
                        ImGui::End();
                    }
                }

                if (w2s::IsVisibleOnScreen(p0)) {
                    drawList->AddText(ImVec2(p0.x + 10.0f, p0.y - 10.0f), IM_COL32(0, 255, 0, 255), objectName);
                }
                break;
            }
            case PM_INF_LIGHT: {
                uInfiniteLight* inf_light = (uInfiniteLight*)obj;

                lightPos   = glm::vec3(0.0f, 0.0f, 0.0f);
                objectName = _("InfiniteLight");

                if (currentLightGizmoOperation == ImGuizmo::ROTATE) {
                    glm::vec3 dirRot = inf_light->mDir;
                    if (glm::length(dirRot) < 0.0001f)
                        dirRot = glm::vec3(0, 0, 1);
                    glm::vec3 up(0, 1, 0);
                    if (fabs(glm::dot(dirRot, up)) > 0.99f)
                        up = glm::vec3(1, 0, 0);
                    glm::vec3 right = glm::normalize(glm::cross(up, dirRot));
                    up              = glm::normalize(glm::cross(dirRot, right));
                    float matrix[16] = {
                        right.x, right.y, right.z, 0, up.x, up.y, up.z, 0, dirRot.x, dirRot.y, dirRot.z, 0, 0.0f, 0.0f, 0.0f, 1};
                    ImGuizmo::Manipulate(view, projection, ImGuizmo::ROTATE, currentLightGizmoMode, matrix);
                    glm::vec3 newDir(matrix[8], matrix[9], matrix[10]);
                    if (glm::length(newDir) > 0.0001f)
                        newDir = glm::normalize(newDir);
                    inf_light->mDir = newDir;
                }

                if (currentLightGizmoOperation == ImGuizmo::SCALE) {
                    glm::vec2 screenPos = w2s::WorldToScreen(lightPos);
                    if (w2s::IsVisibleOnScreen(screenPos)) {
                        ImGui::SetNextWindowPos(ImVec2(screenPos.x + 20.0f, screenPos.y));
                        ImGui::SetNextWindowBgAlpha(0.6f);
                        ImGui::Begin("InfLight Edit", nullptr,
                            ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
                        ImGui::ColorEdit4(_("Color"), &inf_light->mColor.x);
                        ImGui::End();
                    }
                }

                glm::vec2 p0 = w2s::WorldToScreen(glm::vec3(0, 0, 0));
                if (w2s::IsVisibleOnScreen(p0)) {
                    drawList->AddText(ImVec2(p0.x + 10.0f, p0.y - 10.0f), IM_COL32(0, 255, 0, 255), objectName);
                }

                break;
            }
        }
        lightIndex++;
        obj = obj->mp_next_unit;
    }

    ImGuizmo::MODE currentImGuizmode = ImGuizmo::WORLD;
    w2s::ImGuizmoKeyboardShortcuts(currentLightGizmoOperation, currentImGuizmode);
    w2s::ImGuizmoDeselection(selectedLightIndex);
}

void PhotoMode::drawLightInfo(MoveLine* moveline, int& numOfObjs) {
    cUnit* obj    = moveline->mTop;
    while (obj != nullptr) {
        cUnit* nextObj = obj->mp_next_unit;

        bool del_flag     = obj->flags.bits.mBeFlag & BEFLAG_DELETE;
        bool pre_del_flag = obj->flags.bits.mBeFlag & BEFLAG_PRE_DELETE;

        if (del_flag != 0 || pre_del_flag != 0) {
            ImGui::BeginGroup();
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::PushID((uintptr_t)obj);

            switch ((uintptr_t)call_constructor(*(void**)((*(uintptr_t**)obj) + 4))) {
                case PM_INF_LIGHT: {
                    uInfiniteLight* inf_light = (uInfiniteLight*)obj;
                    ImGui::Text(_("Infinite Light"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::ColorEdit4(_("Color"), (float*)&inf_light->mColor);
                    ImGui::InputFloat3(_("Dir"), (float*)&inf_light->mDir);
                    break;
                }

                case PM_SPOT_LIGHT: {
                    uSpotLight* spot_light = (uSpotLight*)obj;
                    spot_light->mpTarget   = (uCoord*)devil4_sdk::get_local_player();
                    spot_light->mTargetNo  = 0;
                    ImGui::Text(_("Spot Light"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::ColorEdit4(_("Color"), (float*)&spot_light->mColor);
                    ImGui::InputFloat3(_("Pos"), (float*)&spot_light->mPos);
                    ImGui::InputFloat3(_("Dir"), (float*)&spot_light->mDir);
                    ImGui::InputFloat3(_("Target Pos"), (float*)&spot_light->mTargetPos);
                    ImGui::InputFloat3(_("World Pos"), (float*)&spot_light->mWPos);
                    ImGui::InputFloat3(_("World Dir"), (float*)&spot_light->mWDir);
                    ImGui::SliderFloat(_("Start"), &spot_light->mStart, 0.f, 3000.f);
                    ImGui::SliderFloat(_("End"), &spot_light->mEnd, 0.f, 3000.f);
                    ImGui::SliderFloat(_("Cone"), &spot_light->mCone, 0.f, 360.f);
                    ImGui::SliderFloat(_("Spread"), &spot_light->mSpread, 0.f, 3000.0f);
                    ImGui::Checkbox(_("Target Mode"), &spot_light->mTargetMode);
                    break;
                }

                case PM_POINT_LIGHT: {
                    uPointLight* point_light = (uPointLight*)obj;
                    ImGui::Text(_("Point Light"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::ColorEdit4(_("Color"), (float*)&point_light->mColor);
                    ImGui::InputFloat3(_("Pos"), (float*)&point_light->mPos);
                    ImGui::InputFloat3(_("World Pos"), (float*)&point_light->mWPos);
                    ImGui::SliderFloat(_("Start"), &point_light->mStart, 0.f, 3000.f);
                    ImGui::SliderFloat(_("End"), &point_light->mEnd, 0.f, 3000.f);
                    break;
                }

                case PM_HEMI_LIGHT: {
                    uHemiSphereLight* hemi_light = (uHemiSphereLight*)obj;
                    ImGui::Text(_("Hemi Light"));
                    if (ImGui::Button(_("Remove")))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::ColorEdit4(_("Color"), (float*)&hemi_light->mColor);
                    ImGui::ColorEdit4(_("RevColor"), (float*)&hemi_light->mRevColor);
                    ImGui::InputFloat3(_("Dir"), (float*)&hemi_light->mDir);
                    break;
                }

                case PM_BACK_LIGHT: {
                    uBackLight* back_light = (uBackLight*)obj;
                    ImGui::Text(_("Back Light"));
                    if (ImGui::Button(_("Remove")))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::ColorEdit4(_("Color"), (float*)&back_light->mColor);
                    break;
                }

                case PM_DOF_FILTER: {
                    uDOFFilter* dof_filter = (uDOFFilter*)obj;
                    ImGui::Text(_("DOF Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::InputInt(_("Blur count"), (int*)&dof_filter->mBlurCount);
                    ImGui::SliderInt(_("Type"), (int*)&dof_filter->mType, 0, 2);
                    ImGui::SliderInt(_("Blur type"), (int*)&dof_filter->mBlurType, 1, 2);
                    ImGui::ColorEdit4(_("Gradate Color"), (float*)&dof_filter->mGradateColor);
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
                    ImGui::Text(_("Tone Map Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    break;
                }

                case PM_COLOR_CORR_FILTER: {
                    ImGui::Text(_("Colour Correct Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    break;
                }

                case PM_COLOR_SPACE_FILTER: {
                    uColorSpaceFilter* cs_filter = (uColorSpaceFilter*)obj;
                    ImGui::Text(_("Colour Space Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::InputFloat4(_("Angle"), (float*)&cs_filter->mAngle);
                    ImGui::InputFloat4(_("Scale"), (float*)&cs_filter->mScale);
                    break;
                }

                case PM_LVL_CORR_FILTER: {
                    uLevelCorrectFilter* lvl_filter = (uLevelCorrectFilter*)obj;
                    ImGui::Text(_("Level Correct Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::InputFloat4(_("In Black"), (float*)&lvl_filter->mInBlack);
                    ImGui::InputFloat4(_("In White"), (float*)&lvl_filter->mInWhite);
                    ImGui::InputFloat4(_("Input Gamma"), (float*)&lvl_filter->mInputGamma);
                    ImGui::InputFloat4(_("Out Black"), (float*)&lvl_filter->mOutBlack);
                    ImGui::InputFloat4(_("Out White"), (float*)&lvl_filter->mOutWhite);
                    break;
                }

                case PM_CONTRAST_FILTER: {
                    uContrastFilter* contr_filter = (uContrastFilter*)obj;
                    ImGui::Text(_("Contrast Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::InputFloat(_("Contrast"), &contr_filter->mContrast);
                    break;
                }

                case PM_HSV_FILTER: {
                    uHSVFilter* hsv_filter = (uHSVFilter*)obj;
                    ImGui::Text(_("HSV Filter"));
                    if (ImGui::Button("Remove"))
                        obj->flags.bits.mBeFlag = 3;
                    ImGui::InputFloat(_("Shift Hue"), &hsv_filter->mShiftHue);
                    ImGui::InputFloat(_("Shift Saturation"), &hsv_filter->mShiftSaturation);
                    ImGui::InputFloat(_("Shift Value"), &hsv_filter->mShiftValue);
                    break;
                }
            }

            ImGui::PopItemWidth();
            ImGui::PopID();
            ImGui::EndGroup();

            numOfObjs++;
            obj = obj->mp_next_unit;
            if (numOfObjs % 3 != 0 && obj != nullptr) {
                ImGui::SameLine();
            }
        }

        obj = nextObj;
    }
}

void PhotoMode::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        ImVec2 ScreenSize = ImGui::GetIO().DisplaySize;
        if (HUDCooldown > 0.0f) {
            /*if (HUDCooldown <= 1.0f) {
                unhide hud here
            }*/
            HUDCooldown -= 1.0f;
        }
        if (player && HUDCooldown <= 0.0f) {
            ImGui::Begin("Photo Mode##UI");
            ImGuizmoManipulators();
            static bool pauseGame = true;
            ImGui::SetWindowPos(ImVec2(ScreenSize.x * 0.6f, ScreenSize.y * 0.0f), ImGuiCond_Once);
            ImGui::SetWindowSize(ImVec2(ScreenSize.x * 0.4f, ScreenSize.y * 0.9f), ImGuiCond_Once);

            ImGui::Text(_("Light Manipulator Controls:"));
            ImGui::Text(_("G - Translate, R - Rotate, S - Settings"));

            if (ImGui::RadioButton(_("Translate"), currentLightGizmoOperation == ImGuizmo::TRANSLATE)) {
                currentLightGizmoOperation = ImGuizmo::TRANSLATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(_("Rotate"), currentLightGizmoOperation == ImGuizmo::ROTATE)) {
                currentLightGizmoOperation = ImGuizmo::ROTATE;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton(_("Settings"), currentLightGizmoOperation == ImGuizmo::SCALE)) {
                currentLightGizmoOperation = ImGuizmo::SCALE;
            }

            if (ImGui::Button("Hide Photo Mode UI")) {
                HUDCooldown = 300.0f;
                /*hide hud here*/
            }
            ImGui::SameLine();
            help_marker(_("Hide UI for a few seconds so you have time to take a screenshot"));

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
                    ImGui::BeginChild("PhotoTabBarChild");
                    ImGui::SeparatorText(_("Camera"));
                    g_framework->get_mods().get()->on_draw_ui("CameraSettings"_hash, DISPLAY_SYSTEM_C);
                    ImGui::SameLine();
                    g_framework->get_mods().get()->on_draw_ui("DebugCam"_hash, DISPLAY_SYSTEM_B);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem(_("Lights/Filters"))) {
                    ImGui::BeginChild("PhotoTabBarChild");
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
                        spawn((void*)uBackLightCons);

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
                    int numOfObjs = 0;
                    MoveLine* moveline = &s_unit->mMoveLine[17];
                    if (moveline != nullptr) {
                        ImGui::SeparatorText(_("Added Objects"));
                        drawLightInfo(moveline, numOfObjs);
                    }

                    moveline = &s_unit->mMoveLine[24];
                    if (moveline != nullptr) {
                        ImGui::SeparatorText(_("Existing Lights"));
                        drawLightInfo(moveline, numOfObjs);
                    }

                    moveline = &s_unit->mMoveLine[29];
                    if (moveline != nullptr) {
                        ImGui::SeparatorText(_("Existing Filters"));
                        drawLightInfo(moveline, numOfObjs);
                    }
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
    }
}
