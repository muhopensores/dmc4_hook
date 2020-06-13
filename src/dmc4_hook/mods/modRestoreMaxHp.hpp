#pragma once

#include "../mod.hpp"

class RestoreMaxHp : public Mod
{
public:
    RestoreMaxHp() = default;

    static bool modEnabled;
    static uintptr_t _restoreMaxHpContinue;

    std::string getModName() override { return "RestoreMaxHp"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Patch patch;
};