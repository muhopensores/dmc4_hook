﻿#include "CameraSettings.hpp"
#include "PhotoMode.hpp"
static bool camera_sens_enabled = false;
static bool camera_auto_correct_towards_cam_enabled = false;
bool CameraSettings::mod_enabled = false;
float CameraSettings::camera_height = 0;
float CameraSettings::camera_distance = 0;
float CameraSettings::camera_distance_lockon = 0;
float CameraSettings::camera_angle = 0;
float CameraSettings::camera_angle_lockon = 0;
float CameraSettings::camera_fov_in_battle = 0;
float CameraSettings::camera_fov = 0;
bool CameraSettings::camera_lookdown_enabled = false;
bool CameraSettings::camera_reset_enabled = false;
bool CameraSettings::cam_right = false;
bool CameraSettings::disable_last_enemy_zoom = false;
bool CameraSettings::force_last_enemy_zoom = false;
bool CameraSettings::pause_camera_enabled = false;
bool CameraSettings::camera_lockon_corrects = false;

constexpr ptrdiff_t camera_towards_auto_correct1 = 0x19514;

uintptr_t CameraSettings::camera_height_continue = NULL;
uintptr_t CameraSettings::camera_distance_continue = NULL;
uintptr_t CameraSettings::camera_distance_lockon_continue = NULL;
uintptr_t CameraSettings::camera_angle_continue = NULL;
uintptr_t CameraSettings::camera_angle_lockon_continue = NULL;
uintptr_t CameraSettings::camera_fov_in_battle_continue = NULL;
uintptr_t CameraSettings::camera_fov_continue = NULL;
uintptr_t CameraSettings::camera_reset_keyboard_continue = NULL;
uintptr_t CameraSettings::camera_reset_continue = NULL;
uintptr_t CameraSettings::camera_sens_clockwise_continue = NULL;
uintptr_t CameraSettings::camera_sens_anti_clockwise_continue = NULL;
uintptr_t CameraSettings::camera_sens_brakes_continue = NULL;

static float degrees = 1.57f;
static float double_camera_sens = 2.0f;

naked void camera_height_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[edi+0x000000D0]
			addss xmm0,[CameraSettings::camera_height]
			jmp dword ptr [CameraSettings::camera_height_continue]

        code:
            movss xmm0,[edi+0x000000D0]
            jmp dword ptr [CameraSettings::camera_height_continue]
    }
}

naked void camera_distance_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[edi+0x000000E0]
			addss xmm0,[CameraSettings::camera_distance]
			jmp dword ptr [CameraSettings::camera_distance_continue]

        code:
            movss xmm0,[edi+0x000000E0]
            jmp dword ptr [CameraSettings::camera_distance_continue]
    }
}

naked void camera_distance_lockon_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[ebx+0x000000DC]
			addss xmm0,[CameraSettings::camera_distance_lockon]
			jmp dword ptr [CameraSettings::camera_distance_lockon_continue]

        code:
            movss xmm0,[ebx+0x000000DC]
            jmp dword ptr [CameraSettings::camera_distance_lockon_continue]
    }
}

naked void camera_angle_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm2,[edi+0x000000D4]
			addss xmm2,[CameraSettings::camera_angle]
            comiss xmm0,xmm1
			jmp dword ptr [CameraSettings::camera_angle_continue]

        code:
            movss xmm2,[edi+0x000000D4]
            comiss xmm0,xmm1
            jmp dword ptr [CameraSettings::camera_angle_continue]
    }
}

naked void camera_angle_lockon_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[ebx+0x000000D4]
			addss xmm0,[CameraSettings::camera_angle_lockon]
			jmp dword ptr [CameraSettings::camera_angle_lockon_continue]

        code:
            movss xmm0,[ebx+0x000000D4]
            jmp dword ptr [CameraSettings::camera_angle_lockon_continue]
    }
}

naked void camera_fov_in_battle_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[esi+0x000000E8]
			addss xmm0,[CameraSettings::camera_fov_in_battle]
			jmp dword ptr [CameraSettings::camera_fov_in_battle_continue]

        code:
            movss xmm0,[esi+0x000000E8]
            jmp dword ptr [CameraSettings::camera_fov_in_battle_continue]
    }
}

naked void camera_fov_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled], 1
            je customcode
            cmp byte ptr [PhotoMode::photo_mode_open], 1
            je customcode
            jmp code

        customcode:
			movss xmm0,[esi+0x000000E4]
			addss xmm0,[CameraSettings::camera_fov]
			jmp dword ptr [CameraSettings::camera_fov_continue]

        code:
            movss xmm0,[esi+0x000000E4]
            jmp dword ptr [CameraSettings::camera_fov_continue]
    }
}

