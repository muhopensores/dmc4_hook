#include "BackForwardQuickDrive.hpp"
#include "../sdk/BossMoves.hpp"
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
        help_marker(_("Replace melee+style input with backforward+melee"));
    }
}

// void BackForwardQuickDrive::on_game_pause(bool toggle) {}
// bool BackForwardQuickDrive::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

// The boss Dante's back-forward Quick Drive on the ground (sdk/BossMoves.hpp): a mid-range punish. The row is lock-on, back,
// forward (15 frames each), melee; Rebellion, any style.
static void register_boss_move() {
    using namespace boss_moves;
    Move m;
    m.name = "Quick Drive (back-forward)";
    m.enabled = &BackForwardQuickDrive::mod_enabled;
    m.sword = SWORD_REBELLION;
    m.cancel = CANCEL_COMMAND;
    m.input(BTN_LOCK_ON, STICK_CENTRE, 2).input(BTN_LOCK_ON, STICK_AWAY, 3).input(BTN_LOCK_ON, STICK_TOWARD, 3)
     .input(BTN_LOCK_ON | BTN_MELEE, STICK_TOWARD, 1);
    m.atck_id = 0x19;
    m.min_run = 20.0f;
    m.where = WHERE_GROUND;
    m.min_dist = 300.0f; m.max_dist = 1300.0f;
    m.stands_in = bit(ACTION_ATK_STINGER) | bit(ACTION_ATK_LONG) | bit(ACTION_ATK_HIGHTIME);
    m.chance = 0.3f; m.cooldown = 480.0f;
    add(m);
}

std::optional<std::string> BackForwardQuickDrive::on_initialize() {
    register_boss_move();
    return Mod::on_initialize();
}

void BackForwardQuickDrive::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("BackForwardQuickDrive").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void BackForwardQuickDrive::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("BackForwardQuickDrive", mod_enabled);
}
