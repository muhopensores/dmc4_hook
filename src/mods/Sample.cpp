// include your mod header file
#include "Sample.hpp"

#if 0
static uintptr_t  jmp_return { NULL };
static bool mod_enabled{ false };
//uintptr_t  ModSample::some_shared_ptr{ NULL };

// detour 
// naked is defined to __declspec(naked)
naked void detour() {
	__asm {
	}
}

std::optional<std::string> ModSample::onInitialize() {
	return Mod::onInitialize();
}

// onFrame()
// do something each frame example
//void ModSample::onFrame(fmilliseconds& dt) {}

// onConfigSave
// save your data into cfg structure.
//void ModSample::onConfigSave(utility::Config& cfg) { cfg.set<variable_type>("config_string",variable_name); };

// onConfigLoad
// load data into variables from config structure.
//void ModSample::onConfigLoad(const utility::Config& cfg) { //variable_name = cfg.get<variable_type>("config_string").value_or(default_value); };

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
//void ModSample::onGUIframe() { ImGui::Text("ModSample text"); };

// onGamePause()
// do something when toggling a gui
//void ModName::onGamePause(bool toggle) { };

// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
//bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

#endif