// I tried and failed, I need to find a better camera rotation value to compare to
/*bool FindOutIfRightIsCloser(float playerFacing, float cameraFacing) {
    const float PI = 3.14159265358979323846f;
    float angleDiff = cameraFacing - playerFacing;
    
    // Normalize the angle difference to be within -PI to PI
    while (angleDiff > PI) angleDiff -= 2 * PI;
    while (angleDiff < -PI) angleDiff += 2 * PI;
    
    // If the absolute difference is positive, right is closer
    return angleDiff > 0;
}

naked void camera_reset_keyboard_proc(void) {
    __asm {
        cmp byte ptr [CameraSettings::camera_reset_enabled], 0
        je originalcode

        push eax
        push dword ptr [edx+0x1210] // player facing
        push dword ptr [esi+0x268]  // camera facing
        call FindOutIfRightIsCloser
        add esp, 8
        test al, al
        pop eax
        jnz RightIsCloser

        movss xmm0, [edx+0x00001210]
        subss xmm0, [degrees] // 1.57
        movss [esi+0x260], xmm0
        jmp retcode

    RightIsCloser:
        movss xmm0, [edx+0x00001210]
        addss xmm0, [degrees] // 1.57
        movss [esi+0x260], xmm0
        jmp retcode

    originalcode:
        movss xmm0, [edx+0x00001210]
    retcode:
        jmp dword ptr [CameraSettings::camera_reset_keyboard_continue]
    }
}

naked void camera_reset_proc(void) {
    __asm {
        cmp byte ptr [CameraSettings::camera_reset_enabled], 0
        je originalcode

        push eax
        push dword ptr [edx+0x1210] // player facing
        push dword ptr [esi+0x268]  // camera facing
        call FindOutIfRightIsCloser
        add esp, 8
        test al, al
        pop eax
        jnz RightIsCloser

        movss xmm0, [edx+0x00001210]
        subss xmm0, [degrees] // 1.57
        jmp retcode

    RightIsCloser:
        movss xmm0, [edx+0x00001210]
        addss xmm0, [degrees] // 1.57
        jmp retcode

    originalcode:
        movss xmm0, [edx+0x00001210]
    retcode:
        jmp dword ptr [CameraSettings::camera_reset_continue]
    }
}*/

naked void camera_reset_keyboard_proc(void) {
    _asm {
		    cmp byte ptr [CameraSettings::camera_reset_enabled], 0
		    je originalcode

            cmp byte ptr [CameraSettings::cam_right], 1
            je camright

        //camleft:
            movss xmm0, [edx+0x00001210]
            subss xmm0, [degrees] // 90 degrees left
            jmp retcode

        camright:
            movss xmm0, [edx+0x00001210]
            addss xmm0, [degrees] // 90 degrees right
            jmp retcode

        originalcode:
            movss xmm0, [edx+0x00001210]
        retcode:
            jmp dword ptr [CameraSettings::camera_reset_keyboard_continue]
    }
}

naked void camera_reset_proc(void) {
    _asm {
		    cmp byte ptr [CameraSettings::camera_reset_enabled], 0
		    je originalcode

            cmp byte ptr [CameraSettings::cam_right], 1
            je camright

        //camleft:
            movss xmm0, [edx+0x00001210]
            subss xmm0, [degrees] // 90 degrees left
            jmp retcode

        camright:
            movss xmm0, [edx+0x00001210]
            addss xmm0, [degrees] // 90 degrees right
            jmp retcode

        originalcode:
            movss xmm0, [edx+0x00001210]
        retcode:
            jmp dword ptr [CameraSettings::camera_reset_continue]
    }
}

naked void camera_sens_clockwise_proc(void) {
    _asm {
            cmp byte ptr [camera_sens_enabled], 0
            je code

            mulss xmm4, [double_camera_sens]
        code:
            movss [esi+0x00000268], xmm4
            jmp dword ptr [CameraSettings::camera_sens_clockwise_continue]
    }
}

naked void camera_sens_anti_clockwise_proc(void) {
    _asm {
            cmp byte ptr [camera_sens_enabled], 0
            je code

            mulss xmm6, [double_camera_sens]
        code:
            movss [esi+0x00000268], xmm6
            jmp dword ptr [CameraSettings::camera_sens_anti_clockwise_continue]
    }
}

