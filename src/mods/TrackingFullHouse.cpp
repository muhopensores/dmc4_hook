#include "TrackingFullHouse.hpp"

bool TrackingFullHouse::tracking_full_house_nero = false;
bool TrackingFullHouse::tracking_full_house_dante = false;
uintptr_t TrackingFullHouse::jmp_ret = NULL;
uintptr_t TrackingFullHouse::jmp_out = 0x007D348A;
static float full_house_angle = 65.0f;

naked void detour(void) { // player in ebx
	_asm {
			cmp byte ptr [ebx+0x1494], 1 // controller id nero
			je NeroCheck
			cmp byte ptr [TrackingFullHouse::tracking_full_house_dante], 1
			jne originalcode
		//DanteCheck:
			cmp byte ptr [ebx+0x16D0], 1 // lockon
			je cheatcode
			jmp originalcode
		NeroCheck:
			cmp byte ptr [TrackingFullHouse::tracking_full_house_nero], 1
			jne originalcode
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

void TrackingFullHouse::on_gui_frame(int display) {
	if (display == 2) {
		ImGui::Checkbox(_("Tracking Full House"), &tracking_full_house_dante);
		ImGui::SameLine();
		help_marker(_("If the lock on target is below you (more than 65 degrees), Full House will adjust vertically like in DMC3 or 5"));
	}
}

void TrackingFullHouse::on_config_load(const utility::Config& cfg) {
	tracking_full_house_nero = cfg.get<bool>("tracking_full_house_nero").value_or(false);
    tracking_full_house_dante = cfg.get<bool>("tracking_full_house_dante").value_or(false);
}

void TrackingFullHouse::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("tracking_full_house_nero", tracking_full_house_nero);
	cfg.set<bool>("tracking_full_house_dante", tracking_full_house_dante);
}
