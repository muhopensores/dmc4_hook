#include "SlowWalk.hpp"

bool SlowWalk::mod_enabled{ false };

std::optional<std::string> SlowWalk::on_initialize() {
    return Mod::on_initialize();
}

void SlowWalk::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x0421C83, patch1, "\x0c\x14\x00\x00\x04\x0f\x83", 7);
        install_patch_offset(0x0421D85, patch2, "\x0c\x14\x00\x00\x39\xc8", 6);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

void SlowWalk::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Slow Walk"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Walk slowly while holding the jump button"));
}

void SlowWalk::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("slow_walk").value_or(false);
    toggle(mod_enabled);
}

void SlowWalk::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("slow_walk", mod_enabled);
}
