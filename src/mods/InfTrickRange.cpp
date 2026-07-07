#include "InfTrickRange.hpp"

bool InfTrickRange::mod_enabled = false;
bool InfTrickRange::mod_enabled_height = false;

std::optional<std::string> InfTrickRange::on_initialize() {
	return Mod::on_initialize();
}

void InfTrickRange::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3CB0A8, patch, "\x90\x90\x90", 3); // horizontal
    }
    else {
        patch.reset();
    }
}

void InfTrickRange::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x3CB0D3, patch2, "\x90\x90\x90", 3); // vertical
    } else {
        patch2.reset();
    }
}

void InfTrickRange::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Infinite Trick Range"), &mod_enabled)) {
            toggle(mod_enabled);
        }
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            if (ImGui::Checkbox(_("Infinite Trick Height"), &mod_enabled_height)) {
                toggle2(mod_enabled_height);
            }
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
}

void InfTrickRange::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("infinite_trick_range").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
    mod_enabled_height = cfg.get<bool>("infinite_trick_height").value_or(false);
    if (mod_enabled_height) toggle2(mod_enabled_height);
}

void InfTrickRange::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_trick_range", mod_enabled);
    cfg.set<bool>("infinite_trick_height", mod_enabled_height);
}
