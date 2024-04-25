#include "TurnSpeedEdits.hpp"

bool TurnSpeedEdits::backslide_enable = false;

float TurnSpeedEdits::turnSpeed = 0.0f;

std::optional<std::string> TurnSpeedEdits::on_initialize() {
    return Mod::on_initialize();
}

naked void TurnSpeedEdits_patch(void) {
    _asm {
		fld dword ptr [TurnSpeedEdits::turnSpeed]
    }
}

void TurnSpeedEdits::backslide_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3DAE1E, backslide_patch, (char*)&TurnSpeedEdits_patch, 6);
    } else {
        backslide_patch.reset();
    }
}


void TurnSpeedEdits::on_gui_frame() {
    if (ImGui::Checkbox(_("Decreased Backslide Turn Speed"), &backslide_enable)) {
        backslide_toggle(backslide_enable);
    }
    ImGui::SameLine();
    help_marker(_("Reduces turn speed on backslide to make it easier to land without the need of a buffer"));
}

void TurnSpeedEdits::on_config_load(const utility::Config& cfg) {
    backslide_enable = cfg.get<bool>("dmc5_backslide").value_or(false);
    backslide_toggle(backslide_enable);
}

void TurnSpeedEdits::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dmc5_backslide", backslide_enable);
}
