
#include "CameraSettings.hpp"

static bool camera_sens_enabled{ false };
static bool camera_auto_correct_towards_cam_enabled{ false };
bool CameraSettings::mod_enabled{ false };
float CameraSettings::camera_height{ 0 };
float CameraSettings::camera_distance{ 0 };
float CameraSettings::camera_distance_lockon{ 0 };
float CameraSettings::camera_angle{ 0 };
float CameraSettings::camera_angle_lockon{ 0 };
float CameraSettings::camera_fov_in_battle{ 0 };
float CameraSettings::camera_fov{ 0 };
bool  CameraSettings::free_cam_enabled{ false };
bool CameraSettings::camera_lookdown_enabled{ false };
bool CameraSettings::camera_reset_enabled{ false };
bool CameraSettings::cam_right{ false };
bool CameraSettings::disable_last_enemy_zoom{ false };

constexpr ptrdiff_t camera_towards_auto_correct1 = 0x195A5;
constexpr ptrdiff_t camera_towards_auto_correct2 = 0x195F7;
constexpr ptrdiff_t camera_towards_auto_correct3 = 0x19822;

uintptr_t CameraSettings::camera_height_continue{ NULL };
uintptr_t CameraSettings::camera_distance_continue{ NULL };
uintptr_t CameraSettings::camera_distance_lockon_continue{ NULL };
uintptr_t CameraSettings::camera_angle_continue{ NULL };
uintptr_t CameraSettings::camera_angle_lockon_continue{ NULL };
uintptr_t CameraSettings::camera_fov_in_battle_continue{ NULL };
uintptr_t CameraSettings::camera_fov_continue{ NULL };
uintptr_t CameraSettings::camera_reset_keyboard_continue{ NULL };
uintptr_t CameraSettings::camera_reset_continue{ NULL };
uintptr_t CameraSettings::camera_sens_clockwise_continue{ NULL };
uintptr_t CameraSettings::camera_sens_anti_clockwise_continue{ NULL };
uintptr_t CameraSettings::camera_sens_brakes_continue{ NULL };

static float degrees{ 1.57f };
static float double_camera_sens{ 2.0f };

naked void camera_height_proc(void) {
    _asm {
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

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
            cmp byte ptr [CameraSettings::mod_enabled],0
            je code

			movss xmm0,[esi+0x000000E4]
			addss xmm0,[CameraSettings::camera_fov]
			jmp dword ptr [CameraSettings::camera_fov_continue]

        code:
            movss xmm0,[esi+0x000000E4]
            jmp dword ptr [CameraSettings::camera_fov_continue]
    }
}

naked void camera_reset_keyboard_proc(void) {
    _asm {
		    cmp byte ptr [CameraSettings::camera_reset_enabled], 1
		    je camleft
            cmp byte ptr [CameraSettings::cam_right], 1
            je camright
            jmp originalcode

        camleft:
            movss xmm0, [edx+00001210h]
            subss xmm0, [degrees] // 90 degrees left
            jmp [CameraSettings::camera_reset_keyboard_continue]

        camright:
            movss xmm0, [edx+00001210h]
            addss xmm0, [degrees] // 90 degrees right
            jmp [CameraSettings::camera_reset_keyboard_continue]

        originalcode:
            movss xmm0, [edx+00001210h]
            jmp [CameraSettings::camera_reset_keyboard_continue]
    }
}

naked void camera_reset_proc(void) {
    _asm {
		    cmp byte ptr [CameraSettings::camera_reset_enabled], 1 
		    je camleft
            cmp byte ptr [CameraSettings::cam_right], 1
            je camright
            jmp originalcode

        camleft:
            movss xmm0, [edx+00001210h]
            subss xmm0, [degrees] // 90 degrees left
            jmp [CameraSettings::camera_reset_continue]

        camright:
            movss xmm0, [edx+00001210h]
            addss xmm0, [degrees] // 90 degrees right
            jmp [CameraSettings::camera_reset_continue]

        originalcode:
            movss xmm0,[edx+00001210h]
            jmp [CameraSettings::camera_reset_continue]
    }
}

