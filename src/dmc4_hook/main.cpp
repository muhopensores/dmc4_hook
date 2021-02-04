
#include <windows.h>
#include <cstdio>
#include <tchar.h>
#include <iostream>

#include <filesystem>

#include "main.h"
#include "hooks.h"
#include "aobs.h"
#include "naked.h"
#include "patches.h"
#include "imgui_dmc4.h"
#include "gui_functions.h"
#include "mods.h"
#include "Mods.hpp"
#include "utils/String.hpp"
#include "utils/crash_handler.h"
#include "utils/Hash.hpp"
#include "MinHook.h"

#include "mods/modWorkRate.hpp"
#include "mods/modAreaJump.hpp"
#include "hacklib/Logging.h"
#include "hacklib/CrashHandler.h"

#include "mods/modFpsLimit.hpp"


uint32_t uninit_value = 0xCCCCCCCC;

// hmodule of dinput8.dll for GetProcAddress
HMODULE g_dinput = 0;
extern "C" {
	// DirectInput8Create wrapper for dinput8.dll
	__declspec(dllexport) HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion, const IID& riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter) {
		// i have no idea why but this gets exported as a decorated
		// function despite it being __declspec(dllexport). This is a
		// workaround to get it to load by the game.
		#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8Create@20")
		return ((decltype(DirectInput8Create)*)GetProcAddress(g_dinput, "DirectInput8Create"))(hinst, dwVersion, riidltf, ppvOut, punkOuter);
	}
}

void failed() {
	MessageBox(0, "Unable to load the original dinput8.dll.", "DMC4Hook", 0);
	ExitProcess(0);
}


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
bool checkDanteMustDie = false;
bool checkMoveIDAlloc = false;
bool checkSelectiveCancels = false;
bool checkEcstasyCancel = false;
bool checkArgumentCancel = false;
bool checkKickThirteenCancel = false;
bool checkSlashDimensionCancel = false;
bool checkPropCancel = false;
bool checkShockCancel = false;
bool checkOmenCancel = false;
bool checkKeyboardEnable = false;
bool checkSlowWalk = false;
bool checkCameraSettings = false;
bool checkSetStyle = false;
bool checkRandomEnemies = false;

hl::StaticInit<class hlMain> g_main;

hlMain* GetMain()
{
    return g_main.getMain();
}

// toggle functions to call from imgui to apply cheats

void hlMain::GamePause()
{
	m_mods->onGamePause(g_drawGUI);
}

void hlMain::ToggleStuff()
{
	// Player
		// General
		// Misc
	// System
		// General
		// Game Mode
		// Misc
	// Practice
		// General
		// Misc
}

