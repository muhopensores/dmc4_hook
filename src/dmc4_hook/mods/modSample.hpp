#pragma once

#if 0
#include "../mod.hpp"

// Define ModName class that is a Mod
class ModName : public Mod {
public:
	// using default constructor
	ModName() = default;

	// static variables that you might want to access from other mods and assembly detours.
	// static variables are not associated with class instance,
	// they are stored in the .data section of a dll.
	// define like this: static_keyword type var_name;
	// initialize them in the cpp file like this:
	// type ModName::var_name = value;
	// or type ModName::var_name { value };
	// use them within assembly caves or other modules by specifying:
	// class_name::static_variable_name
	static uint32_t   some_shared_value;
	static uintptr_t  some_shared_ptr;
	// override getModName() method with your mod name for logging.
	std::string getModName() override { return "ModName"; };

	// override onInitialize() method with routines required to initialize the mod.
	// return Mod::onInitialize() on success or error string on failure.
	std::optional<std::string> onInitialize() override;

	// callbacks
	// onFrame() gets called every frame whether or not gui shows up or not
	// override this method if you want to do something every frame
	void onFrame(fmilliseconds& dt) override;
	// onConfigLoad() gets called on load during hl::init()
	// override this method if you want to load settings from config.
	void onConfigLoad(const utils::Config& cfg) override;
	// onConfigSave() gets called on save when pressing save config button in gui.
	// override this method if you want to save settings to config.
	void onConfigSave(utils::Config& cfg) override;
	// onGUIframe() gets called when imgui window is on the screen.
	// override this method if you want to display something in the gui.
	void onGUIframe() override;
	// onGamePause() gets called from hl::Main when toggling a gui.
	// override this method if you want to do something when toggling a gui.
	// bad name maybe change later.
	// (only used by workrate mod atm)
	void onGamePause(bool toggle) override;
	// onMessage() gets called from window hook.
	// override this method if you want your mod to respond to window messages
	// Return true if you want message handled by the game or drop it by returning false.
	// (only used by borderless mod atm)
	bool onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
private:
	// hook instance if you need to apply detours
	hl::Hooker hook;
	// patch instance if you need to apply patch
	hl::Patch patch;
	// random variable
	int* variable = (int*)0xDEADBEEF;
	int some_config_variable;
	bool mod_name_enabled{ false };
};
#endif