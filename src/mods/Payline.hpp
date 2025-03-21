#pragma once

#include "../mod.hpp"

class Payline : public Mod {
public:
    Payline() = default;

    static bool mod_enabled;
    static bool helm_splitter_remap;

    static uintptr_t full_house;
    static uintptr_t nero_rave;
    static uintptr_t nero_streak_1;
    static uintptr_t helm_splitter_directional;
    static uintptr_t double_down_directional;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_jne5;
    static uintptr_t jmp_ret6;
    static uintptr_t jmp_ret7;
    static uintptr_t jmp_ret8;
    static uintptr_t jmp_ret9;
    static uintptr_t jmp_ret10;

    // void toggle(bool enable);
    static void Payline_Toggle();
    static void Helm_Splitter_Toggle();

    std::string get_mod_name() override { return "Payline"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
    // void onUpdateInput(utility::Input& input) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5, hook6, hook7, hook8, hook9, hook10;
};

// for che
enum DmC4ButtonMappings {
    LOCKON_MELEE               = 0,
    LOCKON_GUN                 = 1,
    LOCKON_STYLE               = 2,
    LOCKON_FORWARD_MELEE       = 3,
    LOCKON_BACK_MELEE          = 4,
    LOCKON_FORWARD_GUN         = 5,
    LOCKON_BACK_GUN            = 6,
    LOCKON_FORWARD_STYLE       = 7,
    LOCKON_BACK_STYLE          = 8,
    LOCKON_FORWARD_MELEE2      = 9,
    LOCKON_BACK_MELEE2         = 10,
    LOCKON_FORWARD_GUN2        = 11,
    LOCKON_BACK_GUN2           = 12,
    LOCKON_FORWARD_STYLE2      = 13,
    LOCKON_BACK_STYLE2         = 14,
    LOCKON_FORWARD_MELEE_STICK = 15,
    LOCKON_BACK_MELEE_STICK    = 16,
    LOCKON_FORWARD_GUN_STICK   = 17,
    LOCKON_BACK_GUN_STICK      = 18,
    LOCKON_FORWARD_STYLE_STICK = 19,
    LOCKON_BACK_STYLE_STICK    = 20,
    LOCKON_ANYDIRECTION_MELEE  = 21,
    LOCKON_ANYDIRECTION_GUN    = 22,
    LOCKON_ANYDIRECTION_STYLE  = 23,
    ANYDIRECTION_MELEE         = 24,
    ANYDIRECTION_GUN           = 25,
    ANYDIRECTION_STYLE         = 26,
    // ...
    GUN_CHARGE        = 32,
    GUN_CHARGE2       = 33,
    LOCKON_GUN_CHARGE = 34,
    GUN_CHARGE_2OR3   = 35,
    GUN_CHARGE3       = 36,
    GUN_CHARGE4       = 37,
    GUN_CHARGE_3      = 38,
    // ...
    LOCKON_BACKFORWARD_MELEE = 82,
    // 83
    GUN = 84
};