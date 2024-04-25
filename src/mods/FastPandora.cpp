#include "FastPandora.hpp"

bool FastPandora::mod_enabled{ false };

std::optional<std::string> FastPandora::on_initialize() {
    return Mod::on_initialize();
}

void FastPandora::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03BCE7E, patch, "\x66\x83\xBF\x72\x17\x00\x00\x01", 8);
    }
    else {
        patch.reset();
    }
}

void FastPandora::on_gui_frame() {
    if (ImGui::Checkbox(_("Fast Pandora"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Cycle to grounded Pandora moves in DT speed outside of DT"));
}

void FastPandora::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("fast_pandora").value_or(false);
    toggle(mod_enabled);
}

void FastPandora::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("fast_pandora", mod_enabled);
}
