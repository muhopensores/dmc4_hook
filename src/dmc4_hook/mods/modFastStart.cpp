#include "../mods.h"
#include "modFastStart.hpp"

bool FastStart::modEnabled{ false };

static uintptr_t jmp_ret;

bool checkClassName(MtDTI* dti) {
	char* aTitle = "aInit";
	return strcmp(dti->mName, aTitle) == 0;
}

naked void detour(void)
{
	_asm {
		cmp byte ptr [FastStart::modEnabled],0
		je originalCode
		
		pushad
		push ecx
		call checkClassName
		test al,al
		pop ecx
		popad
		
		je originalCode
		mov ecx, 0x00E56BC8

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
        install_patch_offset(0x4996B5, patch_menu, "\x07", 1);
    }
    else
    {
		patch_menu.revert();
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
