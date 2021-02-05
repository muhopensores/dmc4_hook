
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

                main->getMods()->onDrawUI("DifficultySelect"_hash);

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
                ImGui::Spacing();

				main->getMods()->onDrawUI("AreaJump"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

				main->getMods()->onDrawUI("EnemySpawn"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("EnemyReplace"_hash);

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