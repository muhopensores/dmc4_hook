
#include "NeroSnatchLength.hpp"

bool NeroSnatchLength::mod_enabled{ false };
uintptr_t NeroSnatchLength::jmp_ret{ NULL };
static float desired_snatch_length = 2550.0f; // 3x lv3

naked void detour(void) {
	_asm {
			cmp byte ptr [NeroSnatchLength::mod_enabled], 0
			je originalcode

			movss xmm0, [desired_snatch_length]
			jmp dword ptr [NeroSnatchLength::jmp_ret]

		originalcode:
			movss xmm0, [ecx+eax*0x4+0x00000214]
			jmp dword ptr [NeroSnatchLength::jmp_ret]
	}
}

std::optional<std::string> NeroSnatchLength::on_initialize() {
	if (!install_hook_offset(0x03E7CBE, hook, &detour, &jmp_ret, 9)) {
		spdlog::error("Failed to init NeroSnatchLength mod\n");
		return "Failed to init NeroSnatchLength mod";
	}
	return Mod::on_initialize();
}

void NeroSnatchLength::on_gui_frame(int display) {
    ImGui::Checkbox(_("Increased Snatch Range"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Triple the reach on Snatch"));
}

void NeroSnatchLength::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("nero_increased_snatch_range").value_or(false);
}

void NeroSnatchLength::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_increased_snatch_range", mod_enabled);
}
