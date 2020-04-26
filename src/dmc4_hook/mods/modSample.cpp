// include your mod header file
#include "modSample.hpp"

#if 1
// static variable defined in cpp file makes it local to current file
static uintptr_t  jmp_return { NULL };
// initialzie our static vairables
// two example styles of assigment use whatever you want
// brace assigment example (muh c++)
uintptr_t  ModName::some_shared_ptr{ NULL };
// regular assigment 
uint32_t   ModName::some_shared_value = 1;

// detour 
// naked is defined to __declspec(naked)
naked void detour() {
	__asm {
		// store something into some_shared_value
		// example of using class static variables
		// note use of class name and scope resolution operator ::
		mov DWORD PTR [ModName::some_shared_value], eax
		mov [eax+20h],ecx
		mov DWORD PTR [ModName::some_shared_ptr], ecx
		jmp DWORD PTR [jmp_return]
	}
}
// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> ModName::onInitialize() {
	// you can use find pattern and all the other stuff here also.
	// maybe do a memcmp to check if the place you are installing the hook at contains proper bytes.
	// https://en.cppreference.com/w/cpp/string/byte/memcmp
	
	// install_hook_offset takes offset from exe base, hl::Hooker instance,
	// pointer to naked function, pointer to return address and next instruction offset
	if (!install_hook_offset(0xBADF00D, hook, &detour, &jmp_return, 6)) {
		return "Failed to init ModName mod";
	}
	// some patch example
	if (mod_name_enabled) {
		patch.apply(0x400000, "\x90\x90\x90\x90");
	}
	return Mod::onInitialize();
}

// onFrame()
// do something each frame example
void ModName::onFrame() {
	// inside class methods you can access static variables without scope specifier.
	if (some_shared_value && mod_name_enabled) { *variable = 1; }
	else { *variable = 0; }
}
// onConfigSave
// save your data into cfg structure.
void ModName::onConfigSave(utils::Config& cfg) {
	//cfg.set<variable_type>("config_string",variable_name);
	cfg.set<bool>("mod_name_enabled", mod_name_enabled);
	cfg.set<int>("some_config_variable", some_config_variable);
};
// onConfigLoad
// load data into variables from config structure.
void ModName::onConfigLoad(const utils::Config& cfg) {
	// value_or sets default value in case config file is not available or missing.
	//variable_name = cfg.get<variable_type>("config_string").value_or(default_value);
	mod_name_enabled = cfg.get<bool>("mod_name_enabled").value_or(false);
	some_config_variable = cfg.get<int>("some_config_variable").value_or(0);
};

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void ModName::onGUIframe() { ImGui::Text("ModName text"); };
// onGamePause()
// do something when toggling a gui
void ModName::onGamePause(bool toggle) { };
// onMessage()
// handle some window message, return true to pass to the game window
// or false to drop it.
bool ModName::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };

#endif