void hlMain::shutdown() {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

void hlMain::loadSettings() {

	// General
	// Game Mode
	// Practice
	// General
	// Misc

	ToggleStuff();
	// load settings for each mod
	m_mods->onConfigLoad(*cfg);
}

void hlMain::saveSettings() {
	HL_LOG_RAW("Saving settings\n");

	// call on config save for each mod
	m_mods->onConfigSave(*cfg);

	cfg->save(m_confPath);
}

namespace fs = std::filesystem;

bool hlMain::init()
{
	wchar_t buffer[MAX_PATH]{ 0 };
	if (GetSystemDirectoryW(buffer, MAX_PATH) != 0) {
		// Load the original dinput8.dll
		if ((g_dinput = LoadLibraryW((std::wstring{ buffer } + L"\\dinput8.dll").c_str())) == NULL) {
			failed();
		}
	}
	else {
		failed();
	}
	MH_Initialize();
	bool exp_result = InstallExceptionHandlerHooks();

	auto cwd = hl::GetCurrentModulePath();
	cwd = cwd.substr(0, cwd.find_last_of("\\/"));
	m_confPath = cwd + "\\dmc4_hook.cfg";
	cfg = std::make_unique<utils::Config>( m_confPath );

	// Wait 3 seconds to let the game start.
	Sleep(3000);

    hl::LogConfig logCfg;
    logCfg.logToFile = true;
    logCfg.logTime = true;
    logCfg.fileName = "dmc4_hook_log.txt";
    hl::ConfigLog(logCfg);
    HL_LOG_RAW("======================================================\n");
    HL_LOG_RAW("                    LOG START                         \n");
    HL_LOG_RAW("======================================================\n");
	HL_LOG_RAW("\n");
	HL_LOG_RAW("dinput8.dll build date %s, commit hash #%s\n", GIT_COMMITTER_DATE, GIT_COMMIT_HASH);
	HL_LOG_RAW("\n");

    modBase = (uintptr_t)GetModuleHandle(NULL);
	
    hookD3D9(modBase);
	// init mods structure
	m_mods = std::make_unique<Mods>();
	// iterate over all the mods and call onInitialize();
	m_mods->onInitialize();
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
    replaceScarecrowLeg = modBase + 0x13F810;
    replaceScarecrowArm = modBase + 0x15E710;
    replaceMegaScarecrow = modBase + 0x15F7E0;
    replacementAddressTwo = modBase + 0x24B77B;
    replaceBiancoAngelo = modBase + 0x161A10;
    replaceAltoAngelo = modBase + 0x176C80;
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

    // hooks and jumps to get back to the correct address after hooking

	// loads settings and toggles refactored mods.
	loadSettings();
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
    return true;
}

// function to render the gui onto screen
void RenderImgui(IDirect3DDevice9* m_pDevice)
{
    auto main = GetMain(); // get ptr to hacklib main
    if (g_bWasInitialized)
    {
        g_bWasInitialized = false;
        //main->ToggleStuff();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        ImGui_ImplWin32_Init(getMainWindow());
        ImGui_ImplDX9_Init(m_pDevice);
		DarkTheme();
    }
	// I don't know why but using MouseDrawCursor draws two cursors whenever i build shit on my machine
	// this is fucked up. It should hide hardware cursor, but just doesnt for whatever reason. I'm just gonna
	// do this instead. If hardware cursor is missing draw an imgui one for people who had troubles.
	ImGui::GetIO().MouseDrawCursor = !IsCursorVisibleWINAPI();
    
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
		main->getMods()->onDrawUI("Borderless"_hash);
        /*if (ImGui::Checkbox("Borderless Window", &g_borderless))
        {
            ToggleBorderless(g_borderless);
        }*/
		// TODO(): properly position this?
        ImGui::SameLine(340.0f);
		if (ImGui::Button("Save config"))
		{
			main->saveSettings();
		}

        if (ImGui::BeginTabBar("Trainer", ImGuiTabBarFlags_FittingPolicyScroll))
        {
            if (ImGui::BeginTabItem("Character"))
            {
                ImGui::Spacing();

                ImGui::Text("Limit Removal");

				main->getMods()->onDrawUI("LimitAdjust"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                (ImGui::Text("Height Restriction Removal"));

                main->getMods()->onDrawUI("HeightRestrictionDante"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("HeightRestrictionNero"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                Misc();

                main->getMods()->onDrawUI("TrickDown"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("TrackingFullHouse"_hash);

                main->getMods()->onDrawUI("TimerMem"_hash); // instant honeycomb
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("RoseRemovesPins"_hash);

                main->getMods()->onDrawUI("EasyJc"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("FreeJc"_hash);

                main->getMods()->onDrawUI("SkipPandora"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("ForceLucifer"_hash);

                main->getMods()->onDrawUI("ManualTwosomeTime"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("NoHbKnockback"_hash); // Takes up a lot of room so leave on the right

                main->getMods()->onDrawUI("KnockbackEdits"_hash); // currently empty

                main->getMods()->onDrawUI("NoDtCooldown"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("FastPandora"_hash);

                ImGui::SameLine(205);

                main->getMods()->onDrawUI("FastSprint"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("InfAirHikes"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfSkyStars"_hash);

                main->getMods()->onDrawUI("InfTrickRange"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfDreadnought"_hash);

                main->getMods()->onDrawUI("InfTableHopper"_hash);

                ImGui::Spacing();

                main->getMods()->onDrawUI("SelectiveCancels"_hash);

                main->getMods()->onDrawUI("DisableDarkslayer"_hash);

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("General"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                main->getMods()->onDrawUI("DamageMultiplier"_hash);

                main->getMods()->onDrawUI("OneHitKill"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("NoDeath"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("InfAllHealth"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfDT"_hash);

                main->getMods()->onDrawUI("InfPlayerHealth"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfRevive"_hash);

                main->getMods()->onDrawUI("RestoreMaxHp"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Game Mode");

                static int diff_index = 0;
                ImGui::PushItemWidth(216);
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
                ImGui::PopItemWidth();

                main->getMods()->onDrawUI("BpBossRush"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("LdkWithDmd"_hash);

                main->getMods()->onDrawUI("EnemyDT"_hash);

                main->getMods()->onDrawUI("DmdBloodyPalace"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("DmdLevelAi"_hash);

                main->getMods()->onDrawUI("DtEnemiesDontStun"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("EnemyAttackOffScreen"_hash);

                main->getMods()->onDrawUI("EnemySlotting"_hash); // needs its own line
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                Misc();

                main->getMods()->onDrawUI("BerialDaze"_hash);

                ImGui::SameLine(205);

                main->getMods()->onDrawUI("StunAnything"_hash);

                main->getMods()->onDrawUI("InfFaustCloak"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("RemoveLaunchArmour"_hash);

                main->getMods()->onDrawUI("PassiveEnemies"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("FreezeEnemies"_hash);

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Environment"))
            {
				//main->m_areaJump->onGUIframe();
				main->getMods()->onDrawUI("AreaJump"_hash);
                ImGui::Separator();
                ImGui::Spacing();

				main->getMods()->onDrawUI("EnemySpawn"_hash);

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

                //main->getMods()->onDrawUI("EnemySpawn"_hash);

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("System"))
            {
                ImGui::Spacing();
                ImGui::Text("General");

                main->getMods()->onDrawUI("InfiniteTime"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("HpInOrbsDisplay"_hash);

                main->getMods()->onDrawUI("HideHud"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("DisableCameraEvents"_hash);

                main->getMods()->onDrawUI("CutsceneSkip"_hash);

                main->getMods()->onDrawUI("FreeCam"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("CameraLookdown"_hash);

                main->getMods()->onDrawUI("CameraReset"_hash);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("CharacterSwap"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("SlowWalk"_hash);

                main->getMods()->onDrawUI("BpPortal"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("DisableLastEnemyZoom"_hash);

                main->getMods()->onDrawUI("NoClip"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                Misc();
                // modBackgroundRendering::onGUIframe();
                main->getMods()->onDrawUI("BackgroundRendering"_hash);

                main->getMods()->onDrawUI("FpsLimit"_hash);

                main->getMods()->onDrawUI("WorkRate"_hash);

				main->getMods()->onDrawUI("CameraSettings"_hash);
				
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