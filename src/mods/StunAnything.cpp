#include "StunAnything.hpp"

bool StunAnything::mod_enabled = false;

std::optional<std::string> StunAnything::on_initialize() {

    return Mod::on_initialize();
}

void StunAnything::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x0011C3B2, patch, "\x83\xf9\x01\x90\x90", 5);
    }
    else {
        patch.reset();
    }
}

void StunAnything::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Stun Anything"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Allows anything (even Lucifer pins) to stun"));
}

void StunAnything::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("stun_anything").value_or(false);
    toggle(mod_enabled);
}

void StunAnything::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("stun_anything", mod_enabled);
}
