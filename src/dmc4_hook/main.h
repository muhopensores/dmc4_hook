#pragma once

#include "hacklib/Main.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/Input.h"
#include "utils/Config.hpp"

class WorkRate;
class AreaJump;

class hlMain* GetMain();

class hlMain : public hl::Main
{
public:
	std::unique_ptr<utils::Config> cfg;
	void ToggleStuff();
    // patch functions definitons
    void ToggleStyleSwitch(bool toggle);
    void ToggleWeaponSwitch(bool toggle);
    void ToggleJcCooldown(bool toggle);
    void ToggleMovingTargetChange(bool toggle);
    void ToggleHeightRestrictionDante(bool toggle);
    void ToggleHeightRestrictionNero(bool toggle);
    void ToggleInfiniteTime(bool toggle);
    void ToggleInfiniteAllHealth(bool toggle);
    void ToggleDisableCameraEvents(bool toggle);
    void ToggleHideHUD(bool toggle);
    void ToggleEnemyNoDT(bool toggle);
    void ToggleEnemyInstantDT(bool toggle);
    void ToggleBpPortalAutoOpen(bool toggle);
    void ToggleFastPandora(bool toggle);
    void ToggleInfiniteAirHike(bool toggle);
    void ToggleBerialPractice(bool toggle);
    void ToggleBossRush(bool toggle);
    void ToggleDisableDarkslayerDown(bool toggle);
    void ToggleDisableDarkslayerLeft(bool toogle);
    void ToggleDisableDarkslayerRight(bool toggle);
    void ToggleDisableDarkslayerUp(bool toggle);
    void ToggleStunAnything(bool toggle);
    void ToggleRemoveLaunchArmour(bool toggle);
    void ToggleCharacterChange(bool toggle);
    void ToggleSprintFasterActivate(bool toggle);
    void ToggleEnemyAttackOffscreen(bool toggle);
    void ToggleSlowWalk(bool toggle);
    void ToggleHideStyle(bool toggle);
    void ToggleAutoSkipIntro(bool toggle);
    void ToggleLDKWithDMD(bool toggle);
    void ToggleInfiniteRevive(bool toggle);
    void ToggleAutoSkipOutro(bool toggle);
    void ToggleInfiniteTableHopper(bool toggle);
    void ToggleInfiniteTrickRange(bool toggle);
    void ToggleCameraSensitivity(bool toggle);

	//imgui toggle functions definitons
    void ImGuiToggleInfPlayerHealth();
    void ImGuiToggleInfDT();
    void ImGuiToggleBerialDaze();
    void ImGuiToggleSelectiveCancels();
		void ImGuiToggleEcstasyCancel();
		void ImGuiToggleArgumentCancel();
		void ImGuiToggleKickThirteenCancel();
		void ImGuiToggleSlashDimensionCancel();
		void ImGuiTogglePropCancel();
		void ImGuiToggleShockCancel();
		void ImGuiToggleOmenCancel();
    void ImGuiToggleDamageModifier();
    void ImGuiToggleLDKWithDMD();
    void ImGuiToggleTrackingFullHouse();
    void ImGuiToggleTrickDown();
    void ImGuiToggleHoneyComb();
    void ImGuiToggleOrbDisplay();
    void ImGuiToggleRoseRemovesPins();
    void ImGuiToggleNoHelmBreakerKnockback();
    
