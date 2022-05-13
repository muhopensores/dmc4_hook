#include "../mods.h"
#include "modInfPlayerHealth.hpp"

bool InfPlayerHealth::modEnabled{ false };
uintptr_t InfPlayerHealth::jmp_ret{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [InfPlayerHealth::modEnabled], 0
			je originalcode

			mov dword ptr [ebp+0x000015CC], 0x469C4000

		originalcode:
            movss xmm0,[ebp+0x000015CC]
			jmp dword ptr [InfPlayerHealth::jmp_ret]
	}
}

std::optional<std::string> InfPlayerHealth::onInitialize() {
	if (!install_hook_offset(0x00FF282, hook, &detour, &jmp_ret, 8)) {
		HL_LOG_ERR("Failed to init InfPlayerHealth mod\n");
		return "Failed to init InfPlayerHealth mod";
	}
	return Mod::onInitialize();
}

void InfPlayerHealth::onGUIframe() {
    ImGui::Checkbox("Freeze Player HP To Max", &modEnabled);
}

void InfPlayerHealth::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("infinite_player_health").value_or(false);
}

void InfPlayerHealth::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("infinite_player_health", modEnabled);
}
