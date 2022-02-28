#include "modEnemyDT.hpp"

bool EnemyDT::modEnabledNoDT{ false };
bool EnemyDT::modEnabledInstantDT{ false };

std::optional<std::string> EnemyDT::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyDT::toggleNoDT(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x03309DD, patch1, "\x75\x0D", 2);
    }
    else
    {
        patch1.revert();
    }
}
void EnemyDT::toggleInstantDT(bool enable)
{
    if (enable)
    {
        //install_patch_offset(0x03309A1, patch2, "\xEB\x1f", 2); // forces DT on any difficulty
        install_patch_offset(0x03309DF, patch3, "\x0f\x57\xc0\x90\x90", 5);
    }
    else
    {
        //patch2.revert();
        patch3.revert();
    }
}

void EnemyDT::onGUIframe()
{
    if (ImGui::Checkbox("Enemies Don't DT", &modEnabledNoDT))
    {
        modEnabledInstantDT = 0;              // turn off other mod
        toggleInstantDT(modEnabledInstantDT); // update other mod
        toggleNoDT(modEnabledNoDT);
    }
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Enemies DT Instantly", &modEnabledInstantDT))
    {
        modEnabledNoDT = 0;                   // turn off other mod
        toggleNoDT(modEnabledNoDT);           // update other mod
        toggleInstantDT(modEnabledInstantDT);
    }
}

void EnemyDT::onConfigLoad(const utils::Config& cfg)
{
    modEnabledNoDT = cfg.get<bool>("enemy_no_dt").value_or(false);
    toggleNoDT(modEnabledNoDT);
    modEnabledInstantDT = cfg.get<bool>("enemy_instant_dt").value_or(false);
    toggleInstantDT(modEnabledInstantDT);
};

void EnemyDT::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("enemy_no_dt", modEnabledNoDT);
    cfg.set<bool>("enemy_instant_dt", modEnabledInstantDT);
};
