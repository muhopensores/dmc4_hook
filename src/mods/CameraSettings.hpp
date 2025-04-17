#pragma once

#include "../mod.hpp"

class CameraSettings : public Mod {
public:
    CameraSettings() = default;

    static bool mod_enabled;
    static float camera_height;
    static float camera_distance;
    static float camera_distance_lockon;
    static float camera_angle;
    static float camera_angle_lockon;
    static float camera_fov_in_battle;
    static float camera_fov;
    static uintptr_t camera_height_continue;
    static uintptr_t camera_distance_continue;
    static uintptr_t camera_distance_lockon_continue;
    static uintptr_t camera_angle_continue;
    static uintptr_t camera_angle_lockon_continue;
    static uintptr_t camera_fov_in_battle_continue;
    static uintptr_t camera_fov_continue;
    static uintptr_t camera_reset_continue;
    static uintptr_t camera_reset_keyboard_continue;
    static uintptr_t camera_sens_clockwise_continue;
    static uintptr_t camera_sens_anti_clockwise_continue;
    static uintptr_t camera_sens_brakes_continue;
    static bool noclip_cam_enabled;
    static bool camera_lookdown_enabled;
    static bool camera_reset_enabled;
    static bool cam_right;
    static bool disable_last_enemy_zoom;
    static bool force_last_enemy_zoom;
    static bool pause_camera_enabled;
    static bool camera_lockon_corrects;

    std::string get_mod_name() override { return "CameraSettings"; };

    std::optional<std::string> on_initialize() override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void toggle_attack_towards_cam(bool toggle);
    void toggle_camera_lookdown(bool toggle);
    void toggle_disable_last_enemy_zoom(bool toggle);
    void toggle_force_last_enemy_zoom(bool toggle);
    void toggle_pause_camera(bool toggle);
    void toggle_camera_lockon_corrects(bool toggle);
    void reset_camera_variables();

private:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
    std::unique_ptr<FunctionHook> hook4;
    std::unique_ptr<FunctionHook> hook5;
    std::unique_ptr<FunctionHook> hook6;
    std::unique_ptr<FunctionHook> hook7;
    std::unique_ptr<FunctionHook> camera_reset_hook;
    std::unique_ptr<FunctionHook> camera_reset_keyboard_hook;
    std::unique_ptr<FunctionHook> camera_sens_clockwise_hook;
    std::unique_ptr<FunctionHook> camera_sens_anti_clockwise_hook;
    std::unique_ptr<FunctionHook> camera_sens_brakes_hook;
    std::unique_ptr<Patch> attack_towards_cam_patch1;
    std::unique_ptr<Patch> patch_camera_lookdown;
    std::unique_ptr<Patch> camera_disable_last_enemy_zoom_patch;
    std::unique_ptr<Patch> patch_pause_camera;
    std::unique_ptr<Patch> camera_lockon_corrects_patch1;
    std::unique_ptr<Patch> camera_lockon_corrects_patch2;
};
