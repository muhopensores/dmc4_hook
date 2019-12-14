
#include <windows.h>
#include <cstdio>
#include <tchar.h>
#include <iostream>

#include "inih/cpp/INIReader.h"
#include "main.h"
#include "hooks.h"
#include "aobs.h"
#include "naked.h"
#include "patches.h"
#include "imgui_dmc4.h"
#include "gui_functions.h"

#include "hacklib/Logging.h"
#include "hacklib/CrashHandler.h"

uint32_t uninit_value = 0xCCCCCCCC;

uintptr_t readMemoryPointer(uintptr_t address)
{
    auto result = 0;
    hl::CrashHandler([address, &result] { result = IsBadReadPtr((const void*)address, sizeof(uintptr_t)); },
                     [](uint32_t code) { HL_LOG_ERR("Crash prevented: %p\n", code); });
    if (result)
    {
        HL_LOG_ERR("IsBadReadPtr: %p\n", address);
        return 0;
    }
    else
        return *((uintptr_t*)address);
}
template <typename T>
T ReadPointerPath(std::vector<uintptr_t> offsets)
{
    auto len = offsets.size();
    auto ret = readMemoryPointer(offsets[0]);
    if (!ret)
        // return (T)&uninit_value;
        return NULL;
    for (uint8_t i = 1; i < len - 1; i++)
    {
        auto offset = offsets[i];
        ret = readMemoryPointer(ret + offset);
        if (!ret)
            break;
    }
    if (!ret)
        return NULL;
    else
        return (T)(ret + offsets[len - 1]);
}
// initialization bools
bool initialized = true;
bool g_bWasInitialized = true;
bool g_borderless = false;

// bools to check if cheat is active or not
bool checkStyleSwitch = false;
bool checkWeaponSwitch = false;
bool checkJcCooldown = false;
bool checkMovingTargetChange = false;
bool checkHeightRestrictionDante = false;
bool checkHeightRestrictionNero = false;
bool checkInfiniteTime = false;
bool checkinfiniteAllHealth = false;
bool checkDisableCameraEvents = false;
bool checkHideHUD = false;
bool checkEnemyNoDT = false;
bool checkEnemyInstantDT = false;
bool checkBpPortalAutoOpen = false;
bool checkFastPandora = false;
bool checkInfiniteAirHike = false;
bool checkInfiniteDT = false;
bool checkInfinitePlayerHealth = false;
bool checkBerialPractice = false;
bool checkBossRush = false;
bool checkDanteMustDie = false;
bool checkDisableDarkslayerDown = false;
bool checkDisableDarkslayerLeft = false;
bool checkDisableDarkslayerRight = false;
bool checkDisableDarkslayerUp = false;
bool checkMoveIDAlloc = false;
bool checkSelectiveCancels = false;
bool checkEcstasyCancel = false;
bool checkArgumentCancel = false;
bool checkKickThirteenCancel = false;
bool checkSlashDimensionCancel = false;
bool checkPropCancel = false;
bool checkShockCancel = false;
bool checkOmenCancel = false;
bool checkDamageModifier = false;
bool checkOrbDisplay = false;
bool checkStunAnything = false;
bool checkRemoveLaunchArmour = false;
bool checkCharacterChange = false;
bool checkSprintFasterActivate = false;
bool checkKeyboardEnable = false;
bool checkEnemyAttackOffscreen = false;
bool checkSlowWalk = false;
bool checkCameraSettings = false;
bool checkHideStyle = false;
bool checkautoSkiptIntro = false;
bool checkLDKWithDMD = false;
bool checkInfiniteRevive = false;
bool checkSetStyle = false;
bool checkRandomEnemies = false;
bool checkAutoSkipOutro = false;
bool checkInfiniteTableHopper = false;
bool checkLockOn = false;
bool checkLockOff = false;
bool checkTrackingFullHouse = false;
bool checkTimerAlloc = false;
bool checkBackForward = false;
bool checkTrickDown = false;
bool checkFloorTouch = false;
bool checkHoneyComb = false;
bool checkInfiniteTrickRange = false;
bool checkCameraSensitivity = false;
bool checkRoseRemovesPins = false;
bool checkNoHelmBreakerKnockback = false;

hl::StaticInit<class hlMain> g_main;

hlMain* GetMain()
{
    return g_main.getMain();
}

// toggle functions to call from imgui to apply cheats
void hlMain::ImGuiToggleInfPlayerHealth()
{
    g_InfPlayerHealthEnable = !g_InfPlayerHealthEnable;
}

void hlMain::ImGuiToggleInfDT()
{
    g_InfDTEnable = !g_InfDTEnable;
}

void hlMain::ImGuiToggleBerialDaze()
{
    g_berialDazeEnable = !g_berialDazeEnable;
}

void hlMain::ImGuiToggleSelectiveCancels()
{
    g_moveIDAllocEnable = !g_moveIDAllocEnable;
    g_selectiveCancelsEnable = !g_selectiveCancelsEnable;
}

void hlMain::ImGuiToggleEcstasyCancel()
{
    g_ecstasyCancelEnable = !g_ecstasyCancelEnable;
}

void hlMain::ImGuiToggleArgumentCancel()
{
    g_argumentCancelEnable = !g_argumentCancelEnable;
}

void hlMain::ImGuiToggleKickThirteenCancel()
{
    g_kickThirteenCancelEnable = !g_kickThirteenCancelEnable;
}

void hlMain::ImGuiToggleSlashDimensionCancel()
{
    g_slashDimensionCancelEnable = !g_slashDimensionCancelEnable;
}

void hlMain::ImGuiTogglePropCancel()
{
    g_propCancelEnable = !g_propCancelEnable;
}

void hlMain::ImGuiToggleShockCancel()
{
    g_shockCancelEnable = !g_shockCancelEnable;
}

void hlMain::ImGuiToggleOmenCancel()
{
    g_omenCancelEnable = !g_omenCancelEnable;
}

void hlMain::ImGuiToggleHoneyComb()
{
    g_honeyCombEnable = !g_honeyCombEnable;
}

void hlMain::ImGuiToggleDamageModifier()
{
    g_damageModifierEnable = !g_damageModifierEnable;
}

void hlMain::ImGuiToggleLDKWithDMD()
{
    g_ldkWithDMDEnable = !g_ldkWithDMDEnable;
}

void hlMain::ImGuiToggleTrackingFullHouse()
{
    g_trackingFullHouseEnable = !g_trackingFullHouseEnable;
}

void hlMain::ImGuiToggleOrbDisplay()
{
    g_orbDisplayEnable = !g_orbDisplayEnable;
}

void hlMain::ImGuiToggleTrickDown()
{
    g_trickDownEnable = !g_trickDownEnable;
}

void hlMain::ImGuiToggleRoseRemovesPins()
{
    g_roseRemovesPinsEnable = !g_roseRemovesPinsEnable;
}

void hlMain::ImGuiToggleNoHelmBreakerKnockback()
{
    g_noHelmBreakerKnockbackEnable = !g_noHelmBreakerKnockbackEnable;
}

void hlMain::ImGuiInitiateJump()
{
    *initiateJump = 1;
}

