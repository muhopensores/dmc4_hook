#pragma once

#include "../mod.hpp"

class EnemyReplaceAgain : public Mod
{
public:
    EnemyReplaceAgain() = default;

    static bool modEnabled;
    hl::Patch* GetPatch(int enemyID);
    uintptr_t GetEnemyAddress(int enemyID);
    void ReplaceEnemyWith(int currentEnemyID, int desiredEnemyID);

    static int desired_enemy[];
    static int default_enemy[];
    
    std::string getModName() override { return "EnemyReplaceAgain"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch replacementAddressTwo_patch,
              replaceScarecrowLeg_patch,
              replaceScarecrowArm_patch,
              replaceScarecrowMega_patch,
              replaceAngeloBianco_patch,
              replaceAngeloAlto_patch,
              replaceMephisto_patch,
              replaceFaust_patch,
              replaceFrost_patch,
              replaceAssault_patch,
              replaceBlitz_patch,
              replaceChimera_patch,
              replaceBasilisk_patch,
              replaceBerial_patch,
              replaceBael_patch,
              replaceEchidna_patch,
              replaceCredo_patch,
              replaceAgnus_patch,
              replaceSanctus_patch,
              replaceKyrie_patch,
              replaceDante_patch;
};