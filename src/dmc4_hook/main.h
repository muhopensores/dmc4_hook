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

	//imgui toggle functions definitons
    
	//functions
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


    hl::Patch
        replaceScarecrowLeg_patch, replaceScarecrowArm_patch, replaceMegaScarecrow_patch, replacementAddressTwo_patch,
        replaceBiancoAngelo_patch, replaceAltoAngelo_patch, replaceMephisto_patch, replaceFaust_patch,
        replaceFrost_patch, replaceAssault_patch, replaceBlitz_patch, replaceChimera_patch, replaceBasilisk_patch,
        replaceBerial_patch, replaceBael_patch, replaceEchidna_patch, replaceCredo_patch, replaceAgnus_patch;


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
