#pragma once

#include "../../mod.hpp"
#include "../../sdk/Pad.hpp"
#include "../../sdk/Cam.hpp"

// Coop: split-screen co-op. Extra players are spawned as their own uPlayerNero / uPlayerDante on move line 13 with their
// own uCameraCtrl (line 23, after the game's) and HUD (line 25), and the sCamera viewports are split between the cameras.
//
// Each player carries a control number in cPeripheral padding (uPlayer +0x1405, peripheral +0x95): 0 is player 1, who
// reads the game's own pad unless he picks one; 1-3 read the XInput pad picked for them (CoopPads). Everything a player
// owns updates with him as the game's player (CoopContext), found through this registry.
//
// Layout of the work (Vibed\Coop\README.md has the plan): CoopPads = input, keyboard owner, rumble; CoopContext = each
// player's context (mpPlayer / mpCamera / pad / lock-on values); CoopDiag = serial move lines and measurements.

namespace coop {

constexpr int kMaxPlayers = 4;

struct Player {
    int character = 0;          // 0 Nero, 1 Dante
    int xinput    = 0;          // XInput user index; player 1: -1 = the game's own pad (default)
    uintptr_t unit = 0;         // uPlayer
    uCameraCtrl* cam = nullptr;
    void* hud = nullptr;        // his cockpit (uCockpitNero / uCockpitDante)
    void* reticle = nullptr;    // his lock-on reticle (uLockOnCock), players 2-4
};

extern Player players[kMaxPlayers];

} // namespace coop

namespace coop_tint {
extern bool enabled;           // each player's models get his colour (Coop.cpp, tint_after_update)
extern float colors[4][4];     // RGB multipliers per player (the 4th float unused: the game reads a MtVector3)
} // namespace coop_tint

namespace coop {

// uPlayer +0x1405: the control number (cPeripheral padding), 0 = player 1.
constexpr uintptr_t kControlNumber = 0x1405;

} // namespace coop

class Coop : public Mod {
public:
    static bool mod_enabled;
    static unsigned int player_num;
    static float revive_delay;   // seconds a dead player 2-4 stays down
    static float revive_hp;      // his HP share after getting up (0.1-1)

    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_ret_water;
    static uintptr_t jmp_ret_water_init;
    static uintptr_t jmp_ret_unit_tick;
    static uintptr_t jmp_ret_unit_pass2;
    static uintptr_t jmp_ret_unit_pass3;
    static uintptr_t jmp_ret_unit_init;

    void toggle(bool enable);

    Coop() = default;
    std::string get_mod_name() override { return "Coop"; };
    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_stage_start() override;
    void on_stage_end() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook2, hook5;
    std::unique_ptr<FunctionHook> hook_kb_merge, hook_kb_exceed, hook_vib, hook_vib_stop, hook_fog1, hook_fog2,
        hook_kb_cam_x, hook_kb_cam_y, hook_kb_cam_y2;
    std::unique_ptr<FunctionHook> hook_water, hook_water_init, hook_unit_init, hook_unit_tick, hook_unit_pass2, hook_unit_pass3, hook_grab_hit, hook_damage_pre, hook_set_game_system,
        hook_lock_on, hook_update_ptr, hook_common_save, hook_dante_save, hook_nero_save, hook_disp_sphere, hook_em_step;
    std::unique_ptr<Patch> patch1;
};
