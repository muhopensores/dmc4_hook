
#include <windows.h>
#include <cstdio>
#include <tchar.h>
#include <iostream>
#include <tlhelp32.h>

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

#include "utils/MessageDisplay.hpp"
#include "utils/Thread.hpp"

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

void hlMain::shutdown() {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

void hlMain::saveSettings() {
	HL_LOG_RAW("Saving settings\n");

	// call on config save for each mod
	m_mods->onConfigSave();

	//cfg->save(m_confPath);
}

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

	//SteamStub shit
	uintptr_t codePtr = 0x008DB650;
	int data = *(int*)(codePtr);
	while (data != 0x5324EC83) {
		data = *(int*)(codePtr);
		Sleep(1);
	}

    std::queue<DWORD> tr = utils::SuspendAllOtherThreads();

    hl::LogConfig logCfg;
    logCfg.logToFile = true;
    logCfg.logTime = true;
    logCfg.fileName = "dmc4_hook_log.txt";
    hl::ConfigLog(logCfg);
    HL_LOG_RAW("======================================================\n");
    HL_LOG_RAW("                    LOG START                         \n");
    HL_LOG_RAW("======================================================\n");
	HL_LOG_RAW("\n");
    HL_LOG_RAW("dinput8.dll %s\n", GUI_VERSION);
	HL_LOG_RAW("\n");

    modBase = (uintptr_t)GetModuleHandle(NULL);
	
    hookD3D9(modBase);
	// init mods structure
	m_mods = std::make_unique<Mods>();
	// iterate over all the mods and call onInitialize();
	m_mods->onInitialize();

    utils::ResumeThreads(tr);

	//DISPLAY_MESSAGE("Welcome to dmc4hook.dll version FIX_FORMAT_STRINGS_BRO");
	Sleep(2000);
	m_mods->onSlowInitialize();

    return true;
}

bool hlMain::step()
{
    input.update();
    if (getMainWindow() == GetForegroundWindow()) {
        m_mods->onUpdateInput(input);
        if (input.wentDown(VK_DELETE))
        {
            g_drawGUI = !g_drawGUI;
            GamePause();
        }
    }
    return true;
}
bool bg_draw = true;

/*
struct message_handler {
	utils::bufferino<message*> m_messages;
	
	void show_messages() 
	{
		for (message* m : m_messages) 
		{
			m->show();
		}
	}

	void update_messages() 
	{
		for (message& m : m_messages) {
			if (!m.update(0.1f)) {
			}
		}
	}
};*/


void RenderBackgroundWindow() {

	if (g_bWasInitialized) { return; }

	ImGuiIO& io = ImGui::GetIO();
	ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoBackground |
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMouseInputs;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
	ImGui::SetNextWindowSize(io.DisplaySize);

	ImGui::Begin("Overlay", &bg_draw, windowFlags);
	SHOW_MESSAGES();
	UPDATE_MESSAGE();
	ImGui::End();
}

