#pragma once

#include "../mod.hpp"

class HideHud : public Mod {
public:
    HideHud() = default;

    static bool mod_enabled_hide_all_hud;
    static bool mod_enabled_health;
    static bool mod_enabled_orbs;
    static bool mod_enabled_style;
    static bool mod_enabled_timer;
    static bool mod_enabled_boey;
    static bool mod_enabled_weapon_selected;
    static bool mod_enabled_hide_weapon_selected;
    static bool mod_enabled_map;
    static bool mod_enabled_boss;
    static bool mod_enabled_style_points;

    static uintptr_t boey_hud_15_continue;

    void hide_all_hud(bool enable);
    void toggle_health(bool enable);
    void toggle_orbs(bool enable);
    void toggle_style(bool enable);
    void toggle_timer(bool enable);
    void toggle_boey(bool enable);
    void toggle_weapon_display(bool enable);
    void toggle_weapon_hide(bool enable);
    void toggle_map(bool enable);
    void toggle_boss_hp(bool enable);
    void toggle_style_points(bool enable);

    std::string get_mod_name() override { return "HideHud"; };
    std::optional<std::string> on_initialize() override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patchhealth;

    std::unique_ptr<Patch> patchstyle;

    std::unique_ptr<Patch> patchorbs1;
    std::unique_ptr<Patch> patchorbs2;
    std::unique_ptr<Patch> patchorbs3;
    std::unique_ptr<Patch> patchorbs4;
    std::unique_ptr<Patch> patchorbs5;
    std::unique_ptr<Patch> patchorbs6;
    std::unique_ptr<Patch> patchorbs7;
    std::unique_ptr<Patch> patchorbs8;
    std::unique_ptr<Patch> patchorbs9;
    std::unique_ptr<Patch> patchorbs10;

    std::unique_ptr<Patch> patchbosshud1;
    std::unique_ptr<Patch> patchbosshud2;
    std::unique_ptr<Patch> patchbosshud3;
    std::unique_ptr<Patch> patchbosshud4;
    std::unique_ptr<Patch> patchbosshud5;
    std::unique_ptr<Patch> patchbosshud6;

    // timer
    std::unique_ptr<Patch> patchbp;
    std::unique_ptr<Patch> patchbpplus;
    std::unique_ptr<Patch> patchm12;
    std::unique_ptr<Patch> patchbpstageclearbonus1;
    std::unique_ptr<Patch> patchbpstageclearbonus2;
    // std::unique_ptr<Patch> patchm12flash;

    // boey
    std::unique_ptr<Patch> patchboey01;
    std::unique_ptr<Patch> patchboey02;
    std::unique_ptr<Patch> patchboey03;
    std::unique_ptr<Patch> patchboey04;
    std::unique_ptr<Patch> patchboey05;
    std::unique_ptr<Patch> patchboey06;
    std::unique_ptr<Patch> patchboey07;
    std::unique_ptr<Patch> patchboey08;
    std::unique_ptr<Patch> patchboey09;
    std::unique_ptr<Patch> patchboey10;
    std::unique_ptr<Patch> patchboey11;
    std::unique_ptr<Patch> patchboey12;
    std::unique_ptr<Patch> patchboey13;
    std::unique_ptr<Patch> patchboey14;
    // std::unique_ptr<Patch> patchboey15;
    std::unique_ptr<Patch> patchboey16;
    std::unique_ptr<Patch> patchboey17;
    std::unique_ptr<Patch> patchboey18;
    // std::unique_ptr<Patch> patchboey19;
    std::unique_ptr<Patch> patchboey20;
    std::unique_ptr<Patch> patchboey21;
    std::unique_ptr<Patch> patchboey22;
    std::unique_ptr<Patch> patchboey23;
    std::unique_ptr<Patch> patchboey24;
    std::unique_ptr<Patch> patchboey25;
    std::unique_ptr<Patch> patchboey26;
    std::unique_ptr<Patch> patchboey27;
    std::unique_ptr<Patch> patchboey28;
    std::unique_ptr<Patch> patchboey29;
    std::unique_ptr<Patch> patchboey30;
    std::unique_ptr<Patch> patchboey31;
    std::unique_ptr<Patch> patchboey32;
    std::unique_ptr<Patch> patchboey33;
    std::unique_ptr<Patch> patchboey34;
    std::unique_ptr<Patch> patchboey35;
    std::unique_ptr<Patch> patchboey36;
    std::unique_ptr<Patch> patchboey37;
    std::unique_ptr<Patch> patchboey38;
    std::unique_ptr<Patch> patchboey39;
    std::unique_ptr<Patch> patchboey40;
    std::unique_ptr<Patch> patchboey41;
    std::unique_ptr<Patch> patchboey42;
    std::unique_ptr<Patch> patchboey43;
    std::unique_ptr<Patch> patchboey44;
    std::unique_ptr<Patch> patchboey45;
    std::unique_ptr<Patch> patchboey46;
    std::unique_ptr<Patch> patchboey47;
    std::unique_ptr<Patch> patchboey48;
    std::unique_ptr<Patch> patchboey49;
    std::unique_ptr<Patch> patchboey50;
    std::unique_ptr<Patch> patchboey51;
    std::unique_ptr<Patch> patchboey52;
    std::unique_ptr<Patch> patchboey53;
    std::unique_ptr<Patch> patchboey54;
    std::unique_ptr<Patch> patchboey55;
    std::unique_ptr<Patch> patchboey56;
    std::unique_ptr<Patch> patchboey57;
    std::unique_ptr<Patch> patchboey58;
    std::unique_ptr<Patch> patchboey59;
    std::unique_ptr<Patch> patchboey60;
    std::unique_ptr<Patch> patchboey61;
    std::unique_ptr<Patch> patchboey62;
    std::unique_ptr<Patch> patchboey63;
    std::unique_ptr<Patch> patchboey64;
    std::unique_ptr<Patch> patchboey65;
    std::unique_ptr<Patch> patchboey66;
    std::unique_ptr<Patch> patchboey67;
    std::unique_ptr<Patch> patchboey68;
    std::unique_ptr<Patch> patchboey69;
    std::unique_ptr<Patch> patchboey70;
    std::unique_ptr<Patch> patchboey71;

    std::unique_ptr<FunctionHook> boey_hud_15_hook;

    // weapon selected display
    std::unique_ptr<Patch> patchswordhud;
    std::unique_ptr<Patch> patchgunhud;

    std::unique_ptr<Patch> patchswordhidehud;
    std::unique_ptr<Patch> patchgunhidehud;

    std::unique_ptr<Patch> patchmaphud;

    std::unique_ptr<Patch> patchstylepoints;
};
