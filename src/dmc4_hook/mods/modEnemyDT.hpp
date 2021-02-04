#pragma once

#include "../mod.hpp"

class EnemyDT : public Mod
{
public:
    EnemyDT() = default;

    static bool modEnabledNoDT;
    static bool modEnabledInstantDT;

    void toggleNoDT(bool enable);
    void toggleInstantDT(bool enable);

    std::string getModName() override { return "EnemyDT"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
};