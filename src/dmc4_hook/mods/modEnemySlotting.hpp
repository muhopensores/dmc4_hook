#pragma once

#include "../mod.hpp"

class EnemySlotting : public Mod
{
public:
    EnemySlotting();

    static bool modEnabled;
    static uintptr_t _enemySlottingContinue;  // = 0x00337729;

    std::string getModName() override { return "EnemySlotting"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};