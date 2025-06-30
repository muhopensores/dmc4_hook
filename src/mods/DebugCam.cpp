#include "DebugCam.hpp"
#include "PhotoMode.hpp"
#include "CustomProjectile.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/Cam.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

//constexpr uintptr_t freecam_cons = 0x9197A0;
bool DebugCam::mod_enabled = false;
static void* (__stdcall*freecam_cons)() = (void*(__stdcall*)())0x9197A0;
uintptr_t DebugCam::jmp_ret1 = NULL;
float DebugCam::freecamSpeed = 1.0f;
float DebugCam::freecamModifierSpeed = 10.0f;
bool DebugCam::toggle_gameplay_cam = true;
bool DebugCam::projectileTest = false;
bool DebugCam::freecamMouseControls = false;
bool DebugCam::freecamKeyboardControls = false;
bool DebugCam::freecamGamepadControls = false;
bool DebugCam::disable_player_inputs = false;

static sCamera* get_sCamera() {
    uintptr_t sMain = 0x00E5574C;
    sCamera* ptr    = *(sCamera**)(*(uintptr_t*)sMain + 0x10358);
    return ptr;
}

static void* get_sMouse() {
    uintptr_t sMain = 0x00E5574C;
    void* ptr    = (void*)(*(uintptr_t*)sMain + 0x10384);
    return ptr;
}

static void set_viewport(uint32_t index, REGION_MODE mode, uintptr_t CameraPtr) {
    sCamera* sCam = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    viewport->mpCamera = (void*)CameraPtr;
    viewport->mMode = mode;
    viewport->mAttr = 0x17;
}

static sCamera_ViewPort* get_viewport(uint32_t index) {
    sCamera* sCam              = get_sCamera();
    sCamera_ViewPort* viewport = &sCam->viewports[index];
    return viewport;
}

bool input_down = false;

