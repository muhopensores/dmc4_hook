#include "GroundedGunCancels.hpp"
#include "MoveTable.hpp"

bool GroundedGunCancels::mod_enabled = false;
uintptr_t GroundedGunCancels::jmp_ret1 = NULL;

void GroundedGunCancels::toggle1(bool enable) {
    kAtckDefTbl* eiShotEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 61;
    kAtckDefTbl* coyoteShotEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 77;
    kAtckDefTbl* backslideEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 85;
    kAtckDefTbl* gunStingerEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 86;
    kAtckDefTbl* fireworksEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 87;
    kAtckDefTbl* pandoraShotEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves + 89;
    if (enable) {
        eiShotEntry->cancelId[0] = 0x00000001; // cancel
        coyoteShotEntry->cancelId[0] = 0x00000001; // cancel
        backslideEntry->cancelId[0] = 0x00000001; // cancel
        gunStingerEntry->cancelId[0] = 0x00000001; // cancel
        fireworksEntry->cancelId[0] = 0x00000001; // cancel
        pandoraShotEntry->cancelId[0] = 0x00000001; // cancel
    }
    else {
        eiShotEntry->cancelId[0] = 0x06000001; // reset
        coyoteShotEntry->cancelId[0] = 0x06000001; // reset
        backslideEntry->cancelId[0] = 0x06000001; // reset
        gunStingerEntry->cancelId[0] = 0x06000001; // reset
        fireworksEntry->cancelId[0] = 0x06000001; // reset
        pandoraShotEntry->cancelId[0] = 0x06000001; // reset
    }
}

// void on_frame(fmilliseconds& dt) {}

void GroundedGunCancels::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Grounded Gun Cancels"), &mod_enabled)) {
        toggle1(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Grounded gun attacks can cancel more moves\n"
        "Currently affects E&I basic shot, Coyote basic shot, Backslide, Gun Stinger, Fireworks, Epidemic"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> GroundedGunCancels::on_initialize() {
    return Mod::on_initialize();
}

void GroundedGunCancels::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("GroundedGunCancels").value_or(false);
    if (mod_enabled) toggle1(mod_enabled);
}

void GroundedGunCancels::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("GroundedGunCancels", mod_enabled);
}
