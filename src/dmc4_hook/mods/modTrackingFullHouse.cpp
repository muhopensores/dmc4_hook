#include "../mods.h"
#include "modTrackingFullHouse.hpp"
#include "modPlayerTracker.hpp"

bool TrackingFullHouse::modEnabled{ false };
uintptr_t TrackingFullHouse::jmp_ret{ NULL };
uintptr_t TrackingFullHouse::jmp_out{ 0x007D348A };
float fullHouseAngle{ 65.0 };

naked void detour(void) { // player in ebx
	_asm {
			cmp byte ptr [TrackingFullHouse::modEnabled], 0
			je originalcode
			cmp byte ptr [ebx+0x16D0], 1 // lockon
			je cheatcode

		originalcode:
			comiss xmm0, [fullHouseAngle] // 65 float
			jna jnacode // DevilMayCry4_DX9.exe+3D348A

		cheatcode:
			jmp dword ptr [TrackingFullHouse::jmp_ret]

		jnacode:
			jmp dword ptr [TrackingFullHouse::jmp_out]
	}
}

std::optional<std::string> TrackingFullHouse::onInitialize() {
	if (!install_hook_offset(0x003D3471, hook, &detour, &jmp_ret, 7)) {
		HL_LOG_ERR("Failed to init TrackingFullHouse mod\n");
		return "Failed to init TrackingFullHouse mod";
	}
	return Mod::onInitialize();
}

void TrackingFullHouse::onGUIframe() {
    ImGui::Checkbox("Tracking Full House", &modEnabled);
}

void TrackingFullHouse::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("tracking_full_house").value_or(false);
}

void TrackingFullHouse::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("tracking_full_house", modEnabled);
}