void hlMain::ImGuiStageJump(int room)
{
    // this could be changed to local variable
    HL_LOG_RAW("roomID = ReadPointerPath<int*>({ modBase + 0xA552C8, 0xDEADBEEF, 0x33 });\n")
    roomID = ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830, 0x6C });
    if (roomID == NULL)
    {
        HL_LOG_ERR("initiateJump is NULL\n");
        roomID = (int*)&uninit_value;
    }

    // just set roomID and after the switch block initateJumps will be called, you can similarly
    // set the pointer to initiateJumps there to avoid having to define and call a function.
    // remove unused functions later.
    switch (room)
    {
    case 0:
        *roomID = 705; // "Bloody Palace 1-19"
        break;
    case 1:
        *roomID = 704; // "Bloody Palace 21-39"
        break;
    case 2:
        *roomID = 703; // "Bloody Palace 41-59"
        break;
    case 3:
        *roomID = 701; // "Bloody Palace 61-79"
        break;
    case 4:
        *roomID = 702; // "Bloody Palace 81-99"
        break;
    case 5:
        *roomID = 503; // "Bloody Palace 20"
        break;
    case 6:
        *roomID = 504; // "Bloody Palace 40"
        break;
    case 7:
        *roomID = 505; // "Bloody Palace 60"
        break;
    case 8:
        *roomID = 507; // "Bloody Palace 80"
        break;
    case 9:
        *roomID = 506; // "Bloody Palace 100"
        break;
    case 10:
        *roomID = 700; // "Bloody Palace 101"
        break;
    case 11:
        *roomID = 0; // "Opera House"
        break;
    case 12:
        *roomID = 1; // "Opera House Plaza"
        break;
    case 13:
        *roomID = 2; // "Storehouse"
        break;
    case 14:
        *roomID = 3; // "Cathedral"
        break;
    case 15:
        *roomID = 4; // "Terrace / Business District"
        break;
    case 16:
        *roomID = 5; // "Residential District"
        break;
    case 17:
        *roomID = 6; // "Port Caerula"
        break;
    case 18:
        *roomID = 7; // "Customs House"
        break;
    case 19:
        *roomID = 8; // "First Mining Area"
        break;
    case 20:
        *roomID = 9; // "Ferrum Hills"
        break;
    case 21:
        *roomID = 10; // "M17 Opera House"
        break;
    case 22:
        *roomID = 11; // "M17 Opera House Plaza"
        break;
    case 23:
        *roomID = 12; // "Business District / Terrace"
        break;
    case 24:
        *roomID = 13; // "M20 Opera House Plaza"
        break;
    case 25:
        *roomID = 100; // "Second Mining Area"
        break;
    case 26:
        *roomID = 105; // "Fortuna Castle Gate"
        break;
    case 27:
        *roomID = 200; // "Grand Hall (Fortuna Castle)"
        break;
    case 28:
        *roomID = 201; // "Large Hall"
        break;
    case 29:
        *roomID = 202; // "Dining Room"
        break;
    case 30:
        *roomID = 203; // "Torture Chamber"
        break;
    case 31:
        *roomID = 204; // "Central Courtyard"
        break;
    case 32:
        *roomID = 205; // "Foris Falls (Bridge Area)
        break;
    case 33:
        *roomID = 206; // "Gallery"
        break;
    case 34:
        *roomID = 207; // "Library"
        break;
    case 35:
        *roomID = 209; // "Soldier's Graveyard"
        break;
    case 36:
        *roomID = 210; // "Master's Chamber"
        break;
    case 37:
        *roomID = 211; // "Spiral Wall"
        break;
    case 38:
        *roomID = 212; // "Underground Labratory"
        break;
    case 39:
        *roomID = 213; // "R&D Access"
        break;
    case 40:
        *roomID = 214; // "Game Room"
        break;
    case 41:
        *roomID = 215; // "Containment Room"
        break;
    case 42:
        *roomID = 216; // "Angel Creation"
        break;
    case 43:
        *roomID = 217; // "Foris Falls (Detour Area)"
        break;
    case 44:
        *roomID = 300; // "Forest Entrance"
        break;
    case 45:
        *roomID = 301; // "Windswept Valley"
        break;
    case 46:
        *roomID = 302; // "Ruined Church"
        break;
    case 47:
        *roomID = 303; // "Ruined Valley"
        break;
    case 48:
        *roomID = 304; // "Ancient Training Ground"
        break;
    case 49:
        *roomID = 305; // "Lapis River"
        break;
    case 50:
        *roomID = 306; // "Ancient Plaza"
        break;
    case 51:
        *roomID = 307; // "Den of the She-Viper"
        break;
    case 52:
        *roomID = 308; // "Forgotten Ruins"
        break;
    case 53:
        *roomID = 309; // "Hidden Pit"
        break;
    case 54:
        *roomID = 310; // "Ruined Lowlands"
        break;
    case 55:
        *roomID = 311; // "Lost Woods"
        break;
    case 56:
        *roomID = 400; // "Gran Album Bridge"
        break;
    case 57:
        *roomID = 401; // "Grand Hall (Order of the Sword HQ)"
        break;
    case 58:
        *roomID = 402; // "Key Chamber"
        break;
    case 59:
        *roomID = 403; // "The Gauntlet"
        break;
    case 60:
        *roomID = 404; // "Agnus' Room"
        break;
    case 61:
        *roomID = 405; // "Security Corridor"
        break;
    case 62:
        *roomID = 406; // "Experiment Disposal"
        break;
    case 63:
        *roomID = 407; // "Meeting Room"
        break;
    case 64:
        *roomID = 408; // "Ascension Chamber"
        break;
    case 65:
        *roomID = 409; // "Advent Chamber"
        break;
    case 66:
        *roomID = 500; // "Machina Ex Deus"
        break;
    case 67:
        *roomID = 501; // "Stairway to Heaven"
        break;
    case 68:
        *roomID = 502; // "Sacred Heart"
        break;
    case 69:
        *roomID = 510; // "M18"
        break;
    case 70:
        *roomID = 512; // "Sky Above Fortuna"
        break;
    case 71:
        *roomID = 800; // "Secret Mission 1"
        break;
    case 72:
        *roomID = 801; // "Secret Mission 2"
        break;
    case 73:
        *roomID = 802; // "Secret Mission 3"
        break;
    case 74:
        *roomID = 803; // "Secret Mission 4"
        break;
    case 75:
        *roomID = 804; // "Secret Mission 5"
        break;
    case 76:
        *roomID = 805; // "Secret Mission 6"
        break;
    case 77:
        *roomID = 806; // "Secret Mission 7"
        break;
    case 78:
        *roomID = 807; // "Secret Mission 8"
        break;
    case 79:
        *roomID = 808; // "Secret Mission 9"
        break;
    case 80:
        *roomID = 809; // "Secret Mission 10"
        break;
    case 81:
        *roomID = 810; // "Secret Mission 11"
        break;
    case 82:
        *roomID = 811; // "Secret Mission 12"
        break;
    }
    ImGuiInitiateJump();
}

void hlMain::GamePause()
{
    if (g_drawGUI)
    {
        *globalSpeed = 0;
    }
    else
    {
        *globalSpeed = 1;
    }
}

void hlMain::ToggleStuff()
{
	// Player
		// General
	ToggleWeaponSwitch(checkWeaponSwitch);
    ToggleJcCooldown(checkJcCooldown);
    ToggleStyleSwitch(checkStyleSwitch);
    ToggleMovingTargetChange(checkMovingTargetChange);
		// Height Restriction Removal
    ToggleHeightRestrictionDante(checkHeightRestrictionDante);
    ToggleHeightRestrictionNero(checkHeightRestrictionNero);
		// Player Damage Modifier
    if (checkDamageModifier)
    {
        g_damageModifierEnable = true;
    }
		// Misc
	ToggleInfiniteAirHike(checkInfiniteAirHike);
    ToggleInfiniteTableHopper(checkInfiniteTableHopper);
    if (checkTrickDown)
    {
        g_trickDownEnable = true;
    }
    ToggleInfiniteTrickRange(checkInfiniteTrickRange);
    if (checkTrackingFullHouse)
    {
        g_trackingFullHouseEnable = true;
    }
    if (checkHoneyComb)
    {
        g_honeyCombEnable = true;
    }
	ToggleFastPandora(checkFastPandora);
    ToggleSprintFasterActivate(checkSprintFasterActivate);
	if (checkRoseRemovesPins)
    {
        g_roseRemovesPinsEnable = true;
    }
	if (checkNoHelmBreakerKnockback)
	{
		g_noHelmBreakerKnockbackEnable = true;
	}
		// Selective Cancels
			// Common
    if (checkSelectiveCancels)
    {
        g_moveIDAllocEnable = true;
        g_selectiveCancelsEnable = true;
    }
    if (checkEcstasyCancel)
    {
        g_ecstasyCancelEnable = true;
    }
    if (checkArgumentCancel)
    {
        g_argumentCancelEnable = true;
    }
    if (checkKickThirteenCancel)
    {
        g_kickThirteenCancelEnable = true;
    }
    if (checkSlashDimensionCancel)
    {
        g_slashDimensionCancelEnable = true;
    }
			// Swords
    if (checkPropCancel)
    {
        g_propCancelEnable = true;
    }
    if (checkShockCancel)
    {
        g_shockCancelEnable = true;
    }
			// Guns
    if (checkOmenCancel)
    {
        g_omenCancelEnable = true;
    }
	// System
		// General
    ToggleInfiniteTime(checkInfiniteTime);
    ToggleDisableCameraEvents(checkDisableCameraEvents);
    ToggleAutoSkipIntro(checkautoSkiptIntro);
    ToggleAutoSkipOutro(checkAutoSkipOutro);
    ToggleHideHUD(checkHideHUD);
    ToggleHideStyle(checkHideStyle);
    ToggleCharacterChange(checkCharacterChange);
    ToggleSlowWalk(checkSlowWalk);
    ToggleBpPortalAutoOpen(checkBpPortalAutoOpen);
    if (checkOrbDisplay)
    {
        g_orbDisplayEnable = true;
    }
		// Game Mode
    ToggleBossRush(checkBossRush);
    ToggleLDKWithDMD(checkLDKWithDMD);
		if (checkLDKWithDMD)
		{
			g_ldkWithDMDEnable = true;
		}
    ToggleInfiniteAllHealth(checkinfiniteAllHealth);
	ToggleEnemyInstantDT(checkEnemyInstantDT);
    ToggleEnemyNoDT(checkEnemyNoDT);
    ToggleEnemyAttackOffscreen(checkEnemyAttackOffscreen);

		// Misc
    ToggleCameraSensitivity(checkCameraSensitivity);

	// Practice
		// General
    ToggleInfiniteAllHealth(checkinfiniteAllHealth);
    if (checkInfiniteDT)
    {
        g_InfDTEnable = true;
    }
    if (checkInfinitePlayerHealth)
    {
        g_InfPlayerHealthEnable = true;
    }
    ToggleInfiniteRevive(checkInfiniteRevive);

		// Misc
    ToggleBerialPractice(checkBerialPractice);
		if (checkBerialPractice)
		{
			g_berialDazeEnable = true;
		}
    ToggleStunAnything(checkStunAnything);
	ToggleRemoveLaunchArmour(checkRemoveLaunchArmour);

		//Disable Darkslayer Inputs
    ToggleDisableDarkslayerUp(checkDisableDarkslayerUp);
    ToggleDisableDarkslayerDown(checkDisableDarkslayerDown);
    ToggleDisableDarkslayerLeft(checkDisableDarkslayerLeft);
    ToggleDisableDarkslayerRight(checkDisableDarkslayerRight);
}

