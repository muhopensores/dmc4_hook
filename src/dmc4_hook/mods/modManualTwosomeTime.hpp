#pragma once

#include "../mod.hpp"

class ManualTwosomeTime : public Mod
{
public:
    ManualTwosomeTime() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "ManualTwosomeTime"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
};