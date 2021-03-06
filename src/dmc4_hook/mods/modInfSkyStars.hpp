#pragma once

#include "../mod.hpp"

class InfSkyStars : public Mod
{
public:
    InfSkyStars() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "InfSkyStars"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
};