naked void camera_sens_brakes_proc(void) {
    _asm {
            cmp byte ptr [camera_sens_enabled], 0
            je code

            mulss xmm6, [double_camera_sens]
        code:
            subss xmm0, xmm6
            movss [esi+0x00000268], xmm0
            jmp dword ptr [CameraSettings::camera_sens_brakes_continue]
    }
}

void CameraSettings::toggle_attack_towards_cam(bool toggle) {
    if (toggle) {
        install_patch_offset(camera_towards_auto_correct1, attack_towards_cam_patch1, "\xE9\x21\x03\x00\x00\x90", 6); // jmp DevilMayCry4_DX9.exe+1983A
    }
    else {
        attack_towards_cam_patch1.reset(); // jne DevilMayCry4_DX9.exe+1983A
    }
}

void CameraSettings::toggle_camera_lookdown(bool toggle) {
    if (toggle) {
        install_patch_offset(0x132483, patch_camera_lookdown, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch_camera_lookdown.reset();
    }
}

void CameraSettings::toggle_disable_last_enemy_zoom(bool toggle) {
    if (toggle) {
        install_patch_offset(0x1A4DA, camera_disable_last_enemy_zoom_patch, "\xEB\x50", 2); // jmp
    }
    else {
        install_patch_offset(0x1A4DA, camera_disable_last_enemy_zoom_patch, "\x75\x50", 2); // jne
    }
}

void CameraSettings::toggle_force_last_enemy_zoom(bool toggle) {
    if (toggle) {
        install_patch_offset(0x1A4DA, camera_disable_last_enemy_zoom_patch, "\x90\x90", 2); // nop nop
    }
    else {
        install_patch_offset(0x1A4DA, camera_disable_last_enemy_zoom_patch, "\x75\x50", 2); // jne
    }
}

void CameraSettings::toggle_pause_camera(bool toggle) {
    if (toggle) {
        install_patch_offset(0xF960A, patch_pause_camera, "\x90\x90", 2); // nops a call
    } else {
        patch_pause_camera.reset();
    }
}

void CameraSettings::toggle_camera_lockon_corrects(bool toggle) {
    if (toggle) {
        install_patch_offset(0x1A402, camera_lockon_corrects_patch1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x1A27B, camera_lockon_corrects_patch2, "\xEB\x09", 2);
    } else {
        camera_lockon_corrects_patch1.reset(); // push eax, call DevilMayCry4_DX9.exe+1F120
        camera_lockon_corrects_patch2.reset(); // jna DevilMayCry4_DX9.exe+1A286
    }
}

void CameraSettings::reset_camera_variables() {
        camera_height = 0.0f;
        camera_distance = 0.0f;
        camera_distance_lockon = 0.0f;
        camera_angle = 0.0f;
        camera_angle_lockon = 0.0f;
        camera_fov = 0.0f;
        camera_fov_in_battle = 0.0f;
        pause_camera_enabled = false;
        toggle_pause_camera(pause_camera_enabled);
}

void CameraSettings::on_gui_frame(int display) {
    ImGui::PushID(display);
    if (display == 1) {
        if (ImGui::Checkbox(_("Disable Lockon Autocorrects"), &camera_lockon_corrects)) {
            toggle_camera_lockon_corrects(camera_lockon_corrects);
        }
        ImGui::SameLine();
        help_marker(_("Attempts to stop the camera spinning behind Dante when enemies are more than like 2ft away"));
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Disable Attack Corrects"), &camera_auto_correct_towards_cam_enabled)) {
            toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Attempts to stop the camera rotating when the player attacks towards the camera, intended for comparison purposes"));

        if (ImGui::Checkbox(_("Disable Last Enemy Zoom"), &disable_last_enemy_zoom)) {
            force_last_enemy_zoom = false;
            toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
        }
        ImGui::SameLine();
        help_marker(_("Disable the zoom-in that happens when you're fighting the last enemy of a room"));
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Force Last Enemy Zoom"), &force_last_enemy_zoom)) {
            disable_last_enemy_zoom = false;
            toggle_force_last_enemy_zoom(force_last_enemy_zoom);
        }

        ImGui::BeginGroup();
        ImGui::Checkbox(_("Side Reset"), &camera_reset_enabled);
        ImGui::SameLine();
        help_marker(_("When pressing camera reset, the camera will be set to the player's left"));
        if (camera_reset_enabled) {
            ImGui::Indent(lineIndent);
            ImGui::Checkbox(_("Right Side Reset"), &cam_right);
            ImGui::SameLine();
            help_marker(_("Set the camera to the right instead"));
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Increased Sensitivity"), &camera_sens_enabled);
        ImGui::SameLine();
        help_marker(_("Increases the camera rotation speed"));

        if (ImGui::Checkbox(_("Camera Lookdown"), &camera_lookdown_enabled)) {
            toggle_camera_lookdown(camera_lookdown_enabled);
        }
        ImGui::SameLine();
        help_marker(_("When above the locked on enemy the camera will look down"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Enable Camera Settings And Hotkeys"), &mod_enabled);
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::CollapsingHeader(_("Camera Settings"))) {
                float availX = ImGui::GetContentRegionAvail().x;
                ImGui::PushItemWidth(availX);
                ImGui::Text(_("Height"));
                ImGui::InputFloat("##Height InputFloat", &CameraSettings::camera_height, 10.0f, 20.0f, "%.0f");
                ImGui::Text(_("Distance"));
                ImGui::InputFloat("##Distance InputFloat", &CameraSettings::camera_distance, 100.0f, 200.0f, "%.0f%");
                ImGui::Text(_("Distance (Lockon)"));
                ImGui::InputFloat("##Distance (Lockon) InputFloat", &CameraSettings::camera_distance_lockon, 100.0f, 200.0f, "%.0f%");
                ImGui::Text(_("Angle"));
                ImGui::InputFloat("##Angle InputFloat", &CameraSettings::camera_angle, 0.1f, 0.2f, "%.1f%");
                ImGui::Text(_("Angle (Lockon) "));
                ImGui::InputFloat("##Angle (Lockon) InputFloat", &CameraSettings::camera_angle_lockon, 0.1f, 0.2f, "%.1f%");
                ImGui::Text(_("FOV"));
                ImGui::InputFloat("##FOV InputFloat", &CameraSettings::camera_fov, 10.0f, 20.0f, "%.0f%");
                ImGui::Text(_("FOV (In Battle)"));
                ImGui::InputFloat("##FOV (In Battle) InputFloat", &CameraSettings::camera_fov_in_battle, 10.0f, 20.0f, "%.0f%");
                if (ImGui::Checkbox(_("Pause Camera##Pause Camera Checkbox"), &pause_camera_enabled)) {
                    toggle_pause_camera(pause_camera_enabled);
                }
                if (ImGui::Button(_("Reset Camera Variables"))) {
                    CameraSettings::reset_camera_variables();
                }
                ImGui::PopItemWidth();
            }
            ImGui::Unindent();
        }
    }
    else if (display == 3) {
        ImGui::BeginGroup();
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::Text(_("Gameplay Cam Settings"));
        float fontSize = ImGui::GetFontSize();
        ImVec2 vSliderSize(fontSize * 2.0f, fontSize * 10.0f);
        ImGui::VSliderFloat(_("Height"), vSliderSize, &CameraSettings::camera_height, -500.0f, 500.0f, "%.0f%");
        ImGui::SameLine();
        if (ImGui::VSliderFloat(_("Distance"), vSliderSize, &CameraSettings::camera_distance, 1000.0f, -1000.0f, "%.0f%")) {
            CameraSettings::camera_distance_lockon = CameraSettings::camera_distance;
        }
        ImGui::SameLine();
        if (ImGui::VSliderFloat(_("Angle"), vSliderSize, &CameraSettings::camera_angle, -1.7f, 1.3f, "%.2f%")) {
            CameraSettings::camera_angle_lockon = CameraSettings::camera_angle;
        }
        ImGui::SameLine();
        if (ImGui::VSliderFloat(_("FOV"), vSliderSize, &CameraSettings::camera_fov, 100.0f, -100.0f, "%.0f%")) {
            CameraSettings::camera_fov_in_battle = CameraSettings::camera_fov;
        }

        if (ImGui::Checkbox(_("Disable Lockon Autocorrects"), &camera_lockon_corrects)) {
            toggle_camera_lockon_corrects(camera_lockon_corrects);
        }
        ImGui::SameLine();
        help_marker(_("Attempts to stop the camera spinning behind Dante when enemies are more than like 2ft away"));

        if (ImGui::Checkbox(_("Disable Attack Corrects"), &camera_auto_correct_towards_cam_enabled)) {
            toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Attempts to stop the camera rotating when the player attacks towards the camera, intended for comparison purposes"));
        if (ImGui::Checkbox(_("Pause Camera##Pause Camera Checkbox"), &pause_camera_enabled)) {
            toggle_pause_camera(pause_camera_enabled);
        }
        if (ImGui::Button(_("Reset Camera Variables"))) {
            CameraSettings::reset_camera_variables();
        }
        ImGui::PopItemWidth();
        ImGui::EndGroup();
    }
    ImGui::PopID();
}

std::optional<std::string> CameraSettings::on_initialize() {
    if (!install_hook_offset(0x0191C5, hook1, &camera_height_proc, &CameraSettings::camera_height_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01946C, hook2, &camera_distance_proc, &CameraSettings::camera_distance_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01A140, hook3, &camera_distance_lockon_proc, &CameraSettings::camera_distance_lockon_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x01914C, hook4, &camera_angle_proc, &CameraSettings::camera_angle_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x0198E6, hook5, &camera_angle_lockon_proc, &CameraSettings::camera_angle_lockon_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x0180EA, hook6, &camera_fov_in_battle_proc, &CameraSettings::camera_fov_in_battle_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x018193, hook7, &camera_fov_proc, &CameraSettings::camera_fov_continue, 8)) {
        spdlog::error("Failed to init CameraSettings mod\n");
        return "Failed to init CameraSettings mod";
    }

    if (!install_hook_offset(0x02261A, camera_reset_hook, &camera_reset_proc, &CameraSettings::camera_reset_continue, 8)) {
        spdlog::error("Failed to init CameraReset mod\n");
        return "Failed to init CameraReset mod";
    }

    if (!install_hook_offset(
            0x022681, camera_reset_keyboard_hook, &camera_reset_keyboard_proc, &CameraSettings::camera_reset_keyboard_continue, 8)) {
        spdlog::error("Failed to init CameraReset2 mod\n");
        return "Failed to init CameraReset2 mod";
    }

    if (!install_hook_offset(
            0x0225A4, camera_sens_clockwise_hook, &camera_sens_clockwise_proc, &CameraSettings::camera_sens_clockwise_continue, 8)) {
        spdlog::error("Failed to init CameraSens mod\n");
        return "Failed to init CameraSens mod";
    }

    if (!install_hook_offset(0x0225BB, camera_sens_anti_clockwise_hook, &camera_sens_anti_clockwise_proc,
            &CameraSettings::camera_sens_anti_clockwise_continue, 8)) {
        spdlog::error("Failed to init CameraSens2 mod\n");
        return "Failed to init CameraSens2 mod";
    }

    if (!install_hook_offset(0x022575, camera_sens_brakes_hook, &camera_sens_brakes_proc, &CameraSettings::camera_sens_brakes_continue, 12)) {
        spdlog::error("Failed to init CameraSens3 mod\n");
        return "Failed to init CameraSens3 mod";
    }

    // hotkeys
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD0}, __("Pause Camera"), "pause_camera");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD1}, __("Pan Camera Down"), "pan_camera_down");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD2}, __("Tilt Camera Down"), "tilt_camera_down");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD3}, __("Dolly Camera In"), "dolly_camera_in");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD4}, __("Zoom Camera In"), "zoom_camera_in");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD5}, __("Reset Camera"), "reset_camera");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD6}, __("Zoom Camera Out"), "zoom_camera_out");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD7}, __("Pan Camera Up"), "pan_camera_up");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD8}, __("Tilt Camera Up"), "tilt_camera_up");
    utility::create_keyboard_hotkey(CameraSettings::m_hotkeys, {VK_NUMPAD9}, __("Dolly Camera Out"), "dolly_camera_out");

    return Mod::on_initialize();
}

