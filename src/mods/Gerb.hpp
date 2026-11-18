#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets / JUMP_MODE; the move itself shares no state with WallRun

// Gerb: a free three-dimensional jump, no wall. The jump-out of WallRun taken on its own: the cycle is the whole
// jump (launch at a frame of it, ballistic physics with fixed values in Gerb.cpp, no fall motion). Two MoveTable rows:
//   0x40 (move_id)          forward / back / left / right by where the stick leans relative to the facing; air only
//   0x41 (move_id_vertical) straight up from the ground, straight down in the air, on its own command
// Nero's dispatch (MoveTable nero_move_switch) calls gerb::move(actor, id) every tick while moveID2 is one of them.
// The phase lives in the game's mBdyAction[1].phase, which setAction zeroes on entry: 0 entry, 1 waiting for the
// launch frame, 2 airborne.
//
// Inputs: each row has its own (`custom_input[ROW_DIRECTIONAL / ROW_VERTICAL]`): a trigger (game buttons, a gamepad
// button combination or a keyboard key) with lock-on held and a stick direction as optional conditions. Gerb's
// pre-check tick (the shared hook after executeCommandCheck, which Gerb installs) checks them and sets the row's command
// bit (`cmd_row`, 85 / 86 of Nero's mCmdFlag: reserved with input_commands::add_for so MoveDef doesn't take them), which the row
// (a COMMAND row: checkAtckSubCommand tests the bit) triggers on. Defaults: lock-on + any direction + style (directional),
// lock-on + the stick centred + style (vertical). The grab button is free for these because Gerb's rows run in the
// Gerbera style only (NeroStyle), where the game's Snatch rows are off; Calibur's back, forward (STICK_BACK_FORWARD, the
// game's cmd 0x52) stays a choice in the GUI.
//
// Motions come from motion\pl000\gerb.lmt (Nero) loaded into a free bank at runtime (MoveDef D14): blocks
// 0 forward, 1 down, 2 back, 3 right, 4 left, 5 up. Without the file the move refuses to start (there is no
// vanilla stand-in for it).
//
// The jump's values are constants in Gerb.cpp: launch at frame 0 of the cycle, 20 cm / frame along the direction plus a
// 14 rise (forward / back / left / right), up 32, down 24, gravity -1.1 a frame, max fall 30, every cancel window open
// from frame 10, sideways beyond 50 deg off the facing, the facing turned toward forward / left / right jumps, AST_JUMP
// from the take-off, and the air hike's effect (list 1 / 0x28, leaned 45 deg toward the jump) and SE 40 at it. The
// cycles borrow the fall motion's .ctn script (bank 0, slot 5). Only the on / off and the rows' inputs are settings.
//
// The move is `namespace gerb` (the state function, the rows, the command); the `Gerb` Mod class below is only the
// framework plumbing (hook install, config, GUI) and holds no state of its own but the hook.

namespace gerb {

constexpr uint32_t move_id = 0x40;            // atckId of the directional row. Nero's vanilla ids: 1-0x2B lower-body
                                              // (moveActAtckL), 0x2C-0x33 upper-body / gun (moveActAtckU 0x7F9E90); 0x34+ are free
constexpr uint32_t move_id_vertical = 0x41;   // the up / down row
enum Row : int { ROW_DIRECTIONAL = 0, ROW_VERTICAL = 1 };
constexpr int cmd_row[2] = { 85, 86 };        // the rows' command bits (85 was the fixed back-forward + style command)

// A row's input: the trigger, pressed this tick, and the optional conditions.
enum InputSource : int { SRC_GAME = 0 /* game buttons */, SRC_PAD = 1 /* gamepad buttons */, SRC_KEY = 2 /* a keyboard key */ };
enum StickReq : int { STICK_NONE = -1, STICK_CENTRED = 0, STICK_FORWARD, STICK_BACK, STICK_LEFT, STICK_RIGHT, STICK_ANY,
                      STICK_BACK_FORWARD /* Calibur's input (game command 0x52): centred, back, forward within 15 frames,
                                            the trigger within 15 more */ };
struct CustomInput {
    int      source;     // InputSource
    uint32_t bits;       // SRC_GAME: mBtnOn bits (1 melee, 2 gun, 4 jump, 0x10 lock-on, 0x200 style / Buster), all held and one
                         // pressed this tick; SRC_PAD: mPeripheral.mPadBtnOn raw bits (captured), all held; SRC_KEY: a virtual-key
                         // code (the game in front, the dmc4_hook menu closed). 0 = unset, never fires
    bool     lockon;     // lock-on must be held (TPS aim's virtual lock-on counts)
    int      stick;      // StickReq: the stick against the lock-on target's direction (else the facing): forward / back within
                         // 45 deg, left / right between
};
extern CustomInput custom_input[2];   // [Row]
extern bool vertical_ground_any_stick;   // on the ground the vertical row takes any stick (the directional row is air only)

enum Dir : int { FORWARD = 0, DOWN = 1, BACK = 2, LEFT = 3, RIGHT = 4, UP = 5, DIR_COUNT = 6 };   // = the LMT slots

extern bool mod_enabled;
extern int  lmt_set;              // the motion_registry handle of the LMT (-1 none): the registry picks the bank

Dir  pick_direction(uintptr_t player, bool vertical);
bool ready(uintptr_t player);          // the LMT is on this player's models (the registry builds them if needed)
void register_lmt();                   // (re-)registers the LMT with the motion registry
void __stdcall move(uintptr_t player, uint32_t atck_id);   // the per-tick state function (atck_id = the row)
void apply_row();                      // writes the inputs and the enable byte into the rows MoveTable inserted

} // namespace gerb

class Gerb : public Mod {
public:
    Gerb() = default;
    std::string get_mod_name() override { return "Gerb"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_cmd;
};
