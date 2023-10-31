
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
bool  CameraSettings::noclip_cam_enabled{ false };
bool CameraSettings::camera_lookdown_enabled{ false };
bool CameraSettings::camera_reset_enabled{ false };
bool CameraSettings::cam_right{ false };
bool CameraSettings::disable_last_enemy_zoom{false};
bool CameraSettings::pause_camera_enabled{false};
bool CameraSettings::camera_lockon_corrects{false};

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

void CameraSettings::toggle_noclip_cam(bool toggle) {
    if (toggle) {
        install_patch_offset(0xF9318, patch_noclip_cam1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0xF9334, patch_noclip_cam2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x180C1, patch_noclip_cam3, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch_noclip_cam1.reset();
        patch_noclip_cam2.reset();
        patch_noclip_cam3.reset();
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
        camera_lockon_corrects_patch1.reset();
        camera_lockon_corrects_patch2.reset();
    }
}

void CameraSettings::on_gui_frame() {
    if (ImGui::Checkbox("Disable Lockon Autocorrects", &camera_lockon_corrects)) {
        toggle_camera_lockon_corrects(camera_lockon_corrects);
    }
    ImGui::SameLine();
    help_marker("Attempts to stop the camera spinning behind Dante when enemies are more than like 2ft away");
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Disable Last Enemy Zoom", &disable_last_enemy_zoom)) {
        toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
    }

    if (ImGui::Checkbox("Left Side Reset", &camera_reset_enabled)) {
        cam_right = 0;
    }
    ImGui::SameLine();
    help_marker("When pressing the button that resets the camera behind Dante, the camera will instead be set to Dante's left");
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Right Side Reset", &cam_right)) {
        camera_reset_enabled = 0;
    }
    ImGui::SameLine();
    help_marker("Set the camera to the right instead");

    ImGui::Checkbox("Increased Sensitivity", &camera_sens_enabled);
    ImGui::SameLine();
    help_marker("Activate this before starting a level!");
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Camera Lookdown", &camera_lookdown_enabled)) {
        toggle_camera_lookdown(camera_lookdown_enabled);
    }
    ImGui::SameLine();
    help_marker("When above the locked on enemy the camera will look down");

    if (ImGui::Checkbox("Noclip Cam", &noclip_cam_enabled)) {
        toggle_noclip_cam(noclip_cam_enabled);
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("Disable Attack Corrects", &camera_auto_correct_towards_cam_enabled)) {
        toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
    }
    ImGui::SameLine();
    help_marker("Attempts to stop the camera rotating when the player attacks towards the camera, intended for comparison purposes");

    ImGui::Checkbox("Use Custom Camera Variables", &mod_enabled);
    ImGui::SameLine();
    help_marker("Also enables the editing of these values via hotkeys");
    if (ImGui::CollapsingHeader("Camera Variables")) {
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::InputFloat("Height", &CameraSettings::camera_height, 1.0f, 1.0f, "%.0f");
        ImGui::Spacing();
        ImGui::InputFloat("Distance", &CameraSettings::camera_distance, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("Distance (Lockon)", &CameraSettings::camera_distance_lockon, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("Angle", &CameraSettings::camera_angle, 0.1f, 0.5f, "%.1f%");
        ImGui::Spacing();
        ImGui::InputFloat("Angle (Lockon)", &CameraSettings::camera_angle_lockon, 0.1f, 0.5f, "%.1f%");
        ImGui::Spacing();
        ImGui::InputFloat("FOV", &CameraSettings::camera_fov, 1.0f, 10.0f, "%.0f%");
        ImGui::Spacing();
        ImGui::InputFloat("FOV (In Battle)", &CameraSettings::camera_fov_in_battle, 1.0f, 10.0f, "%.0f%");
        ImGui::PopItemWidth();
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

    if (!install_hook_offset(
            0x022575, camera_sens_brakes_hook, &camera_sens_brakes_proc, &CameraSettings::camera_sens_brakes_continue, 12)) {
        spdlog::error("Failed to init CameraSens3 mod\n");
        return "Failed to init CameraSens3 mod";
    }

    //  hotkeys
    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD0}, "Pause Camera", "pause_camera"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD1}, "Pan Camera Down", "pan_camera_down"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD2}, "Tilt Camera Down", "tilt_camera_down"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD3}, "Dolly Camera In", "dolly_camera_in"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD4}, "Zoom Camera In", "zoom_camera_in"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD5}, "Reset Camera", "reset_camera"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD6}, "Zoom Camera Out", "zoom_camera_out"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD7}, "Pan Camera Up", "pan_camera_up"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD8}, "Tilt Camera Up", "tilt_camera_up"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_NUMPAD9}, "Dolly Camera Out", "dolly_camera_out"));

    return Mod::on_initialize();
}

