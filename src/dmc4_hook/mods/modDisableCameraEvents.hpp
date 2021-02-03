#pragma once

#include "../mod.hpp"

class DisableCameraEvents : public Mod
{
public:
    DisableCameraEvents() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "DisableCameraEvents"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
};