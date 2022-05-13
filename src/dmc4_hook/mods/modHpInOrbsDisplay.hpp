#pragma once

#include "../mod.hpp"

class HpInOrbsDisplay : public Mod
{
public:
    HpInOrbsDisplay() = default;

    static bool modEnabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    std::string getModName() override { return "HpInOrbsDisplay"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};