#include "DisableDarkslayer.hpp"

bool DisableDarkslayer::mod_enabled_up{ false };
bool DisableDarkslayer::mod_enabled_down{ false };
bool DisableDarkslayer::mod_enabled_left{ false };
bool DisableDarkslayer::mod_enabled_right{ false };

std::optional<std::string> DisableDarkslayer::on_initialize() {
    return Mod::on_initialize();
}

void DisableDarkslayer::toggle_up(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6C84, patch1, "\x90\x90\x90", 3);
    }
    else {
        patch1.reset();
    }
}
void DisableDarkslayer::toggle_down(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6CE3, patch2, "\x90\x90\x90", 3);
    }
    else {
        patch2.reset();
    }
}
void DisableDarkslayer::toggle_left(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6D42, patch3, "\x90\x90\x90", 3);
    }
    else {
        patch3.reset();
    }
}
void DisableDarkslayer::toggle_right(bool enable) {
    if (enable) {
        install_patch_offset(0x03B6D99, patch4, "\x90\x90\x90", 3);
    }
    else {
        patch4.reset();
    }
}

void DisableDarkslayer::on_gui_frame(int display) {
    ImGui::Spacing();
    ImGui::Text(_("Disable Darkslayer Inputs"));
    ImGui::Spacing();

    ImGui::Text("               ");
    ImGui::SameLine();
    if (ImGui::Checkbox(_("Trickster"), &mod_enabled_up)) {
        toggle_up(mod_enabled_up);
    }

    ImGui::Text("");
    ImGui::SameLine();
    if (ImGui::Checkbox(_("Gunslinger"), &mod_enabled_left)) {
        toggle_left(mod_enabled_left);
    }
    ImGui::SameLine();
    if (ImGui::Checkbox(_("Sword Master"), &mod_enabled_right)) {
        toggle_right(mod_enabled_right);
    }

    ImGui::Text("               ");
    ImGui::SameLine();
    if (ImGui::Checkbox(_("Royal Guard"), &mod_enabled_down)) {
        toggle_down(mod_enabled_down);
    }
}

void DisableDarkslayer::on_config_load(const utility::Config& cfg) {
    mod_enabled_up = cfg.get<bool>("disable_darkslayer_up").value_or(false);
    toggle_up(mod_enabled_up);
    mod_enabled_down = cfg.get<bool>("disable_darkslayer_down").value_or(false);
    toggle_down(mod_enabled_down);
    mod_enabled_left = cfg.get<bool>("disable_darkslayer_left").value_or(false);
    toggle_left(mod_enabled_left);
    mod_enabled_right = cfg.get<bool>("disable_darkslayer_right").value_or(false);
    toggle_right(mod_enabled_right);
}

void DisableDarkslayer::on_config_save(utility::Config& cfg)
{
    cfg.set<bool>("disable_darkslayer_up", mod_enabled_up);
    cfg.set<bool>("disable_darkslayer_down", mod_enabled_down);
    cfg.set<bool>("disable_darkslayer_left", mod_enabled_left);
    cfg.set<bool>("disable_darkslayer_right", mod_enabled_right);
}