naked void camera_sens_clockwise_proc(void) {
    _asm {
            cmp byte ptr [camera_sens_enabled], 0
            je code

            mulss xmm4, [double_camera_sens]
        code:
            movss [esi+00000268h], xmm4
            jmp dword ptr [CameraSettings::camera_sens_clockwise_continue]
    }
}

naked void camera_sens_anti_clockwise_proc(void) {
    _asm {
            cmp byte ptr [camera_sens_enabled], 0
            je code

            mulss xmm6, [double_camera_sens]
        code:
            movss [esi+00000268h], xmm6
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
            movss [esi+00000268h], xmm0
            jmp dword ptr [CameraSettings::camera_sens_brakes_continue]
    }
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
    
    if (!install_hook_offset(0x022681, camera_reset_keyboard_hook, &camera_reset_keyboard_proc, &CameraSettings::camera_reset_keyboard_continue, 8)) {
        spdlog::error("Failed to init CameraReset2 mod\n");
        return "Failed to init CameraReset2 mod";
    }

    if (!install_hook_offset(0x0225A4, camera_sens_clockwise_hook, &camera_sens_clockwise_proc, &CameraSettings::camera_sens_clockwise_continue, 8)) {
        spdlog::error("Failed to init CameraSens mod\n");
        return "Failed to init CameraSens mod";
    }

    if (!install_hook_offset(0x0225BB, camera_sens_anti_clockwise_hook, &camera_sens_anti_clockwise_proc, &CameraSettings::camera_sens_anti_clockwise_continue, 8)) {
        spdlog::error("Failed to init CameraSens2 mod\n");
        return "Failed to init CameraSens2 mod";
    }

    if (!install_hook_offset(0x022575, camera_sens_brakes_hook, &camera_sens_brakes_proc, &CameraSettings::camera_sens_brakes_continue, 12)) {
        spdlog::error("Failed to init CameraSens3 mod\n");
        return "Failed to init CameraSens3 mod";
    }

    return Mod::on_initialize();
}

