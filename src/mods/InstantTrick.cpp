#include "InstantTrick.hpp"

#if 1
bool InstantTrick::mod_enabled = false;

std::optional<std::string> InstantTrick::on_initialize() {
    return Mod::on_initialize();
}

void InstantTrick::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03CB005, patch, "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90", 12);
    }
    else {
        patch.reset();
    }
}

void InstantTrick::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Fast Trick"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Skips one part of the trick animation"));
}

void InstantTrick::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("instant_trick").value_or(false);
    toggle(mod_enabled);
}

void InstantTrick::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("instant_trick", mod_enabled);
}

#endif
