#pragma once

#include "../mod.hpp"

class FreezeEnemies : public Mod
{
public:
    FreezeEnemies() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "FreezeEnemies"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    // hl::Hooker hook;
    hl::Patch scarecrowPatch;
    hl::Patch frostPatch;
    hl::Patch armourPatch;
    hl::Patch ghostPatch;
    hl::Patch flyingSwordPatch;
    hl::Patch fishSwordPatch;
    hl::Patch seedPatch;
    hl::Patch assaultPatch;
    hl::Patch dogPatch;
    hl::Patch blitzPatch;
};