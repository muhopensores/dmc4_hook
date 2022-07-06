#pragma once

#if 0
#include "Mod.hpp"

// Define ModName class that is a Mod
class ModSample : public Mod {
public:
	ModSample() = default;

	// override getModName() method with your mod name for logging.
	std::string getModName() override { return "ModSample"; };

	// override onInitialize() method with routines required to initialize the mod.
	// return Mod::onInitialize() on success or error string on failure.
	std::optional<std::string> onInitialize() override;

	// callbacks
	
	// onFrame() gets called every frame whether or not gui shows up or not
	// override this method if you want to do something every frame
	//void onFrame(fmilliseconds& dt) override;
	
	// onConfigLoad() gets called on load during hl::init()
	// override this method if you want to load settings from config.
	//void onConfigLoad(const utility::Config& cfg) override;
	
	// onConfigSave() gets called on save when pressing save config button in gui.
	// override this method if you want to save settings to config.
	//void onConfigSave(utility::Config& cfg) override;
	
	// onGUIframe() gets called when imgui window is on the screen.
	// override this method if you want to display something in the gui.
	//void onGUIframe() override;
	
	// onGamePause() gets called from hl::Main when toggling a gui.
	// override this method if you want to do something when toggling a gui.
	// (only used by workrate mod atm)
	//void onGamePause(bool toggle) override;
	
	// onMessage() gets called from window hook.
	// override this method if you want your mod to respond to window messages
	// Return true if you want message handled by the game or drop it by returning false.
	// (only used by borderless mod atm)
	//bool onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
private:
	// hook instance if you need to apply detours
	//std::unique_ptr<FunctionHook> hook;
	// patch instance if you need to apply patch
	//std::unique_ptr<Patch> patch;
	// random variable
	//bool mod_name_enabled{ false };
};
#endif