void CameraSettings::on_update_input(utility::Input& input) {
    if (!mod_enabled) return;
    if (CameraSettings::m_hotkeys[0]->check(input)) {
        CameraSettings::pause_camera_enabled = !CameraSettings::pause_camera_enabled;
        toggle_pause_camera(pause_camera_enabled);
    }
    if (CameraSettings::m_hotkeys[1]->check(input)) {
        CameraSettings::camera_height -= 10.0f;
    }
    if (CameraSettings::m_hotkeys[2]->check(input)) {
        CameraSettings::camera_angle -= 0.1f;
        CameraSettings::camera_angle_lockon -= 0.1f;
    }
    if (CameraSettings::m_hotkeys[3]->check(input)) {
        CameraSettings::camera_distance -= 100.0f;
        CameraSettings::camera_distance_lockon -= 100.0f;
    }
    if (CameraSettings::m_hotkeys[4]->check(input)) {
        CameraSettings::camera_fov -= 10.0f;
        CameraSettings::camera_fov_in_battle -= 10.0f;
    }
    if (CameraSettings::m_hotkeys[5]->check(input)) {
        CameraSettings::reset_camera_variables();
    }
    if (CameraSettings::m_hotkeys[6]->check(input)) {
        CameraSettings::camera_fov += 10.0f;
        CameraSettings::camera_fov_in_battle += 10.0f;
    }
    if (CameraSettings::m_hotkeys[7]->check(input)) {
        CameraSettings::camera_height += 10.0f;
    }
    if (CameraSettings::m_hotkeys[8]->check(input)) {
        CameraSettings::camera_angle += 0.1f;
        CameraSettings::camera_angle_lockon += 0.1f;
    }
    if (CameraSettings::m_hotkeys[9]->check(input)) {
        CameraSettings::camera_distance += 100.0f;
        CameraSettings::camera_distance_lockon += 100.0f;
    }
}


