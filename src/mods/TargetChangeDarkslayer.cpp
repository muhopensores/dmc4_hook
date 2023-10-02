#include "TargetChangeDarkslayer.hpp"

#if 1
bool TargetChangeDarkslayer::mod_enabled{false};
uintptr_t TargetChangeDarkslayer::jmp_ret{NULL};
uintptr_t TargetChangeDarkslayer::jmp_conditional{0x007B6CF1};

naked void detour(void) {
    _asm {
			cmp byte ptr [TargetChangeDarkslayer::mod_enabled], 0
			je originalcode

			test dword ptr [esi+0x00001410],0x00010000
            jne jmp_conditional

		originalcode:
            test dword ptr [esi+0x00001410],0x00002000
			jmp dword ptr [TargetChangeDarkslayer::jmp_ret]

        jmp_conditional:
            jmp dword ptr [TargetChangeDarkslayer::jmp_conditional]
    }
}

void TargetChangeDarkslayer::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x4208BB, patch, "\xE9\x81\x00\x00\x00\x90", 6);
    } else {
        patch.reset();
    }
}

std::optional<std::string> TargetChangeDarkslayer::on_initialize() {
    if (!install_hook_offset(0x003B6CC6, hook, &detour, &jmp_ret, 10)) {
        spdlog::error("Failed to init TargetChangeDarkslayer mod\n");
        return "Failed to init TargetChangeDarkslayer mod";
    }
    return Mod::on_initialize();
}

void TargetChangeDarkslayer::on_gui_frame() {
    if (ImGui::Checkbox("Target Change Darkslayer", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Darkslayer is accessed by pressing the target change button");
}

void TargetChangeDarkslayer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("target_change_darkslayer").value_or(false);
    toggle(mod_enabled);
}

void TargetChangeDarkslayer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("target_change_darkslayer", mod_enabled);
}
#endif
