#include "InfiniteTime.hpp"

#if 1
bool InfiniteTime::mod_enabled = false;

std::optional<std::string> InfiniteTime::on_initialize() {
    return Mod::on_initialize();
}

void InfiniteTime::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x94D6E, patch_time, "\x77\x54", 2);
    }
    else {
        patch_time.reset();
    }
}

void InfiniteTime::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Disable Timer"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Stops the BP, Secret Mission and Mission 12 timers from decreasing over time"));
}

void InfiniteTime::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_time").value_or(false);
    toggle(mod_enabled);
}

void InfiniteTime::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_time", mod_enabled);
}

#endif