void CameraSettings::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("camera_settings").value_or(false);
    camera_height = cfg.get<float>("camera_height").value_or(0.0f);
    camera_distance = cfg.get<float>("camera_distance").value_or(0.0f);
    camera_distance_lockon = cfg.get<float>("camera_distance_lockon").value_or(0.0f);
    camera_angle = cfg.get<float>("camera_angle").value_or(0.0f);
    camera_angle_lockon = cfg.get<float>("camera_angle_lockon").value_or(0.0f);
    camera_fov = cfg.get<float>("camera_fov").value_or(0.0f);
    camera_fov_in_battle = cfg.get<float>("camera_fov_battle").value_or(0.0f);
    camera_sens_enabled = cfg.get<bool>("increased_camera_sensitivity").value_or(false);
    camera_auto_correct_towards_cam_enabled = cfg.get<bool>("disable_camera_autocorrect_towards_camera").value_or(false);
    if (camera_auto_correct_towards_cam_enabled) toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
    camera_lookdown_enabled = cfg.get<bool>("camera_lookdown").value_or(false);
    if (camera_lookdown_enabled) toggle_camera_lookdown(camera_lookdown_enabled);
    camera_reset_enabled = cfg.get<bool>("camera_reset").value_or(false);
    cam_right = cfg.get<bool>("right_side_reset").value_or(false);
    disable_last_enemy_zoom = cfg.get<bool>("disable_last_enemy_zoom").value_or(false);
    if (disable_last_enemy_zoom) toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
    force_last_enemy_zoom = cfg.get<bool>("force_last_enemy_zoom").value_or(false);
    if (force_last_enemy_zoom) toggle_force_last_enemy_zoom(force_last_enemy_zoom);
    pause_camera_enabled = cfg.get<bool>("pause_camera_enabled").value_or(false);
    if (pause_camera_enabled) toggle_pause_camera(pause_camera_enabled);
    camera_lockon_corrects = cfg.get<bool>("camera_lockon_corrects").value_or(false);
    if (camera_lockon_corrects) toggle_camera_lockon_corrects(camera_lockon_corrects);
}

