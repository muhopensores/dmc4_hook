#include "../mods.h"
#include "modFastStart.hpp"

bool FastStart::modEnabled{ false };

static uintptr_t jmp_ret;

static bool checkClassName(MtDTI* dti) {
	char* aTitle = "aInit";
	return strcmp(dti->mName, aTitle) == 0;
}

static void loadSaveSlot() {
	uintptr_t func = 0x004A03B0; // load function ptr
	_asm {
		mov edx, 0x00E55E48 // sPCSave
		mov edx, [edx]
		mov eax, 0 // slot?
		push edx
		call dword ptr [func]
	}
}

naked void detour(void)
{
	_asm {
		cmp byte ptr[FastStart::modEnabled], 0
		je originalCode

		cmp ecx, 0x00E55C88 // aAdvertise
		je aAdv
		cmp ecx, 0x00E55D88 // aInit
		je aInit
		jmp originalCode
	aAdv:
		mov ecx, 0x00E55E48 // aMissionSelect
		jmp originalCode

		; 0x00E55CE8 // aGame
		; 0x00E55C88 // aAdvertise

		; mov ecx, 0x00E56BC8 // aTitle
		; mov ecx, 0x00E55E48  // aMissionSelect
	aInit:
		mov ecx, 0x00E55CE8 // aGame
		pushad
		call loadSaveSlot //load save in slot 0
		popad

	originalCode:

		mov edx, [ecx]
		mov eax, [edx+4h]

		jmp dword ptr [jmp_ret]
	}
}

void FastStart::toggle(bool enable)
{
    if (enable)
    {
		//float one = 1.0f;
        //install_patch_offset(0x4996B5, patch_menu, "\x07", 1);
		//just a memcpy with VirtualProtect
		//install_patch_offset(0x810268, patch_mt01, (char*)&one, sizeof(float));
		//install_patch_offset(0x79A27C, patch_mt02, (char*)&one, sizeof(float));

    }
    else
    {
		patch_menu.revert();
		patch_mt01.revert();
		patch_mt02.revert();
    }
}

std::optional<std::string> FastStart::onInitialize()
{
	install_hook_absolute(0x008DB77D, hook_dti, detour, &jmp_ret, 5);

	return Mod::onInitialize();
}


void FastStart::onGUIframe()
{
	ImGui::Checkbox("Fast Game Load", &modEnabled);
}

void FastStart::onConfigLoad(const utils::Config& cfg)
{
	modEnabled = cfg.get<bool>("fast_load").value_or(false);
	toggle(modEnabled);
};

void FastStart::onConfigSave(utils::Config& cfg)
{
	cfg.set<bool>("fast_load", modEnabled);
};
