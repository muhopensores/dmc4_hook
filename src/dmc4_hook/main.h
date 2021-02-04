#pragma once

#include "hacklib/Main.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/Input.h"
#include "utils/Config.hpp"

class Mods;

class hlMain* GetMain();

class hlMain : public hl::Main
{
public:

	const auto& getMods() const {
		return m_mods;
	}
	bool isReady() const {
		return m_gameDataInitialized;
	}

	std::unique_ptr<utils::Config> cfg;
	void ToggleStuff();
    // patch functions definitons
    void ToggleRemoveLaunchArmour(bool toggle);
    void ToggleSprintFasterActivate(bool toggle);
    void ToggleEnemyAttackOffscreen(bool toggle);
    void ToggleSlowWalk(bool toggle);
    void ToggleAutoSkipIntro(bool toggle);
    void ToggleAutoSkipOutro(bool toggle);
    void ToggleInfiniteTableHopper(bool toggle);

	//imgui toggle functions definitons
    
	//functions
    void SetDefault();
    void SetDMD();
    void SetGMD();
    void GamePause();
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
    void ReplaceEnemySeventeeen(uintptr_t address);
	void ReplaceEnemyEighteen(uintptr_t address);
    void ReplaceEnemyNineteen(uintptr_t address);


    // hacklib init() / step() functions
    bool init() override;
    bool step() override;
	void shutdown() override;

    // define our pointers here, but init and search for them in init()
    // pointer definitions
    uintptr_t modBase;
    uintptr_t difficultySelectOne;
    uintptr_t difficultySelectTwo;
    uintptr_t difficultySelectThree;
    uintptr_t difficultySelectFour;
    uintptr_t difficultySelectFive;
    uintptr_t difficultySelectSix;
    uintptr_t difficultySelectSeven;
    uintptr_t difficultySelectEight;
    uintptr_t difficultySelectNine;
    uintptr_t difficultySelectTen;
    uintptr_t difficultySelectEleven;
    uintptr_t difficultySelectTwelve;
    uintptr_t difficultySelectThirteen;
    uintptr_t difficultySelectFourteen;
    uintptr_t difficultySelectFifteen;
    uintptr_t difficultySelectSixteen;
    uintptr_t difficultySelectSeventeen;
    uintptr_t difficultySelectEighteen;
    uintptr_t difficultySelectNineteen;
    uintptr_t difficultySelectTwenty;
    uintptr_t difficultySelectTwentyOne;
    uintptr_t difficultySelectTwentyTwo;
    uintptr_t difficultySelectTwentyThree;
    uintptr_t difficultySelectTwentyFour;
    uintptr_t difficultySelectTwentyFive;
    uintptr_t difficultySelectTwentySix;
    uintptr_t difficultySelectTwentySeven;
    uintptr_t difficultySelectTwentyEight;
    uintptr_t difficultySelectTwentyNine;
    uintptr_t difficultySelectThirty;
    uintptr_t difficultySelectThirtyOne;
    uintptr_t difficultySelectThirtyTwo;
    uintptr_t difficultySelectThirtyThree;
    uintptr_t difficultySelectThirtyFour;
    uintptr_t difficultySelectThirtyFive;
    uintptr_t difficultySelectThirtySix;
    uintptr_t difficultySelectThirtySeven;
    uintptr_t difficultySelectThirtyEight;
    uintptr_t difficultySelectThirtyNine;
    uintptr_t difficultySelectForty;
    uintptr_t difficultySelectFortyOne;
    uintptr_t difficultySelectFortyTwo;
    uintptr_t difficultySelectFortyThree;
    uintptr_t difficultySelectFortyFour;
    uintptr_t difficultySelectFortyFive;
    uintptr_t difficultySelectFortySix;
    uintptr_t difficultySelectFortySeven;
    uintptr_t difficultySelectFortyEight;
    uintptr_t difficultySelectFortyNine;
    uintptr_t removeLaunchArmour;
    uintptr_t sprintFasterActivate;
    uintptr_t enemyAttackOffscreen;
    uintptr_t slowWalkOne;
    uintptr_t slowWalkTwo;
    uintptr_t autoSkipIntro;
    uintptr_t autoSkipOutro;
    uintptr_t replaceScarecrowLeg;
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
    uintptr_t infiniteTableHopper;


    hl::Patch difficultySelectOne_patch, difficultySelectTwo_patch, difficultySelectThree_patch,
        difficultySelectFour_patch, difficultySelectFive_patch, difficultySelectSix_patch, difficultySelectSeven_patch,
        difficultySelectEight_patch, difficultySelectNine_patch, difficultySelectTen_patch,
        difficultySelectEleven_patch, difficultySelectTwelve_patch, difficultySelectThirteen_patch,
        difficultySelectFourteen_patch, difficultySelectFifteen_patch, difficultySelectSixteen_patch,
        difficultySelectSeventeen_patch, difficultySelectEighteen_patch, difficultySelectNineteen_patch,
        difficultySelectTwenty_patch, difficultySelectTwentyOne_patch, difficultySelectTwentyTwo_patch,
        difficultySelectTwentyThree_patch, difficultySelectTwentyFour_patch, difficultySelectTwentyFive_patch,
        difficultySelectTwentySix_patch, difficultySelectTwentySeven_patch, difficultySelectTwentyEight_patch,
        difficultySelectTwentyNine_patch, difficultySelectThirty_patch, difficultySelectThirtyOne_patch,
        difficultySelectThirtyTwo_patch, difficultySelectThirtyThree_patch, difficultySelectThirtyFour_patch,
        difficultySelectThirtyFive_patch, difficultySelectThirtySix_patch, difficultySelectThirtySeven_patch,
        difficultySelectThirtyEight_patch, difficultySelectThirtyNine_patch, difficultySelectForty_patch,
        difficultySelectFortyOne_patch, difficultySelectFortyTwo_patch, difficultySelectFortyThree_patch,
        difficultySelectFortyFour_patch, difficultySelectFortyFive_patch, difficultySelectFortySix_patch,
        difficultySelectFortySeven_patch, difficultySelectFortyEight_patch, difficultySelectFortyNine_patch,
        removeLaunchArmour_patch, sprintFasterActivate_patch,
        enemyAttackOffscreen_patch, slowWalkOne_patch, slowWalkTwo_patch, autoSkipIntro_patch,
        replaceScarecrowLeg_patch, replaceScarecrowArm_patch, replaceMegaScarecrow_patch, replacementAddressTwo_patch,
        replaceBiancoAngelo_patch, replaceAltoAngelo_patch, replaceMephisto_patch, replaceFaust_patch,
        replaceFrost_patch, replaceAssault_patch, replaceBlitz_patch, replaceChimera_patch, replaceBasilisk_patch,
        replaceBerial_patch, replaceBael_patch, replaceEchidna_patch, replaceCredo_patch, replaceAgnus_patch,
        autoSkipOutro_patch, infiniteTableHopper_patch;


    const hl::IHook* g_PresentHook;
    const hl::IHook* g_EndSceneHook;
    const hl::IHook* g_ResetHook;
    hl::Hooker m_hooker, m_hook;
    hl::Input input;
    hl::ConsoleEx m_con;

	void loadSettings();
	void saveSettings();

private:
	std::atomic<bool> m_gameDataInitialized{ false };
	std::unique_ptr<Mods> m_mods;
	std::string m_confPath;
};