// function to render the gui onto screen
void RenderImgui(IDirect3DDevice9* m_pDevice, bool draw)
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
	ImGui::GetIO().MouseDrawCursor = false;
    
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	RenderBackgroundWindow();

	if (!draw) {
		goto imgui_finish;
	}
	ImGui::GetIO().MouseDrawCursor = !IsCursorVisibleWINAPI();
    DrawWindow();

    // specific imgui functions, can be looked up in examples or the documentation
    // references/ points to other functions to apply logic behind the gui toggles/ objects
    {
        BeginDrawing();
        ImGui::SameLine(0, 0);
        FPSDrawing();
        ImGui::Spacing();
		main->getMods()->onDrawSlowUI("Borderless"_hash);
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
            if (ImGui::BeginTabItem("General"))
            {
                ImGui::Spacing();

                ImGui::Text("General");

                ImGui::Spacing();

                main->getMods()->onDrawUI("DamageMultiplier"_hash); // needs its own line

                //main->getMods()->onDrawUI("OneHitKill"_hash);

                main->getMods()->onDrawUI("InfAllHealth"_hash);

                main->getMods()->onDrawUI("NoDeath"_hash); // needs its own line

                main->getMods()->onDrawUI("RestoreMaxHp"_hash); // needs its own line

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Enemy Difficulty");

                ImGui::Spacing();

                main->getMods()->onDrawUI("DifficultySelect"_hash); // needs its own line

                main->getMods()->onDrawUI("EnemyDT"_hash);

                main->getMods()->onDrawUI("DmdLevelAi"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("LdkWithDmd"_hash);

                main->getMods()->onDrawUI("EnemyAttackOffScreen"_hash);

                main->getMods()->onDrawUI("EnemySlotting"_hash); // needs its own line

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Bloody Palace");

                ImGui::Spacing();

                main->getMods()->onDrawUI("BpJumpHook"_hash);

                //main->getMods()->onDrawUI("BpBossRush"_hash);
                //ImGui::SameLine(205);
                main->getMods()->onDrawUI("DmdBloodyPalace"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Enemy Options");

                ImGui::Spacing();

                main->getMods()->onDrawUI("StunAnything"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("RemoveLaunchArmour"_hash);

                main->getMods()->onDrawUI("DtEnemiesDontStun"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfFaustCloak"_hash);

                main->getMods()->onDrawUI("PassiveEnemies"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("FreezeEnemies"_hash);

                main->getMods()->onDrawUI("DisableChimeraBlades"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("BerialDaze"_hash);

                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Character"))
            {
                ImGui::Spacing();

                ImGui::Text("Limit Removal");

                ImGui::Spacing();

                main->getMods()->onDrawUI("LimitAdjust"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                (ImGui::Text("Height Restriction Removal"));

                ImGui::Spacing();

                main->getMods()->onDrawUI("HeightRestrictionDante"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("HeightRestrictionNero"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Character Abilities");

                ImGui::Spacing();

                main->getMods()->onDrawUI("TrackingFullHouse"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("TrickDown"_hash);

                main->getMods()->onDrawUI("RoseRemovesPins"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("ForceLucifer"_hash);

                main->getMods()->onDrawUI("TimerMem"_hash); // instant honeycomb
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("SkipPandora"_hash);

                main->getMods()->onDrawUI("ManualTwosomeTime"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("NoHbKnockback"_hash); // needs its own line

                main->getMods()->onDrawUI("KnockbackEdits"_hash); // currently empty

                main->getMods()->onDrawUI("NoDtCooldown"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("NoClip"_hash);

                main->getMods()->onDrawUI("EasyJc"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("FreeJc"_hash);

                main->getMods()->onDrawUI("FastPandora"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("FastSprint"_hash);

                main->getMods()->onDrawUI("NeroFullHouse"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Shared Cheats");

                ImGui::Spacing();

                main->getMods()->onDrawUI("InfRevive"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfPlayerHealth"_hash);

                main->getMods()->onDrawUI("InfDT"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfAirHikes"_hash);

				main->getMods()->onDrawUI("BigHeadMode"_hash);
				
				main->getMods()->onDrawUI("ModTwCmdSuperhot"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Dante Cheats");

                ImGui::Spacing();

                main->getMods()->onDrawUI("InfTrickRange"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfSkyStars"_hash);

                main->getMods()->onDrawUI("InfDreadnought"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("RgMultiplier"_hash);

                main->getMods()->onDrawUI("ActiveBlock"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Nero Cheats");

                ImGui::Spacing();

                main->getMods()->onDrawUI("InfTableHopper"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("InfCalibur"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("SelectiveCancels"_hash);

                main->getMods()->onDrawUI("DisableDarkslayer"_hash);

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
				main->getMods()->onDrawUI("Quicksilver"_hash);
                ImGui::Spacing();

                ImGui::Text("HUD");

                ImGui::Spacing();

                main->getMods()->onDrawUI("InfiniteTime"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("HpInOrbsDisplay"_hash);

                main->getMods()->onDrawUI("HideHud"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Cutscenes");

                ImGui::Spacing();

                main->getMods()->onDrawUI("DisableCameraEvents"_hash);

                main->getMods()->onDrawUI("CutsceneSkip"_hash);
                
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text("Misc");

                ImGui::Spacing();

                main->getMods()->onDrawUI("CharacterSwap"_hash);
                ImGui::SameLine(205);
                main->getMods()->onDrawUI("NoAutomaticCharacters"_hash);
                main->getMods()->onDrawUI("SlowWalk"_hash);

                main->getMods()->onDrawUI("BpPortal"_hash); // needs its own line

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                
                ImGui::Text("System");

                ImGui::Spacing();
				main->getMods()->onDrawUI("TwCmdPlayerTransforms"_hash);
				main->getMods()->onDrawUI("FastStart"_hash);

                main->getMods()->onDrawSlowUI("BackgroundRendering"_hash);

                main->getMods()->onDrawUI("FpsLimit"_hash);

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                main->getMods()->onDrawUI("WorkRate"_hash);

				main->getMods()->onDrawUI("CameraSettings"_hash);
				
				main->getMods()->onDrawUI("TwitchClient"_hash);
                ImGui::Spacing();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Debug"))
            {
                main->getMods()->onDrawUI("PlayerTracker"_hash); // needs its own line

                ImGui::Spacing();
                ImGui::EndTabItem();
            }
            CreditsDrawing();
            ImGui::EndTabBar();
        }
        ImGui::End();
    }
imgui_finish:
    // Render dear imgui into screen
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}