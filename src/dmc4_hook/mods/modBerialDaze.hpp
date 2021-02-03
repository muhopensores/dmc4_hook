#pragma once

#include "../mod.hpp"

class BerialDaze : public Mod
{
public:
    BerialDaze();

    static bool modEnabled;
    static uintptr_t jmp_ret;
    void toggle(bool enable);

    std::string getModName() override { return "BerialDaze"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Patch patch;
};