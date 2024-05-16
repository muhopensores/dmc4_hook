#include "TrackingFullHouse.hpp"

bool TrackingFullHouse::mod_enabled{ false };
uintptr_t TrackingFullHouse::jmp_ret{ NULL };
uintptr_t TrackingFullHouse::jmp_out{ 0x007D348A };
static float full_house_angle{ 65.0 };

naked void detour(void) { // player in ebx
	_asm {
			cmp byte ptr [TrackingFullHouse::mod_enabled], 0
			je originalcode
			cmp byte ptr [ebx+0x16D0], 1 // lockon
			je cheatcode

		originalcode:
			comiss xmm0, [full_house_angle] // 65 float
			jna jnacode // DevilMayCry4_DX9.exe+3D348A

		cheatcode:
			jmp dword ptr [TrackingFullHouse::jmp_ret]

		jnacode:
			jmp dword ptr [TrackingFullHouse::jmp_out]
	}
}

std::optional<std::string> TrackingFullHouse::on_initialize() {
	if (!install_hook_offset(0x003D3471, hook, &detour, &jmp_ret, 7)) {
		spdlog::error("Failed to init TrackingFullHouse mod\n");
		return "Failed to init TrackingFullHouse mod";
	}
	return Mod::on_initialize();
}

void TrackingFullHouse::on_gui_frame() {
    ImGui::Checkbox(_("Tracking Full House"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("If the lock on target is below you (more than 65 degrees), Full House will adjust vertically like in DMC3 or 5"));
}

void TrackingFullHouse::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("tracking_full_house").value_or(false);
}

void TrackingFullHouse::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("tracking_full_house", mod_enabled);
}
