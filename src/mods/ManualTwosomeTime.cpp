#include "ManualTwosomeTime.hpp"

bool ManualTwosomeTime::mod_enabled = false;

std::optional<std::string> ManualTwosomeTime::on_initialize() {
    return Mod::on_initialize();
}

void ManualTwosomeTime::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3B8B1F, patch, "\xE9\x48\x02\x00\x00\x90", 6);
    }
    else {
        patch.reset();
    }
}

void ManualTwosomeTime::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Manual Twosome Time"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Disable Twosome Time's automatic lock-on"));
}

void ManualTwosomeTime::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("manual_twosome_time").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
}

void ManualTwosomeTime::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("manual_twosome_time", mod_enabled);
}
