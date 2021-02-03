#pragma once

#include "../mod.hpp"

class DamageMultiplier : public Mod
{
public:
    DamageMultiplier();

    static bool modEnabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;
    static float enemyHPDisplay;

    std::string getModName() override { return "DamageMultiplier"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};