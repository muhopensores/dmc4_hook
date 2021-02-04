#include "modEnemyReplace.hpp"

bool EnemyReplace::modEnabled{ false };

uintptr_t replaceScarecrowLeg = 0x00000000;
uintptr_t replaceScarecrowArm;
uintptr_t replaceMegaScarecrow;
uintptr_t replacementAddressTwo;
uintptr_t replaceBiancoAngelo;
uintptr_t replaceAltoAngelo;
uintptr_t replaceMephisto;
uintptr_t replaceFaust;
uintptr_t replaceFrost;
uintptr_t replaceAssault;
uintptr_t replaceBlitz;
uintptr_t replaceChimera;
uintptr_t replaceBasilisk;
uintptr_t replaceBerial;
uintptr_t replaceBael;
uintptr_t replaceEchidna;
uintptr_t replaceCredo;
uintptr_t replaceAgnus;

std::optional<std::string> EnemyReplace::onInitialize()
{
    return Mod::onInitialize();
}

void EnemyReplace::ReplaceEnemyDefault(uintptr_t address)
{
    if (address == replaceScarecrowLeg)
    {
        // install_patch_offset(replaceScarecrowLeg, "\x8b\x0d\x4c\x43\xe1\x00", 6); // ?? 
    }
    else
    {
        patch1.revert();
    }
}

void EnemyReplace::onGUIframe()
{
    if (ImGui::Checkbox("Enemy Replacements", &modEnabled))
    {
        // toggle(modEnabled);
    }
}

void EnemyReplace::onConfigLoad(const utils::Config& cfg)
{
    // modEnabled = cfg.get<bool>("enemy_replace").value_or(false);
};

void EnemyReplace::onConfigSave(utils::Config& cfg)
{
    // cfg.set<bool>("enemy_replace", modEnabled);
};
