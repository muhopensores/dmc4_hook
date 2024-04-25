#include "FastSprint.hpp"

#if 1
uintptr_t FastSprint::jmp_ret{NULL};
bool FastSprint::mod_enabled{ false };
float FastSprint::newSprintTimer{50.0f}; // 225

naked void detour(void) {
    _asm {
			cmp byte ptr [FastSprint::mod_enabled], 0
			je originalcode

			movss xmm1, [FastSprint::newSprintTimer]
			jmp dword ptr [FastSprint::jmp_ret]
		
		originalcode:
			movss xmm1,[edx+0x000000C8]
			jmp dword ptr [FastSprint::jmp_ret]
    }
}

std::optional<std::string> FastSprint::on_initialize() {
    if (!install_hook_offset(0x040454C, hook, &detour, &jmp_ret, 8)) {
        spdlog::error("Failed to init FastSprint mod\n");
        return "Failed to init FastSprint mod";
    }
    return Mod::on_initialize();
}

void FastSprint::on_gui_frame() {
    ImGui::Checkbox(_("Fast Sprint"), &mod_enabled);
}

void FastSprint::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("fast_sprint").value_or(false);
}

void FastSprint::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_sprint", mod_enabled);
}

#endif
