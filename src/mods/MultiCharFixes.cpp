#include "MultiCharFixes.hpp"

bool MultiCharFixes::targeting_fix;

void MultiCharFixes::toggle_targeting_fix(bool enable) {
    if (enable) {
        install_patch_offset(0x420880, patch1, "\x90\x90\x90", 3);
        install_patch_offset(0x420891, patch2, "\x90\x90\x90", 3);
    } else {
        patch1.reset();
        patch2.reset();
    }
}

void MultiCharFixes::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Targeting Fix"), &targeting_fix))
        toggle_targeting_fix(targeting_fix);
}

void MultiCharFixes::on_config_load(const utility::Config& cfg) {
    targeting_fix = cfg.get<bool>("targeting_fix").value_or(false);
    toggle_targeting_fix(targeting_fix);
}

void MultiCharFixes::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("targeting_fix", targeting_fix);
}
