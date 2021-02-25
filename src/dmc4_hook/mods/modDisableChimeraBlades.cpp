#include "../mods.h"
#include "modDisableChimeraBlades.hpp"

bool DisableChimeraBlades::modEnabled{ false };
uintptr_t DisableChimeraBlades::jmp_ret{ NULL };
uintptr_t jmp_jne{ 0x05F1B0F };

DisableChimeraBlades::DisableChimeraBlades()
{
	//onInitialize();
}

naked void detour(void)
{
	_asm {
			cmp byte ptr [DisableChimeraBlades::modEnabled], 0
			je originalcode

			cmp byte ptr [esi+00001438h],00
			jmp jnecode

			originalcode:
			cmp byte ptr [esi+00001438h],00
			jne jnecode
			jmp dword ptr [DisableChimeraBlades::jmp_ret]

			jnecode:
			jmp dword ptr [jmp_jne]
	}
}

std::optional<std::string> DisableChimeraBlades::onInitialize()
{

	if (!install_hook_offset(0x01F1A8F, hook, &detour, &jmp_ret, 9))
    {
		HL_LOG_ERR("Failed to init DisableChimeraBlades mod\n");
		return "Failed to init DisableChimeraBlades mod";
	}
	return Mod::onInitialize();
}

void DisableChimeraBlades::onGUIframe()
{
    ImGui::Checkbox("Disable Chimera Blades", &modEnabled);
}

void DisableChimeraBlades::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("disable_chimera_blades").value_or(false);
};

void DisableChimeraBlades::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("disable_chimera_blades", modEnabled);
};
