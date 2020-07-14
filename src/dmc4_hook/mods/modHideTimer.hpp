#pragma once

#include "../mod.hpp"

class HideTimer : public Mod
{
public:
    HideTimer() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "HideTimer"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patchbp;
    hl::Patch patchm12;
    hl::Patch patchm12flash;
    hl::Patch patchbpplus;
};