#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun or Gerb

// DanteAirDodge: a sideways air dodge for Dante on lock-on + stick left / right + jump, built on Gerb's pattern: one
// MoveTable row (id 0x6E, air only) whose cycle is the whole move (launch at a frame of it, Gerb's ballistic
// physics, no fall motion), motions from motion\pl006\airdodge.lmt (slot 0 right, 1 left) in a free bank.
//
// The input isn't one the attack table can express (the jump button is no command button, and the game's lever
// check knows only forward / back / any), so the move has its own command through sdk/InputCommand: lock-on held
// + jump pressed, with a predicate that the stick leans left or right of the facing (within `side_angle`). The air
// hike's check runs before the attack check and would take the same press, so uPlayerDante::checkWJumpBegin
// (0x7B8E30) is hooked to return false under exactly that condition; any other lock-on jump press stays an air hike.
//
// The logic is `namespace dante_air_dodge`; the DanteAirDodge Mod class is the framework plumbing.

namespace dante_air_dodge {

constexpr uint32_t move_id = 0x6E;     // Dante's vanilla ids end at 0x6B; LuciAirThrow took 0x6D
constexpr int cmd_bit = 86;            // its command bit in mCmdFlag (Gerb's back-forward-style is 85)

enum Dir : int { RIGHT = 0, LEFT = 1, DIR_COUNT = 2 };   // = the LMT slots

extern bool mod_enabled;
extern uintptr_t jmp_ret_wjump;        // 0x7B8E36, after the 6 prologue bytes the air-hike hook replaces

// the LMT
extern int  lmt_set;                   // the motion_registry handle of lmt_path (-1 none): the registry picks the bank
extern char lmt_path[128];             // game path without extension
extern int  slot[DIR_COUNT];           // block per direction, 0 right, 1 left
extern int  ctn_bank, ctn_slot;        // vanilla .ctn script borrowed for the cycles (runtime banks have none)

// the dodge (cm per frame)
extern float launch_frame;             // frame of the cycle at which the velocity is applied
extern float speed;                    // sideways
extern float rise;                     // vertical speed added at the launch
extern float gravity;                  // added to the vertical speed every frame (negative)
extern float max_fall;
extern float cancel_frame;             // from this frame of the cycle every cancel window is open (-1 = never)
extern float side_angle;               // degrees off the right / left axis within which the stick counts as sideways
extern bool  face_direction;           // turn the facing toward the dodge (off: a dodge keeps facing the target)
extern bool  airborne_status;          // mark the action as a jump (AST_JUMP) so the game's air rules apply
extern bool  enemy_step_from_start;    // the enemy step (jump cancel) can interrupt the dodge from frame 0
extern bool  cost_air_hike;            // count as the air hike (mWJumpCnt++), so no air hike after it
extern int   dodges_per_air;           // dodges allowed between landings (0 = unlimited); a spent dodge leaves the press to the air hike

// the take-off: Dante's air hike effect and sound (uPlayerDante::setWJumpEffect 0x7B9220)
extern bool  effect_enabled;
extern int   effect_list, effect_index;   // sDevil4Effect list / index: Dante's air hike is 2 / 0x14 (effect\efl\com\ec005_20v1)
extern float effect_tilt;                 // degrees the effect leans from the player's up toward the dodge
extern bool  se_enabled;
extern int   se_id;                       // SE of the player's sound resource: the air hike plays 40

bool stick_sideways(uintptr_t player);        // the stick leans left or right of the facing
bool can_dodge(uintptr_t player);             // the command's predicate: stick sideways and a dodge left this airtime
bool ready(uintptr_t player);                 // the LMT is on this player's models (the registry builds them if needed)
void register_lmt();                          // (re-)registers lmt_path with the motion registry
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
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

    std::unique_ptr<FunctionHook> hook_wjump;
};