static void __stdcall freecam_mouse_input(uCamera* camera) {
    uintptr_t sMouse = (uintptr_t)get_sMouse();
    int x_diff = *(int*)(*(uintptr_t*)sMouse + 0x2C);
    int y_diff = *(int*)(*(uintptr_t*)sMouse + 0x30);
    Vector3f cam_pos = *(Vector3f*)&camera->mCameraPos;
    Vector3f cam_up = *(Vector3f*)&camera->mCameraUp;
    Vector3f cam_lookat = *(Vector3f*)&camera->mTargetPos;
    glm::mat4 viewMatrix = glm::lookAt(cam_pos, cam_lookat, cam_up);
    
    if (DebugCam::freecamMouseControls && !g_framework->m_draw_ui) {
        glm::mat4 rotateX = glm::rotate(glm::mat4(1.0f), (float)(y_diff * 0.001), glm::vec3(1.0f, 0.0f, 0.0f));
        glm::mat4 rotateY = glm::rotate(glm::mat4(1.0f), (float)(x_diff * 0.001), glm::vec3(0.0f, 1.0f, 0.0f));
        
        viewMatrix = glm::translate(viewMatrix, cam_pos);
        viewMatrix = viewMatrix * rotateY;
        viewMatrix = glm::translate(viewMatrix, -cam_pos);
        viewMatrix = rotateX * viewMatrix;
        if (DebugCam::projectileTest) {
            short mouse_click_state = GetAsyncKeyState(VK_RBUTTON);
            if (((mouse_click_state && 0xFF) == 1) && (input_down == false)) {
                input_down = true;
                void* projptr = devil4_sdk::mt_allocate_heap(sizeof(CustomProjectileProp), 16);
                glm::quat qua = glm::quat_cast(glm::inverse(viewMatrix));
                qua = glm::rotate(qua, (float)M_PI, glm::vec3(0.0, 1.0f, 0.0));
                Vector4f quat4 = glm::vec4(qua.x, qua.y, qua.z, qua.w);
                // void* projptr              = devil4_sdk::mt_allocate_heap(0x18D0, 16);
                CustomProjectileProp* proj = new (projptr) CustomProjectileProp(120.0f, 30.0f, (Vector3f*)&camera->mCameraPos, &quat4);
                devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)proj, 1);
            }
            if ((mouse_click_state && 0xFF) == 0)
                input_down = false;
        }
    }
    
    glm::mat4 rotateZ = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 0.0f, 1.0f));
    if (short input = VkKeyScan('Q'); devil4_sdk::internal_kb_check(input & 0xFF))
        rotateZ = glm::rotate(glm::mat4(1.0f), -0.005f, glm::vec3(0.0f, 0.0f, 1.0f));
    if (short input = VkKeyScan('E'); devil4_sdk::internal_kb_check(input & 0xFF))
        rotateZ = glm::rotate(glm::mat4(1.0f), 0.005f, glm::vec3(0.0f, 0.0f, 1.0f));
    viewMatrix = rotateZ * viewMatrix;
    
    if (DebugCam::freecamKeyboardControls) {
        if (devil4_sdk::internal_kb_check(VK_UP)) {
            glm::mat4 rotateUp = glm::rotate(glm::mat4(1.0f), -0.005f, glm::vec3(1.0f, 0.0f, 0.0f));
            viewMatrix = rotateUp * viewMatrix;
        }
        
        if (devil4_sdk::internal_kb_check(VK_DOWN)) {
            glm::mat4 rotateDown = glm::rotate(glm::mat4(1.0f), 0.005f, glm::vec3(1.0f, 0.0f, 0.0f));
            viewMatrix = rotateDown * viewMatrix;
        }
        
        if (devil4_sdk::internal_kb_check(VK_LEFT)) {
            glm::mat4 rotateLeft = glm::rotate(glm::mat4(1.0f), -0.005f, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMatrix = glm::translate(viewMatrix, cam_pos);
            viewMatrix = viewMatrix * rotateLeft;
            viewMatrix = glm::translate(viewMatrix, -cam_pos);
        }
        
        if (devil4_sdk::internal_kb_check(VK_RIGHT)) {
            glm::mat4 rotateRight = glm::rotate(glm::mat4(1.0f), 0.005f, glm::vec3(0.0f, 1.0f, 0.0f));
            viewMatrix = glm::translate(viewMatrix, cam_pos);
            viewMatrix = viewMatrix * rotateRight;
            viewMatrix = glm::translate(viewMatrix, -cam_pos);
        }
    }
    
    glm::mat4 translateMat(1.0f);
    
    if (DebugCam::freecamKeyboardControls) {
        float speed = DebugCam::freecamSpeed;
        if (devil4_sdk::internal_kb_check(VK_SHIFT))
            speed = DebugCam::freecamModifierSpeed;
        
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
    }
    
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
        cmp byte ptr [DebugCam::mod_enabled], 1
        je CallUpdateInput
        cmp byte ptr [PhotoMode::photo_mode_open], 1
        je CallUpdateInput
        jmp originalcode

    CallUpdateInput:
        pushad
        push esi
        call freecam_mouse_input
        popad
    originalcode:
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

static const int NUM_VIEWPORTS = 2;
static const float BUTTON_SIZE = 50.0f;
static const float BUTTON_SPACING = 5.0f;
static const ImVec4 SELECTED_BUTTON_COLOR(0.4f, 0.6f, 0.8f, 1.0f);
static const ImVec4 NORMAL_BUTTON_COLOR(0.2f, 0.2f, 0.2f, 1.0f);
static REGION_MODE selectedViewportOption[NUM_VIEWPORTS];

struct RegionButton {
    REGION_MODE mode;
    const char* label;
};

const RegionButton REGION_BUTTONS[3][3] = {
    {
        { REGION_TOPLEFT, __("Top\nLeft") },
        { REGION_TOP, __("Top") },
        { REGION_TOPRIGHT, __("Top\nRight") }
    },
    {
        { REGION_LEFT, __("Left") },
        { REGION_FULLSCREEN, __("Full") },
        { REGION_RIGHT, __("Right") }
    },
    {
        { REGION_BOTTOMLEFT, __("Bottom\nLeft") },
        { REGION_BOTTOM, __("Bottom") },
        { REGION_BOTTOMRIGHT, __("Bottom\nRight") }
    }
};

static void ToggleGameplayCam(bool enable) {
    sCamera_ViewPort* vp = get_viewport(0);
    vp->mActive = enable;
    uFreeCamera* cam = (uFreeCamera*)freecam_cons();
    if (DebugCam::freecamGamepadControls) cam->mControlPad = 0;
    devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)cam, 0x17);
    sCamera_ViewPort* first_vp = get_viewport(0);
    set_viewport(1, REGION_FULLSCREEN, (uintptr_t)cam);
    cam->mCameraPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraPos;
    cam->mTargetPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mTargetPos;
    cam->mCameraUp = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraUp;
}

static void DisablePlayerInputs(bool enable) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    player->pad_1e80[2] = enable;
}

