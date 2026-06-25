#include "BackForwardQuickDrive.hpp"
#include "MoveTable.hpp"
#include "sdk/uPlayer.hpp"

// I would love to just edit the existing qd entry, but stinger will take priority when forward is still pressed unless this is found first.
// We disable the current quickdrive and enable the new one that is made on boot in MoveTable.cpp

bool BackForwardQuickDrive::mod_enabled = false;

void BackForwardQuickDrive::toggle1(bool enable) {
    kAtckDefTbl* quickDriveEntry      = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 4;
    kAtckDefTbl* backforwardQuickDriveEntry   = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves - MoveTable::backForwardQuickDrive;
    if (enable) {
        quickDriveEntry->command.atckCommandNo = 1; // disable
        backforwardQuickDriveEntry->command.atckCommandNo = 0; // enable
    } else {
        quickDriveEntry->command.atckCommandNo = 0; // enable
        backforwardQuickDriveEntry->command.atckCommandNo = 1; // disable
    }
}

// void BackForwardQuickDrive::on_frame(fmilliseconds& dt) {}

void BackForwardQuickDrive::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        if (ImGui::Checkbox(_("Backforward Quickdrive"), &mod_enabled)) {
            toggle1(mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Replace melee+style input with backfowrward+melee"));
    }
}

// void BackForwardQuickDrive::on_game_pause(bool toggle) {}
// bool BackForwardQuickDrive::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> BackForwardQuickDrive::on_initialize() {
    return Mod::on_initialize();
}

void BackForwardQuickDrive::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("BackForwardQuickDrive").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void BackForwardQuickDrive::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("BackForwardQuickDrive", mod_enabled);
}
