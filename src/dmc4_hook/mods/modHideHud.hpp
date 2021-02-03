#pragma once

#include "../mod.hpp"

class HideHud : public Mod
{
public:
    HideHud() = default;

    static bool modEnabledHealth;
    static bool modEnabledOrbs;
    static bool modEnabledStyle;
    static bool modEnabledTimer;

    void toggleHealth(bool enable);
    void toggleOrbs(bool enable);
    void toggleStyle(bool enable);
    void toggleTimer(bool enable);

    std::string getModName() override { return "HideHud"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
//timer
    hl::Patch patchbp;
    hl::Patch patchm12;
    hl::Patch patchm12flash;
    hl::Patch patchbpplus;
};
