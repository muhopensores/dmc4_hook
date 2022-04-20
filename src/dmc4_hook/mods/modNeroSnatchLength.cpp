#include "../mods.h"
#include "modNeroSnatchLength.hpp"

bool NeroSnatchLength::modEnabled{ false };
uintptr_t NeroSnatchLength::jmp_ret{ NULL };
float desiredSnatchLength = 2550.0f; // 3x lv3

naked void detour(void) {
	_asm {
			cmp byte ptr [NeroSnatchLength::modEnabled], 0
			je originalcode

			movss xmm0, [desiredSnatchLength]
			jmp dword ptr [NeroSnatchLength::jmp_ret]

		originalcode:
			movss xmm0, [ecx+eax*0x4+0x00000214]
			jmp dword ptr [NeroSnatchLength::jmp_ret]
	}
}

std::optional<std::string> NeroSnatchLength::onInitialize() {
	if (!install_hook_offset(0x03E7CBE, hook, &detour, &jmp_ret, 9)) {
		HL_LOG_ERR("Failed to init NeroSnatchLength mod\n");
		return "Failed to init NeroSnatchLength mod";
	}
	return Mod::onInitialize();
}

void NeroSnatchLength::onGUIframe() {
    ImGui::Checkbox("Increased Snatch Range", &modEnabled);
}

void NeroSnatchLength::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("nero_increased_snatch_range").value_or(false);
}

void NeroSnatchLength::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("nero_increased_snatch_range", modEnabled);
}