void CameraSettings::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("camera_settings", mod_enabled);
    cfg.set<float>("camera_height", camera_height);
    cfg.set<float>("camera_distance", camera_distance);
    cfg.set<float>("camera_distance_lockon", camera_distance_lockon);
    cfg.set<float>("camera_angle", camera_angle);
    cfg.set<float>("camera_angle_lockon", camera_angle_lockon);
    cfg.set<float>("camera_fov", camera_fov);
    cfg.set<float>("camera_fov_battle", camera_fov_in_battle);
    cfg.set<bool>("increased_camera_sensitivity", camera_sens_enabled);
    cfg.set<bool>("disable_camera_autocorrect_towards_camera", camera_auto_correct_towards_cam_enabled);
    cfg.set<bool>("camera_lookdown", camera_lookdown_enabled);
    cfg.set<bool>("camera_reset", camera_reset_enabled);
    cfg.set<bool>("right_side_reset", cam_right);
    cfg.set<bool>("disable_last_enemy_zoom", disable_last_enemy_zoom);
    cfg.set<bool>("force_last_enemy_zoom", force_last_enemy_zoom);
    cfg.set<bool>("pause_camera_enabled", pause_camera_enabled);
    cfg.set<bool>("camera_lockon_corrects", camera_lockon_corrects);
}
