#include "../mods.h"
#include "modBerialDaze.hpp"

bool BerialDaze::modEnabled{ false };
uintptr_t BerialDaze::jmp_ret{ NULL };

BerialDaze::BerialDaze()
{
	//onInitialize();
}

naked void detour(void)
{
	_asm {
			cmp byte ptr [BerialDaze::modEnabled], 0
			je originalcode

			movss xmm0, [esi+0x00001B70]
			addss xmm0, [esi+0x00001B70]
			movss [esi+0x1B80], xmm0
			jmp dword ptr [BerialDaze::jmp_ret]
		
		originalcode:
            movss xmm0, [esi+0x00001B80]
			jmp dword ptr [BerialDaze::jmp_ret]
	}
}

void BerialDaze::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x23515B, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else
    {
        patch.revert();
    }
}

std::optional<std::string> BerialDaze::onInitialize() {

	if (!install_hook_offset(0x024271A, hook, &detour, &jmp_ret, 8))
    {
		HL_LOG_ERR("Failed to init BerialDaze mod\n");
		return "Failed to init BerialDaze mod";
	}
	return Mod::onInitialize();
}

void BerialDaze::onGUIframe() {

    if (ImGui::Checkbox("Berial Daze", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void BerialDaze::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("berial_practice").value_or(false);
    toggle(modEnabled);
};

void BerialDaze::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("berial_practice", modEnabled);
};
