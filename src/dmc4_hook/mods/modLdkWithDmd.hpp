#pragma once

#include "../mod.hpp"

class LdkWithDmd : public Mod
{
public:
    LdkWithDmd();

    static bool modEnabled;
    static bool modFrostsCantEscapeEnabled;
    static uintptr_t _ldkWithDmdContinue;
    void toggle(bool enable);

    std::string getModName() override { return "LdkWithDmd"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Patch patch;
};