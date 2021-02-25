#pragma once

#include "../mod.hpp"

class DisableChimeraBlades : public Mod
{
public:
    DisableChimeraBlades();

    static bool modEnabled;
    static uintptr_t jmp_ret;

    std::string getModName() override { return "DisableChimeraBlades"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};
