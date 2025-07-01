#pragma once

#include "../mod.hpp"

class LimitAdjust : public Mod {
public:
    LimitAdjust() = default;

    // nero
    static bool target_switch_enabled_nero;
    static bool enemy_step_enabled_nero;
    static bool whirling_enemy_step_enabled_nero;
    static bool no_dt_cooldown_nero;

    // dante
    static bool target_switch_enabled_dante;
    static bool enemy_step_enabled_dante;
    static bool whirling_enemy_step_enabled_dante;
    static bool gun_sword;
    void f_sword_and_gun(bool enable);
    static bool style;
    void f_style(bool enable);
    static bool same_tick_style_change;
    void f_same_tick_style_change(bool enable);
    static bool no_dt_cooldown_dante;

    static uintptr_t target_switch_ret;
    static uintptr_t enemy_step_ret;
    static uintptr_t no_dt_cooldown_jmp_ret;
    static float whirling_jc_limit;
    static float limit;

    std::string get_mod_name() override { return "LimitAdjust"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    // shared
    std::unique_ptr<FunctionHook> hook_target_switch;
    std::unique_ptr<FunctionHook> hook_enemy_step;
    std::unique_ptr<FunctionHook> no_dt_cooldown_hook;

    // dante
    std::unique_ptr<Patch> patchsword;
    std::unique_ptr<Patch> patchgun;
    std::unique_ptr<Patch> patchstyle;
    std::unique_ptr<Patch> patchsametickstylechange;
};