void DebugCam::DrawDebugCamSettings(int ImGuiID) {
    ImGui::PushID(ImGuiID);
    for (int viewportId = 0; viewportId < NUM_VIEWPORTS; viewportId++) {
        if (viewportId % 2 != 0) {
            ImGui::SameLine(sameLineWidth + lineIndent);
        }
        ImGui::PushID(viewportId);
        ImGui::BeginGroup();
        if (viewportId == 0) {
            ImGui::Text(_("Gameplay Cam"));
        }
        else {
            ImGui::Text(_("Extra Cam %i"), viewportId);
        }
        ImGui::SetNextItemWidth(sameLineItemWidth);
        for (int row = 0; row < 3; row++) {
            for (int col = 0; col < 3; col++) {
                const RegionButton& rb = REGION_BUTTONS[row][col];
                ImGui::PushStyleColor(ImGuiCol_Button,
                    (selectedViewportOption[viewportId] == rb.mode) ?
                    SELECTED_BUTTON_COLOR : NORMAL_BUTTON_COLOR);
                if (ImGui::Button(utility::text_lookup((char*)rb.label), ImVec2(BUTTON_SIZE, BUTTON_SIZE))) {
                    selectedViewportOption[viewportId] = rb.mode;

                    if (viewportId == 0) {
                        sCamera_ViewPort* first_vp = get_viewport(0);
                        first_vp->mMode = selectedViewportOption[viewportId];
                    }
                    else {
                        uFreeCamera* cam = (uFreeCamera*)freecam_cons();
                        if (freecamGamepadControls) cam->mControlPad = 0;
                        devil4_sdk::spawn_or_something((void*)0x00E552CC, (MtObject*)cam, 0x17);
                        sCamera_ViewPort* first_vp = get_viewport(0);
                        set_viewport(viewportId, selectedViewportOption[viewportId], (uintptr_t)cam);
                        cam->mCameraPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraPos;
                        cam->mTargetPos = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mTargetPos;
                        cam->mCameraUp = ((uCamera*)*(uintptr_t*)&first_vp->mpCamera)->mCameraUp;
                    }
                }
                ImGui::PopStyleColor();
                if (col < 2) {
                    ImGui::SameLine(0, BUTTON_SPACING);
                }
            }
        }
        ImGui::EndGroup();
        ImGui::PopID();
    }
    if (ImGui::Checkbox(_("Toggle Gameplay Cam"), &toggle_gameplay_cam)) {
        ToggleGameplayCam(toggle_gameplay_cam);
    }
    ImGui::Checkbox(_("Mouse Controls"), &freecamMouseControls);
    // ImGui::Checkbox("Pew pew", &projectileTest);
    ImGui::Checkbox(_("Keyboard Controls"), &freecamKeyboardControls);
    ImGui::SameLine();
    help_marker(_("Controls are:\nW, A, S, D = movement\nArrow keys = Pitch & Yaw\nSpace & Ctrl = Up & Down\nQ & E = roll\nShift = speed modifier\n"));
    ImGui::Checkbox(_("Gamepad Controls"), &freecamGamepadControls);
    ImGui::SameLine();
    help_marker(_("This option applies when selecting a new splitscreen"));
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Camera Speed"), &freecamSpeed, 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat(_("Camera Modifier Speed"), &freecamModifierSpeed, 0.0f, 200.0f, "%.0f");
    ImGui::PopItemWidth();
    ImGui::PopID();
}

// Used by photo mode. Photo mode should definitely be put in here but it's very crowded with all these struct and func definitions
void DebugCam::DrawDebugCamSettings2(int ImGuiID) {
    ImGui::BeginGroup();
    ImGui::PushID(ImGuiID);
    if (ImGui::Checkbox(_("Gameplay Cam"), &toggle_gameplay_cam)) {
        ToggleGameplayCam(toggle_gameplay_cam);
    }
    ImGui::Checkbox(_("Mouse Controls"), &freecamMouseControls);
    ImGui::Checkbox(_("Keyboard Controls"), &freecamKeyboardControls);
    ImGui::SameLine();
    help_marker(_("Controls are:\nW, A, S, D = movement\nArrow keys = Pitch & Yaw\nSpace & Ctrl = Up & Down\nQ & E = roll\nShift = speed modifier\n"));
    if (ImGui::Checkbox(_("Disable Player Inputs"), &disable_player_inputs)) {
        DisablePlayerInputs(disable_player_inputs);
    }
    // ImGui::Checkbox(_("Gamepad Controls"), &freecamGamepadControls);
    // ImGui::SameLine();
    // help_marker(_("This option applies when selecting a new splitscreen"));
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::SliderFloat(_("Camera Speed"), &freecamSpeed, 0.0f, 100.0f, "%.0f");
    ImGui::SliderFloat(_("Camera Modifier Speed"), &freecamModifierSpeed, 0.0f, 200.0f, "%.0f");
    ImGui::PopItemWidth();
    ImGui::PopID();
    ImGui::EndGroup();
}

void DebugCam::on_gui_frame(int display) {
    if (display == 0) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Free Camera"), &mod_enabled)) {
            if (!mod_enabled) {
                toggle_gameplay_cam = true;
                ToggleGameplayCam(toggle_gameplay_cam);
            }
        }
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            DebugCam::DrawDebugCamSettings(display);
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
    else if (display == 2) {
        DebugCam::DrawDebugCamSettings2(display);
    }
}

// void DebugCam::on_frame(fmilliseconds& dt) {}

void DebugCam::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("debug_cam").value_or(false);
    freecamSpeed = cfg.get<float>("freecamSpeed").value_or(10.0f);
    freecamModifierSpeed = cfg.get<float>("freecamModifierSpeed").value_or(20.0f);
    freecamMouseControls = cfg.get<bool>("freecamMouseControls").value_or(true);
    freecamKeyboardControls = cfg.get<bool>("freecamKeyboardControls").value_or(true);
    freecamGamepadControls = cfg.get<bool>("freecamGamepadControls").value_or(false);
};

void DebugCam::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("debug_cam", mod_enabled);
    cfg.set<float>("freecamSpeed", freecamSpeed);
    cfg.set<float>("freecamModifierSpeed", freecamModifierSpeed);
    cfg.set<bool>("freecamMouseControls", freecamMouseControls);
    cfg.set<bool>("freecamKeyboardControls", freecamKeyboardControls);
    cfg.set<bool>("freecamGamepadControls", freecamGamepadControls);
};
