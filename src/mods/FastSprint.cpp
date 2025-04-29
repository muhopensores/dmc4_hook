#include "FastSprint.hpp"

#if 1
uintptr_t FastSprint::jmp_ret = NULL;
bool FastSprint::mod_enabled_nero = false;
bool FastSprint::mod_enabled_dante = false;
float FastSprint::newSprintTimer = 50.0f; // 225.0f default

naked void detour(void) { // player in ebx
    _asm {
            cmp [ebx+0x1494], 1 // controller id nero
            je CheckNero
            cmp byte ptr [FastSprint::mod_enabled_dante], 1
			je NewCode
            jmp originalcode

        CheckNero:
			cmp byte ptr [FastSprint::mod_enabled_nero], 1
			jne originalcode
        NewCode:
			movss xmm1, [FastSprint::newSprintTimer]
			jmp dword ptr [FastSprint::jmp_ret]
		
		originalcode:
			movss xmm1, [edx+0x000000C8]
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

void FastSprint::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Fast Sprint"), &mod_enabled_nero);
        ImGui::SameLine();
        help_marker(_("Go from Run to Speed faster, even if you're in battle"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Fast Sprint"), &mod_enabled_dante);
        ImGui::SameLine();
        help_marker(_("Go from Run to Speed faster, even if you're in battle"));
    }
}

void FastSprint::on_config_load(const utility::Config& cfg) {
    mod_enabled_nero = cfg.get<bool>("fast_sprint_nero").value_or(false);
    mod_enabled_dante = cfg.get<bool>("fast_sprint_dante").value_or(false);
}

void FastSprint::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("fast_sprint_nero", mod_enabled_nero);
    cfg.set<bool>("fast_sprint_dante", mod_enabled_dante);
}

#endif
