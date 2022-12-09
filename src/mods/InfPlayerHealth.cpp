
#include "InfPlayerHealth.hpp"

bool InfPlayerHealth::mod_enabled{ false };
uintptr_t InfPlayerHealth::jmp_ret{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [InfPlayerHealth::mod_enabled], 0
			je originalcode

			mov dword ptr [ebp+0x000015CC], 0x469C4000

		originalcode:
            movss xmm0,[ebp+0x000015CC]
			jmp dword ptr [InfPlayerHealth::jmp_ret]
	}
}

std::optional<std::string> InfPlayerHealth::on_initialize() {
	if (!install_hook_offset(0x00FF282, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init InfPlayerHealth mod\n");
		return "Failed to init InfPlayerHealth mod";
	}

	MutatorRegistry::define("Godmode")
		.description("hehe")
		.on_init([&] { mod_enabled = true; })
		.set_timer(30.0, [&] { mod_enabled = false; });

	return Mod::on_initialize();
}

void InfPlayerHealth::on_gui_frame() {
    ImGui::Checkbox("Freeze Player HP To Max", &mod_enabled);
}

void InfPlayerHealth::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_player_health").value_or(false);
}

void InfPlayerHealth::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_player_health", mod_enabled);
}
