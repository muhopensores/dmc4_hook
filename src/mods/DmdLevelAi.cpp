
#include "DmdLevelAi.hpp"

#if 1
bool DmdLevelAi::mod_enabled{ false };
bool DmdLevelAi::frenzied_enabled{false};

std::optional<std::string> DmdLevelAi::on_initialize() {
    return Mod::on_initialize();
}

void DmdLevelAi::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x05D598, patch, "\x74", 1);
    }
    else {
        patch.reset();
    }
}

void DmdLevelAi::frenzied_toggle(bool enable) {
    if (enable) {
        install_patch_absolute(0x634805, patch2, "\x74\x76", 2);
        install_patch_absolute(0x64d6fd, patch3, "\x74\x72", 2);
        install_patch_absolute(0x6544a8, patch4, "\x74\x72", 2);
        install_patch_absolute(0x688c9a, patch5, "\x74\x72", 2);
        install_patch_absolute(0x6aba60, patch6, "\x74\x72", 2);
        install_patch_absolute(0x6c03dc, patch7, "\x14\x1C", 2);
        install_patch_absolute(0x6fadad, patch8, "\x74\x76", 2);
        install_patch_absolute(0x704efd, patch9, "\x74\x76", 2);
    } else {
        patch2.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
        patch8.reset();
        patch9.reset();
    }
}

void DmdLevelAi::on_gui_frame() {
    if (ImGui::Checkbox(_("DMD Level AI"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Forces enemies to use the ingame DMD AI, even on non DMD difficulties"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Frenzied Boss AI"), &frenzied_enabled)) {
        frenzied_toggle(frenzied_enabled);
    }
}

void DmdLevelAi::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dmd_level_ai").value_or(false);
    toggle(mod_enabled);
    frenzied_enabled = cfg.get<bool>("frenzied_boss").value_or(false);
    frenzied_toggle(frenzied_enabled);
}

void DmdLevelAi::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmd_level_ai", mod_enabled);
    cfg.set<bool>("frenzied_boss", frenzied_enabled);
}

#endif
