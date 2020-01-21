#include "../mods.h"

// expose functions/variables you need to call from outside in mods.h
namespace modSample {

	constexpr auto mod_aob = "11 22 33 ?? 55";

	// mod variables
	static bool enabled;
	static uintptr_t location;
	static uintptr_t locationPatch;
    static hl::Patch patch;
	static hl::Hooker hook;

	// using detours should be the same as before.
	extern "C" uintptr_t detour_jmp = 0xDEADBEEF;
	_declspec(naked) void detour_proc(void) {
		_asm {
			; some rubbish asm
			mov al, [enabled]
			test al, al
			jmp dword ptr [detour_jmp]
		}
	}

	/* call this in hlMain::init() to initialize the mod, check the return value in case it fails
	you can optionally pass the data for the function for example module base.
	*/
	//bool init(uintptr_t modBase) {
	bool init() {
		location = hl::FindPattern(mod_aob);
		hook.hookJMP(location, 5, &detour_proc);
		return true;
	};
	/* toggling patches or other required mods here. */
	void toggle(bool value) {
		if (value)
			patch.apply(locationPatch, "\x90\x90", 2);
		else
			patch.revert();
	};
	/* call from the imgui drawing routine to draw gui elements if you require */
	void onGUIframe() {
		if (ImGui::Checkbox("mod_gui_entry", &enabled)) {
			toggle(enabled);
		}
	}
	/* call this in config.cpp/updateMods(); to load values from the config and toggle upon start */
	void loadConfig(CONFIG& config) {
		// only define options you want to be saved/loaded
		enabled = config.player.options["mod_config_entry"];
		// iterate over the list and toggle values according to config data.
		toggle(enabled);
	}
	/* call this in config.cpp/updateConfig(); to save values into CONFIG structure 
	   to be later saved into an ini file.
	*/
	void saveConfig(CONFIG& config) {

		config.player.options["mod_config_entry"] = enabled;
	}
};