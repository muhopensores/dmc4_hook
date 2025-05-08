#include "InfFaustCloak.hpp"

#if 1
bool InfFaustCloak::always_cloak = false;
bool InfFaustCloak::no_cloak = false;

std::optional<std::string> InfFaustCloak::on_initialize() {
    return Mod::on_initialize();
}

void InfFaustCloak::toggle1(bool enable) {
    if (enable) {
        install_patch_offset(0x1932E5, patch1, "\x90\x90\x90\x90\x90\x90\x90\x90", 8);
    }
    else {
        patch1.reset();
    }
}

void InfFaustCloak::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x183667, patch2, "\xE9\xE6\x00\x00\x00\x90", 6);
    } else {
        patch2.reset();
    }
}

void InfFaustCloak::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Infinite Mephisto / Faust Cloak"), &always_cloak)) {
        toggle1(always_cloak);
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Disable Mephisto / Faust Recloak"), &no_cloak)) {
        toggle2(no_cloak);
    }
}

void InfFaustCloak::on_config_load(const utility::Config& cfg) {
    always_cloak = cfg.get<bool>("inf_faust_cloak").value_or(false);
    toggle1(always_cloak);
    no_cloak = cfg.get<bool>("disable_faust_cloak").value_or(false);
    toggle2(no_cloak);
}

void InfFaustCloak::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_faust_cloak", always_cloak);
    cfg.set<bool>("disable_faust_cloak", no_cloak);
}

#endif
