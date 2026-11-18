#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun or Gerb

// DanteAirDodge: a sideways air dodge for Dante on lock-on + stick left / right + jump, built on Gerb's pattern: one
// MoveTable row (id 0x6E, air only) whose cycle is the whole move (launch at a frame of it, Gerb's ballistic
// physics), motions from motion\pl006\airdodge.lmt (slot 0 right, 1 left) in a free bank. At the cycle's end the
// dodge falls on with a sideways speed that drains away, as Sky Star's end does (uPlayerDante::moveActTsAirDash
// 0x7CA750 phases 3 / 4).
//
// The input isn't one the attack table can express (the jump button is no command button, and the game's lever
// check knows only forward / back / any), so the move has its own command through sdk/InputCommand: lock-on held
// + jump pressed, with a predicate that the stick leans left or right of the facing. The air hike's check runs
// before the attack check and would take the same press, so uPlayerDante::checkWJumpBegin (0x7B8E30) is hooked to
// return false under exactly that condition; any other lock-on jump press stays an air hike.
//
// The logic is `namespace dante_air_dodge`; the DanteAirDodge Mod class is the framework plumbing.

namespace dante_air_dodge {

constexpr uint32_t move_id = 0x6E;     // Dante's vanilla ids end at 0x6B; LuciAirThrow took 0x6D
extern int cmd_bit;                    // its command bit in mCmdFlag, taken with input_commands::next_free_bit (-1 none)

enum Dir : int { RIGHT = 0, LEFT = 1, DIR_COUNT = 2 };   // = the LMT slots

extern bool mod_enabled;
extern uintptr_t jmp_ret_wjump;        // 0x7B8E36, after the 6 prologue bytes the air-hike hook replaces
extern int  lmt_set;                   // the motion_registry handle of the LMT (-1 none): the registry picks the bank

bool stick_sideways(uintptr_t player);        // the stick leans left or right of the facing
bool can_dodge(uintptr_t player);             // the command's predicate: stick sideways and a dodge left this airtime
bool ready(uintptr_t player);                 // the LMT is on this player's models (the registry builds them if needed)
void register_lmt();                          // (re-)registers the LMT with the motion registry
void __stdcall move(uintptr_t player);        // the per-tick state function (MoveTable's Dante dispatch)
void apply_row();                             // writes the input and the enable byte into the row MoveTable inserted

} // namespace dante_air_dodge

class DanteAirDodge : public Mod {
public:
    DanteAirDodge() = default;
    std::string get_mod_name() override { return "DanteAirDodge"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;   // the setting in Dante's tab, the readout in the Debug tab
    void on_frame(fmilliseconds& dt) override;

    std::unique_ptr<FunctionHook> hook_wjump;

private:
    void debug_panel();
};
