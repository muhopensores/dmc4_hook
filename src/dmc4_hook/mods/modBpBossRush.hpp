#pragma once

#include "../mod.hpp"

class BpBossRush : public Mod
{
public:
    BpBossRush() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "BpBossRush"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
    hl::Patch patch4;
    hl::Patch patch5;
    hl::Patch patch6;
    hl::Patch patch7;
};