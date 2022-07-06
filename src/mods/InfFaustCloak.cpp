
#include "InfFaustCloak.hpp"

#if 1
bool InfFaustCloak::mod_enabled{ false };

std::optional<std::string> InfFaustCloak::on_initialize() {
    return Mod::on_initialize();
}

void InfFaustCloak::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x1932E5, patch, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else {
        patch.reset();
    }
}

void InfFaustCloak::on_gui_frame() {
    if (ImGui::Checkbox("Infinite Faust Cloak", &mod_enabled)) {
        toggle(mod_enabled);
    }
}

void InfFaustCloak::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_faust_cloak").value_or(false);
    toggle(mod_enabled);
}

void InfFaustCloak::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_faust_cloak", mod_enabled);
}

#endif
