#include "RevFlying.hpp"

bool RevFlying::mod_enabled{false};

std::optional<std::string> RevFlying::on_initialize() {
    return Mod::on_initialize();
}

void RevFlying::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3FDFA0, patch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(0x4004BB, patch2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
        install_patch_offset(0x3F9C42, patch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    } else {
        patch1.reset();
    }
}

void RevFlying::on_gui_frame() {
    if (ImGui::Checkbox("Rev Flying", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Kills inertia brakes on Calibur and Rev");
}

void RevFlying::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("rev_flying").value_or(false);
    toggle(mod_enabled);
}

void RevFlying::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("rev_flying", mod_enabled);
}