	//functions
    void ImGuiInitiateJump();
	void ImGuiStageJump(int room);
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
    uintptr_t styleSwitch;
    uintptr_t swordSwitch;
    uintptr_t gunSwitch;
    uintptr_t jcCooldown;
    uintptr_t movingTargetChange;
    uintptr_t damagemodifier;
    uintptr_t orbDisplay;
    uintptr_t heightRestrictionDante;
    uintptr_t heightRestrictionBuster;
    uintptr_t heightRestrictionSplit;
    uintptr_t heightRestrictionCalibur;
    uintptr_t heightRestrictionExCalibur;
    uintptr_t heightRestrictionSnatch;
    uintptr_t heightRestrictionRaveNero;
	uintptr_t heightRestrictionDoubleDown;
    uintptr_t heightRestrictionRev;
    uintptr_t infiniteTime;
    uintptr_t infiniteAllHealth;
    uintptr_t disablecameraEventsOne;
    uintptr_t disablecameraEventsTwo;
    uintptr_t hideHUDOne;
    uintptr_t hideHUDTwo;
    uintptr_t hideHUDThree;
    uintptr_t enemyNoDT;
    uintptr_t enemyInstantDT;
    uintptr_t enemyInstantDTTwo;
    uintptr_t bpPortalAutoOpen;
    uintptr_t fastPandoraOne;
    uintptr_t fastPandoraTwo;
    uintptr_t fastPandoraThree;
    uintptr_t fastPandoraFour;
    uintptr_t fastPandoraFive;
    uintptr_t fastPandoraSix;
    uintptr_t infiniteAirHikeOne;
    uintptr_t infiniteAirHikeTwo;
    uintptr_t infiniteAirHikeThree;
    uintptr_t infiniteDT;
    uintptr_t infinitePlayerHealth;
    uintptr_t berialDazeOne;
    uintptr_t berialDazeTwo;
    uintptr_t berialCollapse;
    uintptr_t bossRushOne;
    uintptr_t bossRushTwo;
    uintptr_t bossRushThree;
    uintptr_t bossRushFour;
    uintptr_t bossRushFive;
    uintptr_t bossRushSix;
    uintptr_t bossRushSeven;
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
    uintptr_t disableDarkslayerDown;
    uintptr_t disableDarkslayerLeft;
    uintptr_t disableDarkslayerRight;
    uintptr_t disableDarkslayerUp;
    uintptr_t moveIDAlloc;
    uintptr_t selectiveCancels;
    uintptr_t stunAnything;
    uintptr_t removeLaunchArmour;
    uintptr_t characterChangeOne;
    uintptr_t characterChangeTwo;
    uintptr_t characterchangeThree;
    uintptr_t characterchangeFour;
    uintptr_t characterChangeFive;
    uintptr_t characterChangeSix;
    uintptr_t characterChangeSeven;
    uintptr_t characterChangeEight;
    uintptr_t characterChangeNine;
    uintptr_t characterChangeTen;
    uintptr_t characterChangeEleven;
    uintptr_t characterChangeTwelve;
    uintptr_t characterChangeThirteen;
    uintptr_t characterChangeFourteen;
    uintptr_t characterChangeFifteen;
    uintptr_t characterChangeSixteen;
    uintptr_t characterChangeSeventeen;
    uintptr_t characterChangeEighteen;
    uintptr_t characterChangeNineteen;
    uintptr_t characterChangeTwenty;
    uintptr_t characterChangeTwentyOne;
    uintptr_t sprintFasterActivate;
    uintptr_t enemyAttackOffscreen;
    uintptr_t slowWalkOne;
    uintptr_t slowWalkTwo;
    uintptr_t cameraHeightSetting;
    uintptr_t cameraDistanceSetting;
    uintptr_t cameraDistanceLockonSetting;
    uintptr_t cameraAngleSetting;
    uintptr_t hideStyle;
    uintptr_t hideOrbs;
    uintptr_t autoSkipIntro;
    uintptr_t autoSkipOutro;
    uintptr_t ldkWithDMDOne;
    uintptr_t ldkWithDMDTwo;
    uintptr_t infiniteReviveOne;
    uintptr_t infiniteReviveTwo;
    uintptr_t infiniteReviveThree;
    uintptr_t infiniteReviveFour;
    uintptr_t replaceScarecrowLeg;
    uintptr_t replaceScarecrowArm;
    uintptr_t replaceMegaScarecrow;
    uintptr_t replacementAddressTwo;
    uintptr_t replaceBiancoAngelo;
    uintptr_t replaceAltoAngelo;
    uintptr_t hideTimer;
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
    uintptr_t lockOn;
    uintptr_t lockOff;
    uintptr_t trackingFullHouse;
    uintptr_t timerAlloc;
    uintptr_t backForward;
    uintptr_t trickDown;
    uintptr_t floorTouch;
    uintptr_t infiniteTrickRange;
    uintptr_t cameraSensitivity;
    uintptr_t roseRemovesPins;
    uintptr_t noHelmBreakerKnockback;
    float* globalSpeed;
    float* playerSpeed;
    float* enemySpeed;
    float* roomSpeed;
    float* turboValue;
    int* roomID;
    int* bpFloorStage;
    int* initiateJump;


