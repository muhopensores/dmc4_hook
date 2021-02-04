#pragma once

#include "../mod.hpp"

class DisableDarkslayer : public Mod
{
public:
    DisableDarkslayer() = default;

    static bool modEnabledUp;
    static bool modEnabledDown;
    static bool modEnabledLeft;
    static bool modEnabledRight;

    void toggleUp(bool enable);
    void toggleDown(bool enable);
    void toggleLeft(bool enable);
    void toggleRight(bool enable);

    std::string getModName() override { return "DisableDarkslayer"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
    hl::Patch patch4;
};
