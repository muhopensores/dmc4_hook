#include "RevFlying.hpp"

bool RevFlying::mod_enabled{false};
bool RevFlying::mod_enabled2{false};
uintptr_t RevFlying::jmp_ret1{NULL};
uintptr_t RevFlying::jmp_ret2{NULL};

void RevFlying::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3F9C42, patch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    } else {
        patch3.reset();
    }
}

void RevFlying::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x3FDFA0, patch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(0x4004BB, patch2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    } else {
        patch1.reset();
        patch2.reset();
    }
}

naked void detour1(void) { // non exceed
    _asm {
			cmp byte ptr [RevFlying::mod_enabled2], 0
			je originalcode

            mov dword ptr [esi+0x00001E1C], 0x0
		originalcode:
            mov dword ptr [esi+0x00001504], 0x00000004
			jmp dword ptr [RevFlying::jmp_ret1]
    }
}

naked void detour2(void) { // exceed
    _asm {
			cmp byte ptr [RevFlying::mod_enabled2], 0
			je originalcode

            mov dword ptr [ebx+0x00001E1C], 0x0
		originalcode:
            mov dword ptr [ebx+0x00001504], 0x00000004
			jmp dword ptr [RevFlying::jmp_ret2]
    }
}

std::optional<std::string> RevFlying::on_initialize() {
    if (!install_hook_offset(0x3FDFF9, hook1, &detour1, &jmp_ret1, 10)) {
        spdlog::error("Failed to init RevFlying mod\n");
        return "Failed to init RevFlying mod";
    }
    if (!install_hook_offset(0x400549, hook2, &detour2, &jmp_ret2, 10)) {
        spdlog::error("Failed to init RevFlying mod2\n");
        return "Failed to init RevFlying mod2";
    }
    return Mod::on_initialize();
}

void RevFlying::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Rev Flying"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Kills inertia brakes on Rev"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Calibur Inertia"), &mod_enabled2)) {
        toggle2(mod_enabled2);
    }
    ImGui::SameLine();
    help_marker(_("Kills inertia brakes on Calibur"));
}

void RevFlying::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("rev_flying").value_or(false);
    toggle(mod_enabled);
    mod_enabled2 = cfg.get<bool>("calibur_inertia").value_or(false);
    toggle2(mod_enabled2);
}

void RevFlying::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rev_flying", mod_enabled);
    cfg.set<bool>("calibur_inertia", mod_enabled2);
}
