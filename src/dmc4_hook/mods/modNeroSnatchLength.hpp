#pragma once

#include "../mod.hpp"

class NeroSnatchLength : public Mod
{
public:
    NeroSnatchLength();

    static bool modEnabled;
    static uintptr_t jmp_ret;

    std::string getModName() override { return "NeroSnatchLength"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};