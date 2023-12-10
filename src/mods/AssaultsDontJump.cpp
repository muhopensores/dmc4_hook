#include "AssaultsDontJump.hpp"

#if 1
bool AssaultsDontJump::mod_enabled{false};

std::optional<std::string> AssaultsDontJump::on_initialize() {
    return Mod::on_initialize();
}

// Found accessing DT value at enemy+148C. Difficulty or something at assault+1F70.
void AssaultsDontJump::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x1CC552, patch1, "\xEB\x3C", 2);
    } else {
        patch1.reset();
    }
}

void AssaultsDontJump::on_gui_frame() {

    if (ImGui::Checkbox("No Assault Escape", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Disable Assaults jumping out of combos");
}

void AssaultsDontJump::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_assault_jumpout").value_or(false);
    toggle(mod_enabled);
}

void AssaultsDontJump::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_assault_jumpout", mod_enabled);
}

#endif