void CameraSettings::on_update_input(utility::Input& input) {
    
    if (m_hotkeys[0]->check(input)) {
        CameraSettings::pause_camera_enabled = !CameraSettings::pause_camera_enabled;
        toggle_pause_camera(pause_camera_enabled);
    }
    if (m_hotkeys[1]->check(input)) {
        CameraSettings::camera_height -= 10.0f;
    }
    if (m_hotkeys[2]->check(input)) {
        CameraSettings::camera_angle -= 0.1f;
        CameraSettings::camera_angle_lockon -= 0.1f;
    }
    if (m_hotkeys[3]->check(input)) {
        CameraSettings::camera_distance -= 100.0f;
        CameraSettings::camera_distance_lockon -= 100.0f;
    }
    if (m_hotkeys[4]->check(input)) {
        CameraSettings::camera_fov -= 10.0f;
        CameraSettings::camera_fov_in_battle -= 10.0f;
    }
    if (m_hotkeys[5]->check(input)) {
        CameraSettings::camera_height          = 0.0f;
        CameraSettings::camera_distance        = 0.0f;
        CameraSettings::camera_distance_lockon = 0.0f;
        CameraSettings::camera_angle           = 0.0f;
        CameraSettings::camera_angle_lockon    = 0.0f;
        CameraSettings::camera_fov_in_battle   = 0.0f;
        CameraSettings::camera_fov             = 0.0f;
    }
    if (m_hotkeys[6]->check(input)) {
        CameraSettings::camera_fov += 10.0f;
        CameraSettings::camera_fov_in_battle += 10.0f;
    }
    if (m_hotkeys[7]->check(input)) {
        CameraSettings::camera_height += 10.0f;
    }
    if (m_hotkeys[8]->check(input)) {
        CameraSettings::camera_angle += 0.1f;
        CameraSettings::camera_angle_lockon += 0.1f;
    }
    if (m_hotkeys[9]->check(input)) {
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
    toggle_attack_towards_cam(camera_auto_correct_towards_cam_enabled);
    noclip_cam_enabled = cfg.get<bool>("noclip_cam").value_or(false);
    toggle_noclip_cam(noclip_cam_enabled);
    camera_lookdown_enabled = cfg.get<bool>("camera_lookdown").value_or(false);
    toggle_camera_lookdown(camera_lookdown_enabled);
    camera_reset_enabled = cfg.get<bool>("camera_reset").value_or(false);
    cam_right = cfg.get<bool>("right_side_reset").value_or(false);
    disable_last_enemy_zoom = cfg.get<bool>("disable_last_enemy_zoom").value_or(false);
    toggle_disable_last_enemy_zoom(disable_last_enemy_zoom);
    pause_camera_enabled = cfg.get<bool>("pause_camera_enabled").value_or(false);
    toggle_pause_camera(pause_camera_enabled);
    camera_lockon_corrects = cfg.get<bool>("camera_lockon_corrects").value_or(false);
    toggle_camera_lockon_corrects(camera_lockon_corrects);
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
    cfg.set<bool>("noclip_cam", noclip_cam_enabled);
    cfg.set<bool>("camera_lookdown", camera_lookdown_enabled);
    cfg.set<bool>("camera_reset", camera_reset_enabled);
    cfg.set<bool>("right_side_reset", cam_right);
    cfg.set<bool>("disable_last_enemy_zoom", disable_last_enemy_zoom);
    cfg.set<bool>("pause_camera_enabled", pause_camera_enabled);
    cfg.set<bool>("camera_lockon_corrects", camera_lockon_corrects);
}