void CameraSettings::toggle_attack_towards_cam(bool toggle) {
    if (toggle) {
        install_patch_offset(camera_towards_auto_correct1, attack_towards_cam_patch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(camera_towards_auto_correct2, attack_towards_cam_patch2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(camera_towards_auto_correct3, attack_towards_cam_patch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else {
        attack_towards_cam_patch1.reset();
        attack_towards_cam_patch2.reset();
        attack_towards_cam_patch3.reset();
    }
}

void CameraSettings::toggle_free_cam(bool toggle) {
    if (toggle) {
        install_patch_offset(0xF9318, patch_free_cam1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0xF9334, patch_free_cam2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x180C1, patch_free_cam3, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch_free_cam1.reset();
        patch_free_cam2.reset();
        patch_free_cam3.reset();
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
        install_patch_offset(0x01A4C1, camera_disable_last_enemy_zoom_patch, "\xEB", 1);
    }
    else {
        camera_disable_last_enemy_zoom_patch.reset();
    }
}

void CameraSettings::on_gui_frame() {
    if (ImGui::CollapsingHeader("Camera")) {
        ImGui::Checkbox("Custom Camera Variables", &mod_enabled);
        ImGui::PushItemWidth(224);
        ImGui::InputFloat("Camera Height", &CameraSettings::camera_height, 1.0f, 1.0f, "%.0f");
        ImGui::Spacing();
        ImGui::InputFloat("Camera Distance", &CameraSettings::camera_distance, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("Camera Distance (Lockon)", &CameraSettings::camera_distance_lockon, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("Camera Angle", &CameraSettings::camera_angle, 0.1f, 0.5f, "%.1f%");
        ImGui::Spacing();
        ImGui::InputFloat("Camera Angle (Lockon)", &CameraSettings::camera_angle_lockon, 0.1f, 0.5f, "%.1f%");
        ImGui::Spacing();
        ImGui::InputFloat("Camera FOV", &CameraSettings::camera_fov, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("Camera FOV (In Battle)", &CameraSettings::camera_fov_in_battle, 1.0f, 10.0f, "%.0f%");
        ImGui::PopItemWidth();
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        if (ImGui::Checkbox("Disable Autocorrect When Attacking Camera Direction", &camera_auto_correct_towards_cam_enabled)) {
            toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
        }

        ImGui::Checkbox("Increased Sensitivity", &camera_sens_enabled);
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Disable Last Enemy Zoom", &disable_last_enemy_zoom)) {
            toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
        }

        if (ImGui::Checkbox("Free Cam", &free_cam_enabled)) {
            toggle_free_cam(free_cam_enabled);
        }
        ImGui::SameLine();
        help_marker("Activate this before starting a level! Forces Free Cam and allows it to pass through walls");
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Camera Lookdown", &camera_lookdown_enabled)) {
            toggle_camera_lookdown(camera_lookdown_enabled);
        }
        ImGui::SameLine();
        help_marker("When above the locked on enemy the camera will look down");

        if (ImGui::Checkbox("Left Side Reset", &camera_reset_enabled)) {
            cam_right = 0;
        }
        ImGui::SameLine();
        help_marker("When pressing the button that resets the camera behind Dante, the camera will instead be set to Dante's left");
        ImGui::SameLine(205);
        if (ImGui::Checkbox("Right Side Reset", &cam_right)) {
            camera_reset_enabled = 0;
        }
        ImGui::SameLine();
        help_marker("Set the camera to the right instead");
    }
}

void CameraSettings::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("camera_settings").value_or(false);
    camera_height = cfg.get<float>("camera_height").value_or(0.0f);
    camera_distance = cfg.get<float>("camera_distance").value_or(0.0f);
    camera_distance_lockon = cfg.get<float>("camera_distance_lockon").value_or(0.0f);
    camera_angle = cfg.get<float>("camera_angle").value_or(0.0f);
    camera_angle_lockon = cfg.get<float>("camera_angle_lockon").value_or(0.0f);
    camera_fov = cfg.get<float>("camera_fov_battle").value_or(0.0f);
    camera_sens_enabled = cfg.get<bool>("increased_camera_sensitivity").value_or(false);
    camera_auto_correct_towards_cam_enabled = cfg.get<bool>("disable_camera_autocorrect_towards_camera").value_or(false);
    toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
    free_cam_enabled = cfg.get<bool>("free_cam").value_or(false);
    toggle_free_cam(free_cam_enabled);
    camera_lookdown_enabled = cfg.get<bool>("camera_lookdown").value_or(false);
    toggle_camera_lookdown(camera_lookdown_enabled);
    camera_reset_enabled = cfg.get<bool>("camera_reset").value_or(false);
    cam_right = cfg.get<bool>("right_side_reset").value_or(false);
    disable_last_enemy_zoom = cfg.get<bool>("disable_last_enemy_zoom").value_or(false);
    toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
}

void CameraSettings::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("camera_settings", mod_enabled);
    cfg.set<float>("camera_height", camera_height);
    cfg.set<float>("camera_distance", camera_distance);
    cfg.set<float>("camera_distance_lockon", camera_distance_lockon);
    cfg.set<float>("camera_angle", camera_angle);
    cfg.set<float>("camera_angle_lockon", camera_angle_lockon);
    cfg.set<float>("camera_fov_battle", camera_fov);
    cfg.set<bool>("increased_camera_sensitivity", camera_sens_enabled);
    cfg.set<bool>("disable_camera_autocorrect_towards_camera", camera_auto_correct_towards_cam_enabled);
    cfg.set<bool>("free_cam", free_cam_enabled);
    cfg.set<bool>("camera_lookdown", camera_lookdown_enabled);
    cfg.set<bool>("camera_reset", camera_reset_enabled);
    cfg.set<bool>("right_side_reset", cam_right);
    cfg.set<bool>("disable_last_enemy_zoom", disable_last_enemy_zoom);
}
