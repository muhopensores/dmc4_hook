
#include "DisableChimeraBlades.hpp"

bool DisableChimeraBlades::mod_enabled{ false };
uintptr_t DisableChimeraBlades::jmp_ret{ NULL };
constexpr uintptr_t jmp_jne{ 0x05F1B0F };

naked void detour(void) {
	_asm {
			cmp byte ptr [DisableChimeraBlades::mod_enabled], 0
			je originalcode

			cmp byte ptr [esi+0x00001438], 00
			jmp jnecode

		originalcode:
			cmp byte ptr [esi+0x00001438], 00
			jne jnecode
			jmp dword ptr [DisableChimeraBlades::jmp_ret]

			jnecode:
			jmp dword ptr [jmp_jne]
	}
}

std::optional<std::string> DisableChimeraBlades::on_initialize() {
	if (!install_hook_offset(0x01F1A8F, hook, &detour, &jmp_ret, 9)) {
		spdlog::error("Failed to init DisableChimeraBlades mod\n");
		return "Failed to init DisableChimeraBlades mod";
	}
	return Mod::on_initialize();
}

void DisableChimeraBlades::on_gui_frame() {
    ImGui::Checkbox("Disable Chimera Blades", &mod_enabled);
}

void DisableChimeraBlades::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_chimera_blades").value_or(false);
}

void DisableChimeraBlades::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_chimera_blades", mod_enabled);
}