bool hlMain::init()
{
    hl::LogConfig logCfg;
    logCfg.logToFile = true;
    logCfg.logTime = true;
    logCfg.fileName = "dmc4_hook.dll_log.txt";
    hl::ConfigLog(logCfg);
    HL_LOG_RAW("======================================================\n");
    HL_LOG_RAW("                    LOG START                         \n");
    HL_LOG_RAW("======================================================\n");
    modBase = (uintptr_t)GetModuleHandle(NULL);
    HWND window = FindWindowA(NULL, windowName);
    oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
    hookD3D9(modBase);

    // define ini options
    INIReader reader("dmc4hook.ini");

	//g_borderless = reader.GetBoolean("general", "borderless_window", true);

	// Player
		// Limit Removal
    checkWeaponSwitch = reader.GetBoolean("player", "sword_&_gun_switch_limits_removed", true);
    checkJcCooldown = reader.GetBoolean("player", "jc_limits_removed", true);
    checkStyleSwitch = reader.GetBoolean("player", "style_switch_limits_removed", true);
    checkMovingTargetChange = reader.GetBoolean("player", "target_change_limit_removed", true);
		// Height Restriction Removal
    checkHeightRestrictionDante = reader.GetBoolean("player", "dante_height_restriction_removed", true);
    checkHeightRestrictionNero = reader.GetBoolean("player", "nero_height_restriction_removed", true);
		// Player Damage Modifier
    checkDamageModifier = reader.GetBoolean("player", "damage_modifier", true);
		// Misc
    checkInfiniteAirHike = reader.GetBoolean("player", "infinite_air_hike", true);
    checkInfiniteTableHopper = reader.GetBoolean("player", "infinite_table_hopper", true);
    checkTrickDown = reader.GetBoolean("player", "trick_down", true);
    checkInfiniteTrickRange = reader.GetBoolean("player", "infinite_trick_range", true);
    checkTrackingFullHouse = reader.GetBoolean("player", "tracking_full_house", true);
    checkHoneyComb = reader.GetBoolean("player", "instant_honeycomb", true);
    checkFastPandora = reader.GetBoolean("player", "fast_pandora", true);
    checkSprintFasterActivate = reader.GetBoolean("player", "faster_sprint_activation", true);
    checkRoseRemovesPins = reader.GetBoolean("player", "rose_removes_pins", true);
    checkNoHelmBreakerKnockback = reader.GetBoolean("player", "no_helmbreaker_knockback", true);
		// Selective Cancels
    checkSelectiveCancels = reader.GetBoolean("player", "selective_cancels", true);
    checkEcstasyCancel = reader.GetBoolean("player", "ecstasy_cancel", true);
    checkArgumentCancel = reader.GetBoolean("player", "argument_cancel", true);
    checkKickThirteenCancel = reader.GetBoolean("player", "kick_13_cancel", true);
    checkSlashDimensionCancel = reader.GetBoolean("player", "slash_dimension_cancel", true);
    checkPropCancel = reader.GetBoolean("player", "prop_cancel", true);
    checkShockCancel = reader.GetBoolean("player", "shock_cancel", true);
    checkOmenCancel = reader.GetBoolean("player", "omen_cancel", true);

	// System
		// General
    checkInfiniteTime = reader.GetBoolean("system", "infinite_time", true);
    checkDisableCameraEvents = reader.GetBoolean("system", "disable_camera_events", true);
    checkautoSkiptIntro = reader.GetBoolean("system", "auto_skip_mission_intro", true);
	checkAutoSkipOutro = reader.GetBoolean("system", "auto_skip_mission_outros", true);
    checkHideHUD = reader.GetBoolean("system", "hide_hud", true);
	checkHideStyle = reader.GetBoolean("system", "hide_Style_meter_and_orbs", true);
    checkCharacterChange = reader.GetBoolean("system", "character_change", true);
	checkSlowWalk = reader.GetBoolean("system", "enable_slow_walk", true);
    checkBpPortalAutoOpen = reader.GetBoolean("system", "auto_open_doors_and_BP_portal", true);
	checkOrbDisplay = reader.GetBoolean("system", "enemy_hp_red_orb_display", true);
		// Game Mode
	checkBossRush = reader.GetBoolean("system", "boss_rush_mode", true);
	checkLDKWithDMD = reader.GetBoolean("system", "ldk_on_DMD_diff", true);
	checkEnemyInstantDT = reader.GetBoolean("system", "enemy_instant_DT", true);
    checkEnemyNoDT = reader.GetBoolean("system", "enemy_no_DT", true);
    checkEnemyAttackOffscreen = reader.GetBoolean("system", "enemies_attack_offscreen", true);
		// Misc
    checkCameraSensitivity = reader.GetBoolean("system", "increased_camera_sensitivity", true);

	// Practice
		// General
    checkinfiniteAllHealth = reader.GetBoolean("practice", "infinite_health_all", true);
    checkInfiniteDT = reader.GetBoolean("practice", "infinite_DT", true);
    checkInfinitePlayerHealth = reader.GetBoolean("practice", "infinite_player_health", true);
    checkInfiniteRevive = reader.GetBoolean("practice", "infinite_revive", true);
		// Misc
    checkBerialPractice = reader.GetBoolean("practice", "berial_practice", true);
	checkStunAnything = reader.GetBoolean("practice", "stun_anything", true);
    checkRemoveLaunchArmour = reader.GetBoolean("practice", "remove_launch_armour", true);
		// Disable Darkslayer Inputs
    checkDisableDarkslayerUp = reader.GetBoolean("practice", "disable_darkslayer_Dpad_up", true);
    checkDisableDarkslayerDown = reader.GetBoolean("practice", "disable_darkslayer_Dpad_down", true);
    checkDisableDarkslayerLeft = reader.GetBoolean("practice", "disable_darkslayer_Dpad_left", true);
    checkDisableDarkslayerRight = reader.GetBoolean("practice", "disable_darkslayer_Dpad_right", true);

    // checking for ini
    if (reader.ParseError() < 0)
    {
        m_con.printf("Can't load 'dmc4hook.ini'. Using default settings\n");
    }

    // find our aobs/ addresses
    styleSwitch = hl::FindPattern(styleSwitch_aob);
    swordSwitch = hl::FindPattern(swordSwitch_aob);
    gunSwitch = hl::FindPattern(gunSwitch_aob);
    jcCooldown = hl::FindPattern(jcCooldown_aob);
    movingTargetChange = hl::FindPattern(movingTargetChange_aob);
    damagemodifier = hl::FindPattern(damagemodifier_aob);
    orbDisplay = modBase + 0xFDD35;
    heightRestrictionDante = modBase + 0x3B764E;
    heightRestrictionBuster = modBase + 0x3E614B;
    heightRestrictionSplit = modBase + 0x3E5F8D;
    heightRestrictionCalibur = modBase + 0x3E6248;
    heightRestrictionExCalibur = modBase + 0x3E62B6;
    heightRestrictionSnatch = modBase + 0x3E60E4;
    heightRestrictionRaveNero = modBase + 0x3E603F;
    heightRestrictionDoubleDown = modBase + 0x3E5FE1;
    heightRestrictionRev = modBase + 0x3E4B12;
    infiniteTime = modBase + 0x94D6E;
    infiniteAllHealth = modBase + 0x11BFD9;
    disablecameraEventsOne = hl::FindPattern(disableCameraEvents1_aob);
    disablecameraEventsTwo = hl::FindPattern(disableCameraEvents2_aob);
    hideHUDOne = modBase + 0xFEFE5;
    hideHUDTwo = modBase + 0xFF1F0;
    hideHUDThree = modBase + 0x1017F8;
    enemyNoDT = modBase + 0x3309DD;
    enemyInstantDT = modBase + 0x3309A1;
    enemyInstantDTTwo = modBase + 0x3309DF;
    bpPortalAutoOpen = modBase + 0x4E1E0;
    fastPandoraOne = modBase + 0x3BCE7E;
    fastPandoraTwo = modBase + 0x3BD34F;
    fastPandoraThree = modBase + 0x3EACCA;
    fastPandoraFour = modBase + 0x3EAD76;
    fastPandoraFive = modBase + 0x3EAD98;
    fastPandoraSix = modBase + 0x3EADCB;
    infiniteAirHikeOne = modBase + 0x3ACA3A;
    infiniteAirHikeTwo = modBase + 0x3ACA4A;
    infiniteAirHikeThree = modBase + 0x3ACA51;
    infiniteDT = modBase + 0xFF315;
    infinitePlayerHealth = modBase + 0xFF282;
    berialDazeOne = modBase + 0x23515B;
    berialDazeTwo = modBase + 0x24271A;
    berialCollapse = modBase + 0x234FDA;
    bossRushOne = modBase + 0x101AA4;
    bossRushTwo = modBase + 0x4AB8E2;
    bossRushThree = modBase + 0x4AB8FD;
    bossRushFour = modBase + 0x837BAC;
    bossRushFive = modBase + 0x837CCA;
    bossRushSix = modBase + 0x837DE8;
    bossRushSeven = modBase + 0x837F07;
    difficultySelectOne = modBase + 0x4D6C7;
    difficultySelectTwo = modBase + 0x94AE7;
    difficultySelectThree = modBase + 0x94B02;
    difficultySelectFour = modBase + 0xB5689;
    difficultySelectFive = modBase + 0x11B9B6;
    difficultySelectSix = modBase + 0x11B9BE;
    difficultySelectSeven = modBase + 0x11B9DB;
    difficultySelectEight = modBase + 0x11B9E4;
    difficultySelectNine = modBase + 0x11BAFA;
    difficultySelectTen = modBase + 0x11BB02;
    difficultySelectEleven = modBase + 0x11BB19;
    difficultySelectTwelve = modBase + 0x11BB21;
    difficultySelectThirteen = modBase + 0x234D4C;
    difficultySelectFourteen = modBase + 0x24D80D;
    difficultySelectFifteen = modBase + 0x2892AB;
    difficultySelectSixteen = modBase + 0x2ABB41;
    difficultySelectSeventeen = modBase + 0x2C0435;
    difficultySelectEighteen = modBase + 0x2DA9EB;
    difficultySelectNineteen = modBase + 0x2FAD4E;
    difficultySelectTwenty = modBase + 0x304E9E;
    difficultySelectTwentyOne = modBase + 0x3309A0;
    difficultySelectTwentyTwo = modBase + 0x343B6A;
    difficultySelectTwentyThree = modBase + 0x4AB93C;
    difficultySelectTwentyFour = modBase + 0x837A5F;
    difficultySelectTwentyFive = modBase + 0x9EC8B4;
    difficultySelectTwentySix = modBase + 0x9EC9F4;
    difficultySelectTwentySeven = modBase + 0x9ECB34;
    difficultySelectTwentyEight = modBase + 0x9ECC74;
    difficultySelectTwentyNine = modBase + 0x9ECDB4;
    difficultySelectThirty = modBase + 0x11BA85;
    difficultySelectThirtyOne = modBase + 0x11BA90;
    difficultySelectThirtyTwo = modBase + 0x1D2197;
    difficultySelectThirtyThree = modBase + 0x234D5B;
    difficultySelectThirtyFour = modBase + 0x24D6F6;
    difficultySelectThirtyFive = modBase + 0x24D81C;
    difficultySelectThirtySix = modBase + 0x254383;
    difficultySelectThirtySeven = modBase + 0x2892BA;
    difficultySelectThirtyEight = modBase + 0x2949A4;
    difficultySelectThirtyNine = modBase + 0x2ABB50;
    difficultySelectForty = modBase + 0x2C0445;
    difficultySelectFortyOne = modBase + 0x2DA9CA;
    difficultySelectFortyTwo = modBase + 0x2FAD60;
    difficultySelectFortyThree = modBase + 0x304EB0;
    difficultySelectFortyFour = modBase + 0x3BFD41;
    difficultySelectFortyFive = modBase + 0x3BFD72;
    difficultySelectFortySix = modBase + 0x3C0309;
    difficultySelectFortySeven = modBase + 0x378D60;
    difficultySelectFortyEight = modBase + 0x3A7B09;
    difficultySelectFortyNine = modBase + 0x9EC0E0;
    disableDarkslayerDown = modBase + 0x3B6CE3;
    disableDarkslayerLeft = modBase + 0x3B6D42;
    disableDarkslayerRight = modBase + 0x3B6D99;
    disableDarkslayerUp = modBase + 0x3B6C84;

    // specific pointer checks for logging purposes - logs into the logfile after every boot of the game
    HL_LOG_RAW("globalSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x28 });\n");
    globalSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x28 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x28);
    if (globalSpeed == NULL)
    {
        HL_LOG_ERR("globalSpeed ptr is NULL");
        globalSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("playerSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x2c });\n");
    playerSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x2c }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x2c);
    if (playerSpeed == NULL)
    {
        HL_LOG_ERR("playerSpeed is NULL\n");
        playerSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("enemySpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x30 });\n");
    enemySpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x30 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x30);
    if (enemySpeed == NULL)
    {
        HL_LOG_ERR("enemySpeed is NULL\n");
        enemySpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("roomSpeed = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x34 });\n");
    roomSpeed =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x34 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x34);
    if (roomSpeed == NULL)
    {
        HL_LOG_ERR("roomSpeed is NULL\n");
        roomSpeed = (float*)&uninit_value;
    }
    HL_LOG_RAW("turboValue = ReadPointerPath<float*>({ modBase + 0xA558D0, 0x38 });\n");
    turboValue =
        ReadPointerPath<float*>({ modBase + 0xA558D0, 0x38 }); //(float*)(*(uintptr_t*)(modBase + 0xA558D0) + 0x38);
    if (turboValue == NULL)
    {
        HL_LOG_ERR("turboValue is NULL\n");
        turboValue = (float*)&uninit_value;
    }

    moveIDAlloc = modBase + 0x43EBD6;
    selectiveCancels = modBase + 0x40332A;
    stunAnything = hl::FindPattern(stunAnything_aob);
    removeLaunchArmour = hl::FindPattern(removeLaunchArmour_aob);
    characterChangeOne = modBase + 0x3790CF;
    characterChangeTwo = modBase + 0x87825C;
    characterchangeThree = modBase + 0x4ABC19;
    characterchangeFour = modBase + 0x4ABE59;
    characterChangeFive = modBase + 0x4ABFB9;
    characterChangeSix = modBase + 0x4AC109;
    characterChangeSeven = modBase + 0x4AC259;
    characterChangeEight = modBase + 0x4AC3A9;
    characterChangeNine = modBase + 0x4AC4F9;
    characterChangeTen = modBase + 0x4AC649;
    characterChangeEleven = modBase + 0x4AC769;
    characterChangeTwelve = modBase + 0x4AC889;
    characterChangeThirteen = modBase + 0x4AC999;
    characterChangeFourteen = modBase + 0x4ACAE9;
    characterChangeFifteen = modBase + 0x4ACC39;
    characterChangeSixteen = modBase + 0x4ACD99;
    characterChangeSeventeen = modBase + 0x4ACF19;
    characterChangeEighteen = modBase + 0x4AD069;
    characterChangeNineteen = modBase + 0x4AD1D9;
    characterChangeTwenty = modBase + 0x4AD339;
    characterChangeTwentyOne = modBase + 0x4AD449;
    lockOn = modBase + 0x3a8337;
    lockOff = modBase + 0x3a838c;
    trackingFullHouse = modBase + 0x3d3471;
    timerAlloc = modBase + 0x3AD76A;
    backForward = modBase + 0x405BC1;
    trickDown = modBase + 0x3CB119;
    floorTouch = modBase + 0x3CB33D;
    infiniteTrickRange = modBase + 0x3CB0A8;
    cameraSensitivity = modBase + 0x180A8;
    roseRemovesPins = modBase + 0x4158C3;
    noHelmBreakerKnockback = modBase + 0x11C384;

    // we'll set this in StageJump function
    // roomID = ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830, 0x6C });
    // //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x6C);
    bpFloorStage =
        ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830,
                                0x74 }); //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x74);
    if (bpFloorStage == NULL)
    {
        HL_LOG_ERR("bpFloorStage is NULL\n");
        bpFloorStage = (int*)&uninit_value;
    }
    initiateJump =
        ReadPointerPath<int*>({ modBase + 0xA552C8, 0x3830,
                                0x68 }); //(int*)(*(uintptr_t*)(*(uintptr_t*)(modBase + 0xA552C8) + 0x3830) + 0x68);
    if (initiateJump == NULL)
    {
        HL_LOG_ERR("initiateJump is NULL\n");
        initiateJump = (int*)&uninit_value;
    }

    sprintFasterActivate = modBase + 0x40456C;
    enemyAttackOffscreen = modBase + 0xA8CE9;
    slowWalkOne = modBase + 0x421C83;
    slowWalkTwo = modBase + 0x421D85;
    cameraHeightSetting = modBase + 0x191C5;
    cameraDistanceSetting = modBase + 0x1EF09;
    cameraDistanceLockonSetting = modBase + 0x19D16;
    cameraAngleSetting = modBase + 0x1914C;
    hideStyle = modBase + 0x1017F8;
    hideOrbs = modBase + 0xFF1F2;
    autoSkipIntro = modBase + 0x6DF1;
    autoSkipOutro = modBase + 0x7E6D;
    ldkWithDMDOne = modBase + 0x94AED;
    ldkWithDMDTwo = modBase + 0x3309A1;
    infiniteReviveOne = modBase + 0x39FA7C;
    infiniteReviveTwo = modBase + 0xD270;
    infiniteReviveThree = modBase + 0xD49B;
    infiniteReviveFour = modBase + 0xD25A;
    replaceScarecrowLeg = modBase + 0x13F810;
    replaceScarecrowArm = modBase + 0x15E710;
    replaceMegaScarecrow = modBase + 0x15F7E0;
    replacementAddressTwo = modBase + 0x24B77B;
    replaceBiancoAngelo = modBase + 0x161A10;
    replaceAltoAngelo = modBase + 0x176C80;
    hideTimer = modBase + 0xFDE27;
    replaceMephisto = modBase + 0x17F1E0;
    replaceFaust = modBase + 0x195810;
    replaceFrost = modBase + 0x1A3F60;
    replaceAssault = modBase + 0x1B3170;
    replaceBlitz = modBase + 0x1D1760;
    replaceChimera = modBase + 0x1DC160;
    replaceBasilisk = modBase + 0x21A7B0;
    replaceBerial = modBase + 0x230AC0;
    replaceBael = modBase + 0x249CB0;
    replaceEchidna = modBase + 0x285340;
    replaceCredo = modBase + 0x2AA2C0;
    replaceAgnus = modBase + 0x2BDE60;
    infiniteTableHopper = modBase + 0x3F873C;

    // hooks and jumps to get back to the correct address after hooking
    if (damagemodifier != 0)
    {
        auto damagemodifier_hk = m_hook.hookJMP(damagemodifier, 5, &damagemodifier_proc, &_damagemodifierContinue);
    }

    if (infiniteDT != 0)
    {
        auto infiniteDT_hk = m_hook.hookJMP(infiniteDT, 8, &infiniteDT_proc, &_infiniteDTContinue);
    }

    if (infinitePlayerHealth != 0)
    {
        auto infinitePlayerHealth_hk =
            m_hook.hookJMP(infinitePlayerHealth, 8, &infinitePlayerHealth_proc, &_infinitePlayerHealthContinue);
    }

    if (berialDazeTwo != 0)
    {
        auto berialDazeTwo_hk = m_hook.hookJMP(berialDazeTwo, 8, &berialDaze_proc, &_berialDazeContinue);
    }

    if (moveIDAlloc != 0)
    {
        auto moveIDAlloc_hk = m_hook.hookJMP(moveIDAlloc, 6, &moveIDAlloc_proc);
    }

    if (selectiveCancels != 0)
    {
        auto selectiveCancels_hk = m_hook.hookJMP(selectiveCancels, 6, &selectiveCancels_proc);
    }

    if (cameraHeightSetting != 0)
    {
        auto cameraHeightSetting_hk =
            m_hook.hookJMP(cameraHeightSetting, 8, &cameraHeight_proc, &_cameraHeightContinue);
    }

    if (cameraDistanceSetting != 0)
    {
        auto cameraDistanceSetting_hk =
            m_hook.hookJMP(cameraDistanceSetting, 8, &cameraDistance_proc, &_cameraDistanceContinue);
    }

    if (cameraDistanceLockonSetting != 0)
    {
        auto cameraDistanceLockonSetting_hk =
            m_hook.hookJMP(cameraDistanceLockonSetting, 8, &cameraDistanceLockon_proc, &_cameraDistanceLockonContinue);
    }

    if (cameraAngleSetting != 0)
    {
        auto cameraAngleSeting_hk = m_hook.hookJMP(cameraAngleSetting, 8, &cameraAngle_proc, &_cameraAngleContinue);
    }

    if (ldkWithDMDOne != 0)
    {
        auto ldkWithDMDOne_hk = m_hook.hookJMP(ldkWithDMDOne, 6, &ldkWithDMD_proc); //, &_ldkWithDMDContinue);
    }

    if (lockOn != 0)
    {
        auto lockOn_hk = m_hook.hookJMP(lockOn, 10, &lockOn_proc, &_lockOnContinue);
    }

    if (lockOff != 0)
    {
        auto lockOff_hk = m_hook.hookJMP(lockOff, 10, &lockOff_proc, &_lockOffContinue);
    }

    if (trackingFullHouse != 0)
    {
        auto trackingFullHouse_hk = m_hook.hookJMP(trackingFullHouse, 7, &trackingFullHouse_proc);
    }

    if (orbDisplay != 0)
    {
        auto orbDisplay_hk = m_hook.hookJMP(orbDisplay, 6, &orbDisplay_proc); //, &_orbDisplayContinue
    }

    if (timerAlloc != 0)
    {
        auto timerAlloc_hk = m_hook.hookJMP(timerAlloc, 10, &timerAlloc_proc);
    }

    if (backForward != 0)
    {
        auto backForward_hk = m_hook.hookJMP(backForward, 8, &backForward_proc);
    }

    if (trickDown != 0)
    {
        auto trickDown_hk = m_hook.hookJMP(trickDown, 8, &trickDown_proc);
    }

    if (floorTouch != 0)
    {
        auto floorTouch_hk = m_hook.hookJMP(floorTouch, 8, &floorTouch_proc);
    }

    if (roseRemovesPins != 0)
    {
        auto roseRemovesPins_hk = m_hook.hookJMP(roseRemovesPins, 10, &roseRemovesPins_proc);
    }

    if (noHelmBreakerKnockback != 0)
    {
        auto noHelmBreakerKnockback_hk = m_hook.hookJMP(noHelmBreakerKnockback, 5, &noHelmBreakerKnockback_proc);
    }

    ToggleStuff();
    return true;
}

