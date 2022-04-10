#pragma once

#include "../mod.hpp"

class NeroFullHouse : public Mod
{
public:
    NeroFullHouse() = default;

    static bool modEnabled;

    static uintptr_t full_house;
    static uintptr_t nero_rave;
    static uintptr_t nero_streak_1;
    static uintptr_t helmSplitterDirectional;
    static uintptr_t doubleDownDirectional;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_jne5;

    std::string getModName() override { return "NeroFullHouse"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    // void onUpdateInput(hl::Input& input) override;

private:
    hl::Hooker hook1;
    hl::Hooker hook2;
    hl::Hooker hook3;
    hl::Hooker hook4;
    hl::Hooker hook5;
};

// for che
enum DMC4_button_mappings {
    LOCKON_MELEE = 0,
    LOCKON_GUN = 1,
    LOCKON_STYLE = 2,
    LOCKON_FORWARD_MELEE = 3,
    LOCKON_BACK_MELEE = 4,
    LOCKON_FORWARD_GUN = 5,
    LOCKON_BACK_GUN = 6,
    LOCKON_FORWARD_STYLE = 7,
    LOCKON_BACK_STYLE = 8,
    LOCKON_FORWARD_MELEE2 = 9,
    LOCKON_BACK_MELEE2 = 10,
    LOCKON_FORWARD_GUN2 = 11,
    LOCKON_BACK_GUN2 = 12,
    LOCKON_FORWARD_STYLE2 = 13,
    LOCKON_BACK_STYLE2 = 14,
    LOCKON_FORWARD_MELEE_STICK = 15,
    LOCKON_BACK_MELEE_STICK = 16,
    LOCKON_FORWARD_GUN_STICK = 17,
    LOCKON_BACK_GUN_STICK = 18,
    LOCKON_FORWARD_STYLE_STICK = 19,
    LOCKON_BACK_STYLE_STICK = 20,
    LOCKON_ANYDIRECTION_MELEE = 21,
    LOCKON_ANYDIRECTION_GUN = 22,
    LOCKON_ANYDIRECTION_STYLE = 23,
    ANYDIRECTION_MELEE = 24,
    ANYDIRECTION_GUN = 25,
    ANYDIRECTION_STYLE = 26,
    // ...
    GUN_CHARGE = 32,
    GUN_CHARGE2 = 33,
    LOCKON_GUN_CHARGE = 34,
    GUN_CHARGE_2OR3 = 35,
    GUN_CHARGE3 = 36,
    GUN_CHARGE4 = 37,
    GUN_CHARGE_3 = 38,
    // ...
    LOCKON_BACKFORWARD_MELEE = 82,
    // 83
    GUN = 84
};
