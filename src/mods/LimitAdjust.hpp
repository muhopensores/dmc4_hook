#pragma once

#include "../mod.hpp"

class LimitAdjust : public Mod {
public:
    LimitAdjust() = default;

    static bool gun_sword;
    static bool enemy_step;
    static bool enemy_step_lower;
    static bool style;
    static bool target;
    static bool same_tick_style_change;

    static float limit;
    static float whirlingLimit;

    void f_sword_and_gun(bool enable);
    void f_enemy_step(bool enable);
    void f_enemy_step_lower(bool enable);
    void f_style(bool enable);
    void f_target(bool enable);
    void f_same_tick_style_change(bool enable);

    std::string get_mod_name() override { return "LimitAdjust"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patchsword;
    std::unique_ptr<Patch> patchgun;
    std::unique_ptr<Patch> patchenemystep;
    std::unique_ptr<Patch> patchstyle;
    std::unique_ptr<Patch> patchtarget;
    std::unique_ptr<Patch> patchsametickstylechange;
};
