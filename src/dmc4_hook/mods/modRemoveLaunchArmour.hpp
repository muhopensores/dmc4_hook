#pragma once

#include "../mod.hpp"

class RemoveLaunchArmour : public Mod
{
public:
    RemoveLaunchArmour() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "RemoveLaunchArmour"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch;
};