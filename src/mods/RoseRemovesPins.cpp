#include "RoseRemovesPins.hpp"

bool RoseRemovesPins::mod_enabled = false;
uintptr_t RoseRemovesPins::jmp_ret = NULL;
uintptr_t RoseRemovesPins::jmp_out = 0x00815970;

naked void detour(void) {
	_asm {
			cmp byte ptr [RoseRemovesPins::mod_enabled], 0
			je originalcode

			cmp byte ptr [esi+0x14], 04
			je removepinsje
			cmp byte ptr [esi+0x14], 03
			je threecode
			jmp dword ptr [RoseRemovesPins::jmp_ret]

		originalcode:
			cmp byte ptr [esi+0x14], 04
			je removepinsje
			jmp dword ptr [RoseRemovesPins::jmp_ret]

		threecode:
			mov byte ptr [esi+0x14], 00
		removepinsje:
			jmp dword ptr [RoseRemovesPins::jmp_out]
	}
}

std::optional<std::string> RoseRemovesPins::on_initialize() {
	if (!install_hook_offset(0x04158C3, hook, &detour, &jmp_ret, 10)) {
		spdlog::error("Failed to init RoseRemovesPins mod\n");
		return "Failed to init RoseRemovesPins mod";
	}
	return Mod::on_initialize();
}

void RoseRemovesPins::on_gui_frame(int display) {
    ImGui::Checkbox(_("Rose Removes Pins"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Rose will despawn pins rather than detonating them"));
}

void RoseRemovesPins::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("rose_removes_pins").value_or(false);
}

void RoseRemovesPins::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rose_removes_pins", mod_enabled);
}