    hl::Patch styleSwitch_patch, swordSwitch_patch, gunSwitch_patch, jcCooldown_patch, movingTargetChange_patch,
        heightRestrictionDante_patch, heightRestrictionBuster_patch, heightRestrictionSplit_patch,
        heightRestrictionCalibur_patch, heightRestrictionExCalibur_patch, heightRestrictionSnatch_patch,
        heightRestrictionRaveNero_patch, infiniteTime_patch, infiniteAllHealth_patch, disablecameraEventsOne_patch,
        disablecameraEventsTwo_patch, hideHUDOne_patch, hideHUDTwo_patch, hideHUDThree_patch, enemyNoDT_patch,
        enemyInstantDT_patch, enemyInstantDTTwo_patch, bpPortalAutoOpen_patch, fastPandoraOne_patch,
        fastPandoraTwo_patch, fastPandoraThree_patch, fastPandoraFour_patch, fastPandoraFive_patch,
        fastPandoraSix_patch, infiniteAirHikeOne_patch, infiniteAirHikeTwo_patch, infiniteAirHikeThree_patch,
        berialDazeOne_patch, berialCollapse_patch, bossRushOne_patch, bossRushTwo_patch, bossRushThree_patch,
        bossRushFour_patch, bossRushFive_patch, bossRushSix_patch, bossRushSeven_patch, difficultySelectOne_patch,
        difficultySelectTwo_patch, difficultySelectThree_patch, difficultySelectFour_patch, difficultySelectFive_patch,
        difficultySelectSix_patch, difficultySelectSeven_patch, difficultySelectEight_patch, difficultySelectNine_patch,
        difficultySelectTen_patch, difficultySelectEleven_patch, difficultySelectTwelve_patch,
        difficultySelectThirteen_patch, difficultySelectFourteen_patch, difficultySelectFifteen_patch,
        difficultySelectSixteen_patch, difficultySelectSeventeen_patch, difficultySelectEighteen_patch,
        difficultySelectNineteen_patch, difficultySelectTwenty_patch, difficultySelectTwentyOne_patch,
        difficultySelectTwentyTwo_patch, difficultySelectTwentyThree_patch, difficultySelectTwentyFour_patch,
        difficultySelectTwentyFive_patch, difficultySelectTwentySix_patch, difficultySelectTwentySeven_patch,
        difficultySelectTwentyEight_patch, difficultySelectTwentyNine_patch, difficultySelectThirty_patch,
        difficultySelectThirtyOne_patch, difficultySelectThirtyTwo_patch, difficultySelectThirtyThree_patch,
        difficultySelectThirtyFour_patch, difficultySelectThirtyFive_patch, difficultySelectThirtySix_patch,
        difficultySelectThirtySeven_patch, difficultySelectThirtyEight_patch, difficultySelectThirtyNine_patch,
        difficultySelectForty_patch, difficultySelectFortyOne_patch, difficultySelectFortyTwo_patch,
        difficultySelectFortyThree_patch, difficultySelectFortyFour_patch, difficultySelectFortyFive_patch,
        difficultySelectFortySix_patch, difficultySelectFortySeven_patch, difficultySelectFortyEight_patch,
        difficultySelectFortyNine_patch, disableDarkslayerDown_patch, disableDarkslayerLeft_patch,
        disableDarkslayerRight_patch, disableDarkslayerUp_patch, stunAnything_patch, removeLaunchArmour_patch,
        characterChangeOne_patch, characterChangeTwo_patch, characterchangeThree_patch, characterchangeFour_patch,
        characterChangeFive_patch, characterChangeSix_patch, characterChangeSeven_patch, characterChangeEight_patch,
        characterChangeNine_patch, characterChangeTen_patch, characterChangeEleven_patch, characterChangeTwelve_patch,
        characterChangeThirteen_patch, characterChangeFourteen_patch, characterChangeFifteen_patch,
        characterChangeSixteen_patch, characterChangeSeventeen_patch, characterChangeEighteen_patch,
        characterChangeNineteen_patch, characterChangeTwenty_patch, characterChangeTwentyOne_patch,
        sprintFasterActivate_patch, enemyAttackOffscreen_patch, slowWalkOne_patch, slowWalkTwo_patch, hideStyle_patch,
        hideOrbs_patch, hideTimer_patch, autoSkipIntro_patch, ldkWithDMDTwo_patch, infiniteReviveOne_patch,
        infiniteReviveTwo_patch, infiniteReviveThree_patch, replaceScarecrowLeg_patch, replaceScarecrowArm_patch,
        replaceMegaScarecrow_patch, replacementAddressTwo_patch, replaceBiancoAngelo_patch, replaceAltoAngelo_patch,
        replaceMephisto_patch, replaceFaust_patch, replaceFrost_patch, replaceAssault_patch, replaceBlitz_patch,
        replaceChimera_patch, replaceBasilisk_patch, replaceBerial_patch, replaceBael_patch, replaceEchidna_patch,
        replaceCredo_patch, replaceAgnus_patch, autoSkipOutro_patch, infiniteTableHopper_patch,
        infiniteReviveFour_patch, heightRestrictionDoubleDown_patch, heightRestrictionRev_patch,
        infiniteTrickRange_patch, cameraSensitivity_patch;


    const hl::IHook* g_PresentHook;
    const hl::IHook* g_EndSceneHook;
    const hl::IHook* g_ResetHook;
    hl::Hooker m_hooker, m_hook;
    hl::Input input;
    hl::ConsoleEx m_con;

	void loadSettings();
	void saveSettings();
	//@TODO: remove this later, testing refactored mods
	bool m_modsInitialized;
	std::unique_ptr<WorkRate> m_workRate;
	std::unique_ptr<AreaJump> m_areaJump;
private:
	std::string m_confPath;
};
