#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets / JUMP_MODE; the move itself shares no state with WallRun

// Gerb: a free three-dimensional jump, no wall. The jump-out of WallRun taken on its own: the cycle is the whole
// jump (launch at a frame of it, tunable ballistic physics, no fall motion). Two MoveTable rows:
//   0x40 (move_id)          forward / back / left / right by where the stick leans relative to the facing; air only
//   0x41 (move_id_vertical) straight up from the ground, straight down in the air, on its own command
// Nero's dispatch (MoveTable nero_move_switch) calls gerb::move(actor, id) every tick while moveID2 is one of them.
// The phase lives in the game's mBdyAction[1].phase, which setAction zeroes on entry: 0 entry, 1 waiting for the
// launch frame, 2 airborne.
//
// Inputs: each row has its own (`custom_input[ROW_DIRECTIONAL / ROW_VERTICAL]`): a trigger (game buttons, a gamepad
// button combination or a keyboard key) with lock-on held and a stick direction as optional conditions. Gerb's
// pre-check tick (the shared hook after executeCommandCheck, which Gerb installs) checks them and sets the row's command
// bit (`cmd_row`, 85 / 86 of mCmdFlag: reserved with input_commands::add so MoveDef doesn't take them), which the row
// (a COMMAND row: checkAtckSubCommand tests the bit) triggers on. Defaults: lock-on + any direction + style (directional),
// lock-on + back, forward + style (vertical: the stick back, then forward within 15 frames each, as the game's cmd 0x52).
//
// Motions come from motion\pl000\gerb.lmt (Nero) loaded into a free bank at runtime (MoveDef D14): slots
// 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up. Without the file the move refuses to start (there is no
// vanilla stand-in for it).
//
// The move is `namespace gerb` (tunables, the state function, the rows, the command); the `Gerb` Mod class below
// is only the framework plumbing (hook install, config, GUI) and holds no state of its own but the hook.

namespace gerb {

constexpr uint32_t move_id = 0x40;            // atckId of the directional row. Nero's vanilla ids: 1-0x2B lower-body
                                              // (moveActAtckL), 0x2C-0x33 upper-body / gun (moveActAtckU 0x7F9E90); 0x34+ are free
constexpr uint32_t move_id_vertical = 0x41;   // the up / down row
enum Row : int { ROW_DIRECTIONAL = 0, ROW_VERTICAL = 1 };
constexpr int cmd_row[2] = { 85, 86 };        // the rows' command bits (85 was the fixed back-forward + style command)

// A row's input: the trigger, pressed this tick, and the optional conditions.
enum InputSource : int { SRC_GAME = 0 /* game buttons */, SRC_PAD = 1 /* gamepad buttons */, SRC_KEY = 2 /* a keyboard key */ };
enum StickReq : int { STICK_NONE = -1, STICK_CENTRED = 0, STICK_FORWARD, STICK_BACK, STICK_LEFT, STICK_RIGHT, STICK_ANY,
                      STICK_BACK_FORWARD /* back, then forward within 15 frames, the trigger within 15 more */ };
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

enum Dir : int { FORWARD = 0, DOWN = 1, BACK = 2, LEFT = 3, RIGHT = 4, UP = 5, DIR_COUNT = 6 };   // = the LMT slots

extern bool mod_enabled;

// the LMT
extern int  lmt_bank;             // 10-14 are free on both characters (WallRun uses 14)
extern char lmt_path[128];        // game path without extension
extern int  slot[DIR_COUNT];      // block per direction, 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up
extern int  ctn_bank, ctn_slot;   // vanilla .ctn script borrowed for the cycles (runtime banks have none)

// the jump (cm per frame)
extern float launch_frame;        // frame of the cycle at which the velocity is applied
extern float speed;               // along the chosen direction (forward / back / left / right)
extern float rise;                // vertical speed added at launch for forward / left / right
extern float up_speed;            // the up jump
extern float down_speed;          // the down jump (dive)
extern float gravity;             // added to the vertical speed every frame (negative)
extern float max_fall;
extern float cancel_frame;        // from this frame of the cycle every cancel window is open (-1 = never)
extern float side_angle;          // degrees off the facing (or its opposite) beyond which a stick is left / right, not forward / back
extern bool  face_direction;      // turn the facing toward the jump direction (forward / left / right; a back jump keeps facing)
extern bool  airborne_status;     // mark the action as a jump (AST_JUMP) so the game's air rules apply

// the take-off: the air hike's effect and sound (uPlayerNero::setWJumpEffect 0x7E8850), fired at the launch frame
extern bool  effect_enabled;
extern int   effect_list, effect_index;   // sDevil4Effect list / index: Nero's air hike is 1 / 0x28 (effect\efl\com\ec000_00v2),
                                          // Dante's 2 / 0x14 (ec005_20v1)
extern float effect_tilt;                 // degrees the effect leans from the player's up toward the jump direction (level part)
extern bool  se_enabled;
extern int   se_id;                       // SE of the player's sound resource (+0x1D60): the air hike plays 40

Dir  pick_direction(uintptr_t player, bool vertical);
bool ready(uintptr_t player);          // the LMT is in this player's bank
void ensure_bank(uintptr_t player);    // loads it when the slot is empty (per player object)
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
    void on_frame(fmilliseconds& dt) override;

    std::unique_ptr<FunctionHook> hook_cmd;
};
