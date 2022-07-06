#if 0
#pragma once

#include "../mod.hpp"

class EnemyReplace : public Mod {
public:
    EnemyReplace() = default;

    static bool modEnabled;

    void ReplaceEnemyDefault(uintptr_t address);
    void ReplaceEnemyOne(uintptr_t address);
    void ReplaceEnemyTwo(uintptr_t address);
    void ReplaceEnemyThree(uintptr_t address);
    void ReplaceEnemyFour(uintptr_t address);
    void ReplaceEnemyFive(uintptr_t address);
    void ReplaceEnemySix(uintptr_t address);
    void ReplaceEnemySeven(uintptr_t address);
    void ReplaceEnemyEight(uintptr_t address);
    void ReplaceEnemyNine(uintptr_t address);
    void ReplaceEnemyTen(uintptr_t address);
    void ReplaceEnemyEleven(uintptr_t address);
    void ReplaceEnemyTwelve(uintptr_t address);
    void ReplaceEnemyThirteen(uintptr_t address);
    void ReplaceEnemyFourteen(uintptr_t address);
    void ReplaceEnemyFifteen(uintptr_t address);
    void ReplaceEnemySixteen(uintptr_t address);
    void ReplaceEnemySeventeen(uintptr_t address);
    void ReplaceEnemyEighteen(uintptr_t address);
    void ReplaceEnemyNineteen(uintptr_t address);

    std::string getModName() override { return "EnemyReplace"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utility::Config& cfg) override;
    void onConfigSave(utility::Config& cfg) override;
    void onGUIframe() override;

private:
    std::unique_ptr<Patch> replaceScarecrowLeg_patch, replaceScarecrowArm_patch, replaceMegaScarecrow_patch, replacementAddressTwo_patch,
        replaceBiancoAngelo_patch, replaceAltoAngelo_patch, replaceMephisto_patch, replaceFaust_patch, replaceFrost_patch,
        replaceAssault_patch, replaceBlitz_patch, replaceChimera_patch, replaceBasilisk_patch, replaceBerial_patch, replaceBael_patch,
        replaceEchidna_patch, replaceCredo_patch, replaceAgnus_patch;
};
#endif
