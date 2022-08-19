
#include "RoseRemovesPins.hpp"

bool RoseRemovesPins::mod_enabled{ false };
bool RoseRemovesPins::mod_enabled2{false};
uintptr_t RoseRemovesPins::jmp_ret{ NULL };
uintptr_t RoseRemovesPins::jmp_out{ 0x00815970 };

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

void RoseRemovesPins::no_pin_timer(bool enable) {
    if (enable) {
        install_patch_offset(0x04135F6, patch, "\xEB\x14", 2); // not in enemy
        install_patch_offset(0x041362C, patch2, "\xEB\x12", 2); // in enemy
    }
    else {
        patch.reset();
        patch2.reset();
    }
}

std::optional<std::string> RoseRemovesPins::on_initialize() {
	if (!install_hook_offset(0x04158C3, hook, &detour, &jmp_ret, 10)) {
		spdlog::error("Failed to init RoseRemovesPins mod\n");
		return "Failed to init RoseRemovesPins mod";
	}
	return Mod::on_initialize();
}

void RoseRemovesPins::on_gui_frame() {
    ImGui::Checkbox("Rose Removes Pins", &mod_enabled);
    ImGui::SameLine();
    help_marker("Rose despawns pins rather than detonating them");
    ImGui::SameLine(205);
    if (ImGui::Checkbox("Infinite Pin Timer", &mod_enabled2)) {
        no_pin_timer(mod_enabled2);
    }
}

void RoseRemovesPins::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("rose_removes_pins").value_or(false);
    mod_enabled2 = cfg.get<bool>("disable_pin_timer").value_or(false);
    no_pin_timer(mod_enabled2);
}

void RoseRemovesPins::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rose_removes_pins", mod_enabled);
    cfg.set<bool>("disable_pin_timer", mod_enabled2);
}
