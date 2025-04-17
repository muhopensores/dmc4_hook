
#include "DtEnemiesDontStun.hpp"

#if 1
bool DtEnemiesDontStun::mod_enabled{ false };

std::optional<std::string> DtEnemiesDontStun::on_initialize() {
    return Mod::on_initialize();
}

void DtEnemiesDontStun::toggle(bool enable){
    if (enable){
        install_patch_offset(0x11B88E, patch, "\x90\x90", 2);
    }
    else {
        patch.reset();
    }
}

void DtEnemiesDontStun::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("DT Enemies Don't Stun"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Stops enemies from receiving stun from most attacks"));
}

void DtEnemiesDontStun::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dt_enemies_dont_stun").value_or(false);
    toggle(mod_enabled);
}

void DtEnemiesDontStun::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dt_enemies_dont_stun", mod_enabled);
}

#endif