bool hlMain::step()
{
    input.update();
    if (input.wentDown(VK_DELETE))
    {
        g_drawGUI = !g_drawGUI;
        GamePause();
    }
    /*
	if (input.wentDown(VK_OEM_3)) 
	{
		g_borderless = !g_borderless;
		ToggleBorderless(g_borderless);
	}*/
    return true;
}

// function to render the gui onto screen
void RenderImgui(IDirect3DDevice9* m_pDevice)
{
    auto main = GetMain(); // get ptr to hacklib main
    if (g_bWasInitialized)
    {
        g_bWasInitialized = false;
        main->ToggleStuff();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui_ImplWin32_Init(FindWindowA(NULL, windowName));
        ImGui_ImplDX9_Init(m_pDevice);
    }

    DarkTheme();
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    DrawWindow();

    // specific imgui functions, can be looked up in examples or the documentation
    // references/ points to other functions to apply logic behind the gui toggles/ objects
    {
        BeginDrawing();
        ImGui::SameLine(0, 0);
        FPSDrawing();
        ImGui::Spacing();
        if (ImGui::Checkbox("Borderless Window", &g_borderless))
        {
            ToggleBorderless(g_borderless);
        }
        ImGui::Spacing();

        if (ImGui::BeginTabBar("Trainer", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ImGui::BeginTabItem("Player"))
            {
                ImGui::Spacing();
                ImGui::Text("Limit Removal");

                if (ImGui::Checkbox("Remove Sword & Gun Switch Limit", &checkWeaponSwitch))
                {
                    main->ToggleWeaponSwitch(checkWeaponSwitch);
                }

                if (ImGui::Checkbox("Remove JC Limit", &checkJcCooldown))
                {
                    main->ToggleJcCooldown(checkJcCooldown);
                }

                if (ImGui::Checkbox("Remove Style Switch Limit", &checkStyleSwitch))
                {
                    main->ToggleStyleSwitch(checkStyleSwitch);
                }

                if (ImGui::Checkbox("Remove Target Switch Limit", &checkMovingTargetChange))
                {
                    main->ToggleMovingTargetChange(checkMovingTargetChange);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                (ImGui::Text("Height Restriction Removal"));

                if (ImGui::Checkbox("Dante", &checkHeightRestrictionDante))
                {
                    main->ToggleHeightRestrictionDante(checkHeightRestrictionDante);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Nero", &checkHeightRestrictionNero))
                {
                    main->ToggleHeightRestrictionNero(checkHeightRestrictionNero);
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Checkbox("Player Damage Modifier", &checkDamageModifier))
                {
                    main->ImGuiToggleDamageModifier();
                }
                ImGui::InputFloat("Multiplier", &damagemultiplier, 0.1f, 1.0f, "%.1f");

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::Checkbox("Infinite Air Hike", &checkInfiniteAirHike))
                {
                    main->ToggleInfiniteAirHike(checkInfiniteAirHike);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Infinite Table Hopper", &checkInfiniteTableHopper))
                {
                    main->ToggleInfiniteTableHopper(checkInfiniteTableHopper);
                }

                if (ImGui::Checkbox("Trick Down", &checkTrickDown))
                {
                    main->ImGuiToggleTrickDown();
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Infinite Trick Range", &checkInfiniteTrickRange))
                {
                    main->ToggleInfiniteTrickRange(checkInfiniteTrickRange);
                }

                if (ImGui::Checkbox("Tracking Full House", &checkTrackingFullHouse))
                {
                    main->ImGuiToggleTrackingFullHouse();
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Instant Honeycomb", &checkHoneyComb))
                {
                    main->ImGuiToggleHoneyComb();
                }

                if (ImGui::Checkbox("Fast Pandora", &checkFastPandora))
                {
                    main->ToggleFastPandora(checkFastPandora);
                }

                ImGui::SameLine(198);

                if (ImGui::Checkbox("Fast Speed", &checkSprintFasterActivate))
                {
                    main->ToggleSprintFasterActivate(checkSprintFasterActivate);
                }

                if (ImGui::Checkbox("Rose Removes Pins", &checkRoseRemovesPins))
                {
                    main->ImGuiToggleRoseRemovesPins();
                }

				ImGui::SameLine(198);

                if (ImGui::Checkbox("No Helm Breaker Knockback", &checkNoHelmBreakerKnockback))
                {
                    main->ImGuiToggleNoHelmBreakerKnockback();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Selective Cancels"))
                {
                    if (ImGui::Checkbox("Enable", &checkSelectiveCancels))
                    {
                        main->ImGuiToggleSelectiveCancels();
                    }

                    ImGui::Separator();
                    ImGui::Text("Common");

                    if (ImGui::Checkbox("Ecstasy", &checkEcstasyCancel))
                    {
                        main->ImGuiToggleEcstasyCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Argument", &checkArgumentCancel))
                    {
                        main->ImGuiToggleArgumentCancel();
                    }

                    if (ImGui::Checkbox("Kick 13", &checkKickThirteenCancel))
                    {
                        main->ImGuiToggleKickThirteenCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Slash Dimension", &checkSlashDimensionCancel))
                    {
                        main->ImGuiToggleSlashDimensionCancel();
                    }

                    ImGui::Separator();
                    ImGui::Text("Swords");

                    if (ImGui::Checkbox("Prop", &checkPropCancel))
                    {
                        main->ImGuiTogglePropCancel();
                    }

                    ImGui::SameLine(198);

                    if (ImGui::Checkbox("Shock", &checkShockCancel))
                    {
                        main->ImGuiToggleShockCancel();
                    }

                    ImGui::Separator();
                    ImGui::Text("Guns");

                    if (ImGui::Checkbox("Omen", &checkOmenCancel))
                    {
                        main->ImGuiToggleOmenCancel();
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("System"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                if (ImGui::Checkbox("Disable Timer", &checkInfiniteTime))
                {
                    main->ToggleInfiniteTime(checkInfiniteTime);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Disable Camera Events", &checkDisableCameraEvents))
                {
                    main->ToggleDisableCameraEvents(checkDisableCameraEvents);
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Certain missions or parts will cause a black screen when this option is enabled. Press the "
                           "start button and then X/A to go into a menu. The screen should return to normal.");

                if (ImGui::Checkbox("Skip Mission Intros", &checkautoSkiptIntro))
                {
                    main->ToggleAutoSkipIntro(checkautoSkiptIntro);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Skip Mission Outros", &checkAutoSkipOutro))
                {
                    main->ToggleAutoSkipOutro(checkAutoSkipOutro);
                }

                if (ImGui::Checkbox("Hide HUD", &checkHideHUD))
                {
                    checkHideStyle = false;
                    main->ToggleHideHUD(checkHideHUD);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Hide Timer,Style & Orbs", &checkHideStyle))
                {
                    checkHideHUD = false;
                    main->ToggleHideStyle(checkHideStyle);
                }

                if (ImGui::Checkbox("Character Change", &checkCharacterChange))
                {
                    main->ToggleCharacterChange(checkCharacterChange);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Slow Walk", &checkSlowWalk))
                {
                    main->ToggleSlowWalk(checkSlowWalk);
                }

                ImGui::SameLine(0, 1);

                HelpMarker("Press & hold the jump button to walk slowly");

                if (ImGui::Checkbox("Auto Open Sealed Doors & Portals", &checkBpPortalAutoOpen))
                {
                    main->ToggleBpPortalAutoOpen(checkBpPortalAutoOpen);
                }

                if (ImGui::Checkbox("Enemy HP Red Orb Display", &checkOrbDisplay))
                {
                    main->ImGuiToggleOrbDisplay();
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Text("Game Mode");

                static int diff_index = 0;
                if (ImGui::Combo("Difficulty", &diff_index, "Default\0Dante Must Die\0God Must Die\0"))
                {
                    switch (diff_index)
                    {
                    case 0:
                        main->SetDefault();
                        break;
                    case 1:
                        main->SetDMD();
                        break;
                    case 2:
                        main->SetGMD();
                        break;
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();

                if (ImGui::Checkbox("Boss Rush Mode", &checkBossRush))
                {
                    main->ToggleBossRush(checkBossRush);
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Toggle and start BP run");
                ImGui::SameLine(205);

                if (ImGui::Checkbox("LDK on DMD Difficulty", &checkLDKWithDMD))
                {
                    main->ToggleLDKWithDMD(checkLDKWithDMD);
                    main->ImGuiToggleLDKWithDMD();
                }

                ImGui::SameLine(0, 1);
                HelpMarker("Start a Mission on Legendary Dark Knight. Enemies are on DMD Difficulty, instead of SOS");

                if (ImGui::Checkbox("Enemies DT Instantly", &checkEnemyInstantDT))
                {
                    main->ToggleEnemyInstantDT(checkEnemyInstantDT);
                }

                ImGui::SameLine(205);

                if (ImGui::Checkbox("Enemies Don't DT", &checkEnemyNoDT))
                {
                    main->ToggleEnemyNoDT(checkEnemyNoDT);
                }

                if (ImGui::Checkbox("Enemies Attack Off-Screen", &checkEnemyAttackOffscreen))
                {
                    main->ToggleEnemyAttackOffscreen(checkEnemyAttackOffscreen);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::CollapsingHeader("Speed"))
                {
                    ImGui::InputFloat("Turbo Value", main->turboValue, 0.1f, 0.5f, "%.1f%");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Global Speed", main->globalSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Room Speed", main->roomSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Player Speed", main->playerSpeed, 0.0f, 3.0f, "%.1f");
                    ImGui::Spacing();
                    ImGui::SliderFloat("Enemy Speed", main->enemySpeed, 0.0f, 3.0f, "%.1f");
                }

                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Camera"))
                {
                    ImGui::InputFloat("Camera Height", &cameraHeight, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between -10000 / 10000. Default value is 170");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Distance", &cameraDistance, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between 0 / 50000. Default value is 550");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Distance \n(Lockon)", &cameraDistanceLockon, 1.0f, 10.0f, "%.0f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between 0 / 50000. Default value is 520");
                    ImGui::Spacing();
                    ImGui::InputFloat("Camera Angle", &cameraAngle, 0.1f, 0.5f, "%.1f%");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Value between -1.5 / 1.5. Default value is 0.2");
                    ImGui::Spacing();

                    if (ImGui::Checkbox("Increased Camera Sensitivity", &checkCameraSensitivity))
                    {
                        main->ToggleCameraSensitivity(checkCameraSensitivity);
                    }
                }

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Environment"))
            {
                ImGui::Spacing();
                ImGui::Text("Area Jump");
                ImGui::Spacing();
                ImGui::InputInt("BP Floor\n(no boss stages)", main->bpFloorStage, 1, 10,
                                ImGuiInputTextFlags_AllowTabInput);
                ImGui::SameLine(0, 1);
                HelpMarker("Type in the BP floor you want to teleport to and choose the correct BP area below (example "
                           "21-39). Only "
                           "1-99, no boss stages. For boss stages simply select the boss room from the listbox below.");
                ImGui::Spacing();

                const char* room_items[] = {
                    "Bloody Palace 1-19",                 // 705
                    "Bloody Palace 21-39",                // 704
                    "Bloody Palace 41-59",                // 703
                    "Bloody Palace 61-79",                // 701
                    "Bloody Palace 81-99",                // 702
                    "Bloody Palace 20",                   // 503
                    "Bloody Palace 40",                   // 504
                    "Bloody Palace 60",                   // 505
                    "Bloody Palace 80",                   // 502
                    "Bloody Palace 100",                  // 501
                    "Bloody Palace 101",                  // 700
                    "Opera House",                        // 0
                    "Opera House Plaza",                  // 1
                    "Storehouse",                         // 2
                    "Cathedral",                          // 3
                    "Terrace / Business District",        // 4
                    "Residential District",               // 5
                    "Port Caerula",                       // 6
                    "Customs House",                      // 7
                    "First Mining Area",                  // 8
                    "Ferrum Hills",                       // 9
                    "M17 Opera House",                    // 10
                    "M17 Opera House Plaza",              // 11
                    "Business District / Terrace",        // 12
                    "M20 Opera House Plaza",              // 13
                    "Second Mining Area",                 // 100
                    "Fortuna Castle Gate",                // 105
                    "Grand Hall (Fortuna Castle)",        // 200
                    "Large Hall",                         // 201
                    "Dining Room",                        // 202
                    "Torture Chamber",                    // 203
                    "Central Courtyard",                  // 204
                    "Foris Falls (Bridge Area)",          // 205
                    "Gallery",                            // 206
                    "Library",                            // 207
                    "Soldier's Graveyard",                // 209
                    "Master's Chamber",                   // 210
                    "Spiral Well",                        // 211
                    "Underground Laboratory",             // 212
                    "R&D Access",                         // 213
                    "Game Room",                          // 214
                    "Containment Room",                   // 215
                    "Angel Creation",                     // 216
                    "Foris Falls (Detour Area)",          // 217
                    "Forest Entrance",                    // 300
                    "Windswept Valley",                   // 301
                    "Ruined Church",                      // 302
                    "Ruined Valley",                      // 303
                    "Ancient Training Ground",            // 304
                    "Lapis River",                        // 305
                    "Ancient Plaza",                      // 306
                    "Den of the She-Viper",               // 307
                    "Forgotten Ruins",                    // 308
                    "Hidden Pit",                         // 309
                    "Ruined Lowlands",                    // 310
                    "Lost Woods",                         // 311
                    "Gran Album Bridge",                  // 400
                    "Grand Hall (Order of the Sword HQ)", // 401
                    "Key Chamber",                        // 402
                    "The Gauntlet",                       // 403
                    "Agnus' Room",                        // 404
                    "Security Corridor",                  // 405
                    "Experiment Disposal",                // 406
                    "Meeting Room",                       // 407
                    "Ascension Chamber",                  // 408
                    "Advent Chamber",                     // 409
                    "Machina Ex Deus",                    // 500
                    "Stairway to Heaven",                 // 501
                    "Sacred Heart",                       // 502
                    "M18",                                // 510
                    "Sky Above Fortuna",                  // 512
                    "Secret Mission 1",                   // 800
                    "Secret Mission 2",                   // 801
                    "Secret Mission 3",                   // 802
                    "Secret Mission 4",                   // 803
                    "Secret Mission 5",                   // 804
                    "Secret Mission 6",                   // 805
                    "Secret Mission 7",                   // 806
                    "Secret Mission 8",                   // 807
                    "Secret Mission 9",                   // 808
                    "Secret Mission 10",                  // 809
                    "Secret Mission 11",                  // 810
                    "Secret Mission 12"                   // 811
                };

                int room_item_current = 0;
                if (ImGui::ListBox("Room Codes\n(including BP)", &room_item_current, room_items,
                                   IM_ARRAYSIZE(room_items), 10))
                {
                    main->ImGuiStageJump(room_item_current);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Enemy Replacement"))
                {
                    ImGui::Spacing();
                    ImGui::Text("No Cyclical Replacements!");
                    ImGui::SameLine(0, 1);
                    HelpMarker("Example: Don't replace Mephisto with Faust and Faust with Mephisto at the same time. "
                               "The game will get stuck in a loop and cause undesired behaviour.");
                    ImGui::Spacing();
                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Scarecrow(Leg) with: "))
                    {
                        const char* scarecrowLeg_items[] = {
                            "Default", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",         "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",       "Dante",
                        };
                        static int scarecrowLeg_current = 0;
                        if (ImGui::Combo("Replace SC(L)", &scarecrowLeg_current, scarecrowLeg_items,
                                         IM_ARRAYSIZE(scarecrowLeg_items)))
                        {
                            switch (scarecrowLeg_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceScarecrowLeg);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceScarecrowLeg);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceScarecrowLeg);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceScarecrowLeg);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceScarecrowLeg);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceScarecrowLeg);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceScarecrowLeg);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceScarecrowLeg);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceScarecrowLeg);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceScarecrowLeg);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceScarecrowLeg);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceScarecrowLeg);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceScarecrowLeg);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceScarecrowLeg);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceScarecrowLeg);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceScarecrowLeg);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceScarecrowLeg);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Scarecrow(Arm) with: "))
                    {
                        const char* scarecrowArm_items[] = {
                            "Default", "Scarecrow(Leg)", "Mega Scarecrow", "Bianco Angelo", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",         "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",       "Dante"
                        };
                        static int scarecrowArm_current = 0;
                        if (ImGui::Combo("Replace SC(A)", &scarecrowArm_current, scarecrowArm_items,
                                         IM_ARRAYSIZE(scarecrowArm_items)))
                        {
                            switch (scarecrowArm_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceScarecrowArm);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceScarecrowArm);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceScarecrowArm);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceScarecrowArm);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceScarecrowArm);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceScarecrowArm);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceScarecrowArm);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceScarecrowArm);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceScarecrowArm);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceScarecrowArm);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceScarecrowArm);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceScarecrowArm);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceScarecrowArm);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceScarecrowArm);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceScarecrowArm);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceScarecrowArm);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceScarecrowArm);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Mega Scarecrow with: "))
                    {
                        const char* megaScarecrow_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Bianco Angelo", "Alto Angelo",
                            "Mephisto",     "Faust",          "Frost",          "Assault",       "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",          "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",         "Dante"
                        };
                        static int megaScarecrow_current = 0;
                        if (ImGui::Combo("Replace MSC", &megaScarecrow_current, megaScarecrow_items,
                                         IM_ARRAYSIZE(megaScarecrow_items)))
                        {
                            switch (megaScarecrow_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceMegaScarecrow);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceMegaScarecrow);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceMegaScarecrow);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceMegaScarecrow);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceMegaScarecrow);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceMegaScarecrow);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceMegaScarecrow);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceMegaScarecrow);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceMegaScarecrow);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceMegaScarecrow);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceMegaScarecrow);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceMegaScarecrow);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceMegaScarecrow);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceMegaScarecrow);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceMegaScarecrow);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceMegaScarecrow);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceMegaScarecrow);
                                break;
                            case 17:
                                main->ReplaceEnemySeventeeen(main->replaceMegaScarecrow);
                                break;
                            case 18:
                                main->ReplaceEnemyEighteen(main->replaceMegaScarecrow);
                                break;
                            case 19:
                                main->ReplaceEnemyNineteen(main->replaceMegaScarecrow);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Bianco Angelo with: "))
                    {
                        const char* bianco_items[] = {
                            "Default", "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Alto Angelo",  "Mephisto",
                            "Faust",   "Frost",          "Assault",        "Blitz",          "Chimera Seed", "Basilisk",
                            "Berial",  "Credo",          "Agnus",          "Sanctus",        "Dante"
                        };
                        static int bianco_current = 0;
                        if (ImGui::Combo("Replace Bianco", &bianco_current, bianco_items, IM_ARRAYSIZE(bianco_items)))
                        {
                            switch (bianco_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceBiancoAngelo);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceBiancoAngelo);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceBiancoAngelo);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceBiancoAngelo);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceBiancoAngelo);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceBiancoAngelo);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceBiancoAngelo);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceBiancoAngelo);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceBiancoAngelo);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceBiancoAngelo);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceBiancoAngelo);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceBiancoAngelo);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceBiancoAngelo);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceBiancoAngelo);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceBiancoAngelo);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceBiancoAngelo);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceBiancoAngelo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Alto Angelo with: "))
                    {
                        const char* alto_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Mephisto",     "Faust",          "Frost",          "Assault",        "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int alto_current = 0;
                        if (ImGui::Combo("Replace Alto", &alto_current, alto_items, IM_ARRAYSIZE(alto_items)))
                        {
                            switch (alto_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceAltoAngelo);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceAltoAngelo);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceAltoAngelo);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceAltoAngelo);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceAltoAngelo);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceAltoAngelo);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceAltoAngelo);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceAltoAngelo);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceAltoAngelo);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceAltoAngelo);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceAltoAngelo);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceAltoAngelo);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceAltoAngelo);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceAltoAngelo);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceAltoAngelo);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceAltoAngelo);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceAltoAngelo);
                                break;
                            case 17:
                                main->ReplaceEnemySeventeeen(main->replaceAltoAngelo);
                                break;
                            case 18:
                                main->ReplaceEnemyEighteen(main->replaceAltoAngelo);
                                break;
                            case 19:
                                main->ReplaceEnemyNineteen(main->replaceAltoAngelo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Mephisto with: "))
                    {
                        const char* mephisto_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                         "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                         "Faust",          "Frost",          "Assault",
                                                         "Blitz",          "Chimera Seed",   "Basilisk",
                                                         "Berial",         "Credo",          "Agnus",
                                                         "Sanctus",        "Dante" };
                        static int mephisto_current = 0;
                        if (ImGui::Combo("Replace Mephisto", &mephisto_current, mephisto_items,
                                         IM_ARRAYSIZE(mephisto_items)))
                        {
                            switch (mephisto_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceMephisto);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceMephisto);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceMephisto);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceMephisto);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceMephisto);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceMephisto);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceMephisto);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceMephisto);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceMephisto);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceMephisto);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceMephisto);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceMephisto);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceMephisto);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceMephisto);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceMephisto);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceMephisto);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceMephisto);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Faust with: "))
                    {
                        const char* faust_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Alto Angelo",  "Mephisto",       "Frost",          "Assault",        "Blitz",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int faust_current = 0;
                        if (ImGui::Combo("Replace Faust", &faust_current, faust_items, IM_ARRAYSIZE(faust_items)))
                        {
                            switch (faust_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceFaust);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceFaust);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceFaust);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceFaust);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceFaust);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceFaust);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceFaust);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceFaust);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceFaust);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceFaust);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceFaust);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceFaust);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceFaust);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceFaust);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceFaust);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceFaust);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceFaust);
                                break;
                            case 17:
                                main->ReplaceEnemySeventeeen(main->replaceFaust);
                                break;
                            case 18:
                                main->ReplaceEnemyEighteen(main->replaceFaust);
                                break;
                            case 19:
                                main->ReplaceEnemyNineteen(main->replaceFaust);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Frost with: "))
                    {
                        const char* frost_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                      "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                      "Mephisto",       "Faust",          "Assault",
                                                      "Blitz",          "Chimera Seed",   "Basilisk",
                                                      "Berial",         "Credo",          "Agnus",
                                                      "Sanctus",        "Dante" };
                        static int frost_current = 0;
                        if (ImGui::Combo("Replace Faust", &frost_current, frost_items, IM_ARRAYSIZE(frost_items)))
                        {
                            switch (frost_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceFrost);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceFrost);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceFrost);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceFrost);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceFrost);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceFrost);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceFrost);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceFrost);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceFrost);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceFrost);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceFrost);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceFrost);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceFrost);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceFrost);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceFrost);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceFrost);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceFrost);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Assault with: "))
                    {
                        const char* assault_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                        "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                        "Mephisto",       "Faust",          "Frost",
                                                        "Blitz",          "Chimera Seed",   "Basilisk",
                                                        "Berial",         "Credo",          "Agnus",
                                                        "Sanctus",        "Dante" };
                        static int assault_current = 0;
                        if (ImGui::Combo("Replace Assault", &assault_current, assault_items,
                                         IM_ARRAYSIZE(assault_items)))
                        {
                            switch (assault_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceAssault);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceAssault);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceAssault);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceAssault);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceAssault);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceAssault);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceAssault);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceAssault);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceAssault);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceAssault);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceAssault);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceAssault);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceAssault);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceAssault);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceAssault);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceAssault);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceAssault);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Blitz with: "))
                    {
                        const char* blitz_items[] = {
                            "Default",      "Scarecrow(Leg)", "Scarecrow(Arm)", "Mega Scarecrow", "Bianco Angelo",
                            "Alto Angelo",  "Mephisto",       "Faust",          "Frost",          "Assault",
                            "Chimera Seed", "Basilisk",       "Berial",         "Bael",           "Echidna",
                            "Credo",        "Agnus",          "Sanctus",        "Kyrie",          "Dante"
                        };
                        static int blitz_current = 0;
                        if (ImGui::Combo("Replace Blitz", &blitz_current, blitz_items, IM_ARRAYSIZE(blitz_items)))
                        {
                            switch (blitz_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceBlitz);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceBlitz);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceBlitz);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceBlitz);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceBlitz);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceBlitz);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceBlitz);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceBlitz);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceBlitz);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceBlitz);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceBlitz);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceBlitz);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceBlitz);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceBlitz);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceBlitz);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceBlitz);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceBlitz);
                                break;
                            case 17:
                                main->ReplaceEnemySeventeeen(main->replaceBlitz);
                                break;
                            case 18:
                                main->ReplaceEnemyEighteen(main->replaceBlitz);
                                break;
                            case 19:
                                main->ReplaceEnemyNineteen(main->replaceBlitz);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Chimera Seed with: "))
                    {
                        const char* chimera_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                        "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                        "Mephisto",       "Faust",          "Frost",
                                                        "Assault",        "Blitz",          "Basilisk",
                                                        "Berial",         "Credo",          "Agnus",
                                                        "Sanctus",        "Dante" };
                        static int chimera_current = 0;
                        if (ImGui::Combo("Replace Seed", &chimera_current, chimera_items, IM_ARRAYSIZE(chimera_items)))
                        {
                            switch (chimera_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceChimera);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceChimera);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceChimera);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceChimera);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceChimera);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceChimera);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceChimera);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceChimera);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceChimera);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceChimera);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceChimera);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceChimera);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceChimera);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceChimera);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceChimera);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceChimera);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceChimera);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Basilisk with: "))
                    {
                        const char* basilisk_items[] = { "Default",        "Scarecrow(Leg)", "Scarecrow(Arm)",
                                                         "Mega Scarecrow", "Bianco Angelo",  "Alto Angelo",
                                                         "Mephisto",       "Faust",          "Frost",
                                                         "Assault",        "Blitz",          "Chimera Seed",
                                                         "Berial",         "Credo",          "Agnus",
                                                         "Sanctus",        "Dante" };
                        static int basilisk_current = 0;
                        if (ImGui::Combo("Replace Basilisk", &basilisk_current, basilisk_items,
                                         IM_ARRAYSIZE(basilisk_items)))
                        {
                            switch (basilisk_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceBasilisk);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceBasilisk);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceBasilisk);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceBasilisk);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceBasilisk);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceBasilisk);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceBasilisk);
                                break;
                            case 7:
                                main->ReplaceEnemySeven(main->replaceBasilisk);
                                break;
                            case 8:
                                main->ReplaceEnemyEight(main->replaceBasilisk);
                                break;
                            case 9:
                                main->ReplaceEnemyNine(main->replaceBasilisk);
                                break;
                            case 10:
                                main->ReplaceEnemyTen(main->replaceBasilisk);
                                break;
                            case 11:
                                main->ReplaceEnemyEleven(main->replaceBasilisk);
                                break;
                            case 12:
                                main->ReplaceEnemyTwelve(main->replaceBasilisk);
                                break;
                            case 13:
                                main->ReplaceEnemyThirteen(main->replaceBasilisk);
                                break;
                            case 14:
                                main->ReplaceEnemyFourteen(main->replaceBasilisk);
                                break;
                            case 15:
                                main->ReplaceEnemyFifteen(main->replaceBasilisk);
                                break;
                            case 16:
                                main->ReplaceEnemySixteen(main->replaceBasilisk);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Berial with: "))
                    {
                        const char* berial_items[] = { "Default", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int berial_current = 0;
                        if (ImGui::Combo("Replace Berial", &berial_current, berial_items, IM_ARRAYSIZE(berial_items)))
                        {
                            switch (berial_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceBerial);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceBerial);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceBerial);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceBerial);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceBerial);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Bael with: "))
                    {
                        const char* bael_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int bael_current = 0;
                        if (ImGui::Combo("Replace Bael", &bael_current, bael_items, IM_ARRAYSIZE(bael_items)))
                        {
                            switch (bael_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceBael);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceBael);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceBael);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceBael);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceBael);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceBael);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Echidna with: "))
                    {
                        const char* echidna_items[] = { "Default", "Berial", "Credo", "Agnus", "Sanctus", "Dante" };
                        static int echidna_current = 0;
                        if (ImGui::Combo("Replace Echidna", &echidna_current, echidna_items,
                                         IM_ARRAYSIZE(echidna_items)))
                        {
                            switch (echidna_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceEchidna);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceEchidna);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceEchidna);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceEchidna);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceEchidna);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceEchidna);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Credo with: "))
                    {
                        const char* credo_items[] = { "Default", "Berial",  "Bael", "Echidna",
                                                      "Agnus",   "Sanctus", "Dante" };
                        static int credo_current = 0;
                        if (ImGui::Combo("Replace Credo", &credo_current, credo_items, IM_ARRAYSIZE(credo_items)))
                        {
                            switch (credo_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceCredo);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceCredo);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceCredo);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceCredo);
                                break;
                            case 4:
                                main->ReplaceEnemyFour(main->replaceCredo);
                                break;
                            case 5:
                                main->ReplaceEnemyFive(main->replaceCredo);
                                break;
                            case 6:
                                main->ReplaceEnemySix(main->replaceCredo);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }

                    ImGui::Spacing();

                    if (ImGui::TreeNode("Replace Agnus with: "))
                    {
                        const char* agnus_items[] = { "Default", "Berial", "Credo", "Sanctus" };
                        static int agnus_current = 0;
                        if (ImGui::Combo("Replace Agnus", &agnus_current, agnus_items, IM_ARRAYSIZE(agnus_items)))
                        {
                            switch (agnus_current)
                            {
                            case 0:
                                main->ReplaceEnemyDefault(main->replaceAgnus);
                                break;
                            case 1:
                                main->ReplaceEnemyOne(main->replaceAgnus);
                                break;
                            case 2:
                                main->ReplaceEnemyTwo(main->replaceAgnus);
                                break;
                            case 3:
                                main->ReplaceEnemyThree(main->replaceAgnus);
                                break;
                            }
                        }
                        ImGui::TreePop();
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Practice"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                if (ImGui::Checkbox("Infinite Health (All)", &checkinfiniteAllHealth))
                {
                    main->ToggleInfiniteAllHealth(checkinfiniteAllHealth);
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Infinite DT", &checkInfiniteDT))
                {
                    main->ImGuiToggleInfDT();
                }

                if (ImGui::Checkbox("Infinite Player Health", &checkInfinitePlayerHealth))
                {
                    main->ImGuiToggleInfPlayerHealth();
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Infinite Revive", &checkInfiniteRevive))
                {
                    main->ToggleInfiniteRevive(checkInfiniteRevive);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                Misc();

                if (ImGui::Checkbox("Berial Daze", &checkBerialPractice))
                {
                    main->ToggleBerialPractice(checkBerialPractice);
                    main->ImGuiToggleBerialDaze();
                }

                ImGui::SameLine(202);

                if (ImGui::Checkbox("Stun Anything", &checkStunAnything))
                {
                    main->ToggleStunAnything(checkStunAnything);
                }

                if (ImGui::Checkbox("Remove Launch & Knockaway Armour", &checkRemoveLaunchArmour))
                {
                    main->ToggleRemoveLaunchArmour(checkRemoveLaunchArmour);
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader("Disable Darkslayer Inputs"))
                {
                    if (ImGui::Checkbox("Disable Darkslayer Dpad Up", &checkDisableDarkslayerUp))
                    {
                        main->ToggleDisableDarkslayerUp(checkDisableDarkslayerUp);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Down", &checkDisableDarkslayerDown))
                    {
                        main->ToggleDisableDarkslayerDown(checkDisableDarkslayerDown);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Left", &checkDisableDarkslayerLeft))
                    {
                        main->ToggleDisableDarkslayerLeft(checkDisableDarkslayerLeft);
                    }

                    if (ImGui::Checkbox("Disable Darkslayer Dpad Right", &checkDisableDarkslayerRight))
                    {
                        main->ToggleDisableDarkslayerRight(checkDisableDarkslayerRight);
                    }
                }

                ImGui::Spacing();
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            CreditsDrawing();
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
    // Render dear imgui into screen
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}