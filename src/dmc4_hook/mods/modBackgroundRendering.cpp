#include "MinHook.h"
#include "../hooks.h"
#include "modBackgroundRendering.hpp"
// expose functions/variables you need to call from outside in mods.h

typedef HWND (WINAPI *GETFOCUS)(VOID);
typedef HWND (WINAPI *GETFOREGROUNDWINDOW)(VOID);

GETFOCUS            fpGetFocus            = NULL;
GETFOREGROUNDWINDOW fpGetForegroundWindow = NULL;

HWND modGameWindow{ 0 };
bool modEnabled{ false };

#if 1
HWND WINAPI DetourGetForegroundWindow() {
	if (modEnabled) {
		return modGameWindow;
	}
	else
		return fpGetForegroundWindow();
}

HWND WINAPI DetourGetFocus() {
	if (modEnabled) {
		return modGameWindow;
	}
	else
		return fpGetForegroundWindow();
}



std::optional<std::string> BackgroundRendering::onInitialize() {
	modGameWindow = FindWindowA(NULL, "DEVIL MAY CRY 4");
	if (!modGameWindow) {
		throw std::runtime_error("[BackgroundRendering] WINDOW NOT FOUND\n");
	}

	if (MH_CreateHookApi(L"user32", "GetForegroundWindow", &DetourGetForegroundWindow, (LPVOID*)&fpGetForegroundWindow) == MH_OK) {
		HL_LOG_RAW("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) returned MH_OK\n");
	}
	else {
		HL_LOG_ERR("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed";
	}
	if (MH_EnableHook(&GetForegroundWindow) == MH_OK) {
		HL_LOG_RAW("[BackgroundRendering]: EnableHook(&GetForegroundWindow) returned MH_OK\n");
	}
	else {
		HL_LOG_RAW("[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed";
	}

	if (MH_CreateHookApi(L"user32", "GetFocus", &DetourGetFocus, (LPVOID*)&fpGetFocus) == MH_OK) {
		HL_LOG_RAW("[BackgroundRendering]: CreateHookApi(user32, GetFocus) returned MH_OK\n");
	}
	else {
		HL_LOG_ERR("[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed";
	}
	if (MH_EnableHook(&GetFocus) == MH_OK) {
		HL_LOG_RAW("[BackgroundRendering]: EnableHook(&GetFocus) returned MH_OK\n");
	}
	else {
		HL_LOG_RAW("[BackgroundRendering]: EnableHook(&GetFocus) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetFocus) failed";
	}
	return Mod::onInitialize();
}

void BackgroundRendering::onConfigLoad(const utils::Config& cfg) {
	modEnabled = cfg.get<bool>("enable_focus_patch").value_or(false);
}

void BackgroundRendering::onConfigSave(utils::Config& cfg) {
	cfg.set<bool>("enable_focus_patch", modEnabled);
}

void BackgroundRendering::onGUIframe() {
	ImGui::Checkbox("Background Input", &modEnabled);
}

bool* BackgroundRendering::getModEnabledPtr() {
	return &modEnabled;
}

#else
namespace modBackgroundRendering {

	static bool modEnabled;
	static HWND modGameWindow;
	// uncomment this patches if you want instead of hooking GetFocus to disable
	// muting audio on focus loss.
	//static uintptr_t soundPatchLocation01;
	//static uintptr_t soundPatchLocation02;
	//static hl::Patch patchSound01;
	//static hl::Patch patchSound02;

	bool* getModEnabledPtr() {
		return &modEnabled;
	}

    HWND WINAPI DetourGetForegroundWindow() {
        if (modEnabled) {
            return modGameWindow;
        }
        else
            return fpGetForegroundWindow();
    }

	HWND WINAPI DetourGetFocus() {
		if (modEnabled) {
			return modGameWindow;
		}
		else
			return fpGetForegroundWindow();
	}

    /* call this in hlMain::init() to initialize the mod, check the return value in case it fails
	you can optionally pass the data for the function for example module base.
	*/
	bool init(HWND gameWindow, uintptr_t modBase) {
		modGameWindow = gameWindow;
		if (MH_CreateHookApi(L"user32", "GetForegroundWindow", &DetourGetForegroundWindow, (LPVOID*)&fpGetForegroundWindow) == MH_OK) {
			HL_LOG_RAW("[minhook]: CreateHookApi(user32, GetForegroundWindow) returned MH_OK\n");
		}
		else {
			HL_LOG_ERR("[minhook]: CreateHookApi(user32, GetForegroundWindow) failed\n");
			return false;
		}
		if (MH_EnableHook(&GetForegroundWindow) == MH_OK) {
			HL_LOG_RAW("[minhook]: EnableHook(&GetForegroundWindow) returned MH_OK\n");
		}
		else {
			HL_LOG_RAW("[minhook]: EnableHook(&GetForegroundWindow) failed\n");
			return false;
		}

		if (MH_CreateHookApi(L"user32", "GetFocus", &DetourGetFocus, (LPVOID*)&fpGetFocus) == MH_OK) {
			HL_LOG_RAW("[minhook]: CreateHookApi(user32, GetFocus) returned MH_OK\n");
		}
		else {
			HL_LOG_ERR("[minhook]: CreateHookApi(user32, GetFocus) failed\n");
			return false;
		}
		if (MH_EnableHook(&GetFocus) == MH_OK) {
			HL_LOG_RAW("[minhook]: EnableHook(&GetFocus) returned MH_OK\n");
		}
		else {
			HL_LOG_RAW("[minhook]: EnableHook(&GetFocus) failed\n");
			return false;
		}

		//soundPatchLocation01 = modBase + 0x8725B;
		//soundPatchLocation02 = modBase + 0x87263;

		// if everything went well register our ini callbacks.
		// those will be called at ini load/save.
		/*loadCallbackRegister(loadConfig);
		saveCallbackRegister(saveConfig);*/

		return true;
	};
	
	/*void toggle(bool value) {
		modEnabled = value;
		if (value) {
			//patchSound01.apply(soundPatchLocation01, "\x90\x90", 2);
			//patchSound02.apply(soundPatchLocation02, "\xEB", 1);
		}
		else {
			//patchSound01.revert();
			//patchSound02.revert();
		}
	};*/


	// will be called during config.cpp/updateConfig() if you've set your callbacks
	// to load values from the config and toggle upon start
	void onConfigLoad(const utils::Config& cfg) {
		modEnabled = cfg.get<bool>("enable_focus_patch").value_or(false);
	};
	// will be called during config.cpp/updateConfig() if you've set your callbacks
	// to write values into config file
	void onConfigSave(utils::Config& cfg) {
		cfg.set<bool>("enable_focus_patch", modEnabled);
	};

	// call from the imgui drawing routine to draw gui elements if you require
	void onGUIframe() {
		/*if (ImGui::Checkbox("Focus patch (background input)", &modEnabled)) {
			toggle(modEnabled);
		}*/
		ImGui::Checkbox("Focus patch (background input)", &modEnabled);
	}
};
#endif
