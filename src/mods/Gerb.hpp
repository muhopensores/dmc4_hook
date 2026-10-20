#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets / JUMP_MODE; the move itself shares no state with WallRun

// Gerb: a free three-dimensional jump, no wall. The jump-out of WallRun taken on its own: the cycle is the whole
// jump (launch at a frame of it, tunable ballistic physics, no fall motion), and the direction comes from the
// stick: forward / back / left / right by where it leans relative to the facing, centred = straight up, or
// straight down when a button of `down_mask` is held with the stick centred.
//
// A MoveTable move: call Gerb::move(actor) from the dispatch every tick while moveID2 is its row
// (dante_move_switch: `case <id + 1>: Gerb::move(actor); break;`). The phase lives in the game's
// mBdyAction[1].phase, which setAction zeroes on entry: 0 entry, 1 waiting for the launch frame, 2 airborne.
//
// Motions come from motion\pl000\gerb.lmt (Nero) loaded into a free bank at runtime (MoveDef D14): slots
// 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up. Without the file the move refuses to start (there is no
// vanilla stand-in for it).

class Gerb : public Mod {
public:
    Gerb() = default;
    std::string get_mod_name() override { return "Gerb"; };

    static bool mod_enabled;
    static const uint32_t move_id = 0x40;   // atckId of its kAtckDefTbl row. Nero's vanilla ids: 1-0x2B lower-body
                                            // (moveActAtckL), 0x2C-0x33 upper-body / gun (moveActAtckU 0x7F9E90); 0x34+ are free
    static int  input_cmd;                  // ATTACK_DIRECTION_CMD the row triggers on (its own input, not the jump button)
    static void apply_row();                // writes input_cmd and the enable byte into the row MoveTable inserted

    enum Dir : int { FORWARD = 0, DOWN = 1, BACK = 2, LEFT = 3, RIGHT = 4, UP = 5, DIR_COUNT = 6 };   // = the LMT slots

    // the LMT
    static int  lmt_bank;             // 10-14 are free on both characters (WallRun uses 14)
    static char lmt_path[128];        // game path without extension
    static int  slot[DIR_COUNT];      // block per direction, 0 forward, 1 down, 2 back, 3 left, 4 right, 5 up
    static int  ctn_bank, ctn_slot;   // vanilla .ctn script borrowed for the cycles (runtime banks have none)
    static uint32_t down_mask;        // mBtnOn bits: held with the stick centred = the down jump (0 = never)

    // the jump (cm per frame)
    static float launch_frame;        // frame of the cycle at which the velocity is applied
    static float speed;               // along the chosen direction (forward / back / left / right)
    static float rise;                // vertical speed added at launch for forward / left / right
    static float up_speed;            // the up jump
    static float down_speed;          // the down jump (dive)
    static float gravity;             // added to the vertical speed every frame (negative)
    static float max_fall;
    static float cancel_frame;        // from this frame of the cycle every cancel window is open (-1 = never)
    static float side_angle;          // degrees off the facing (or its opposite) beyond which a stick is left / right, not forward / back
    static bool  face_direction;      // turn the facing toward the jump direction (forward / left / right; a back jump keeps facing)
    static bool  airborne_status;     // mark the action as a jump (AST_JUMP) so the game's air rules apply

    // the take-off: the air hike's effect and sound (uPlayerNero::setWJumpEffect 0x7E8850), fired at the launch frame
    static bool  effect_enabled;
    static int   effect_list, effect_index;   // sDevil4Effect list / index: Nero's air hike is 1 / 0x28 (effect\efl\com\ec000_00v2),
                                              // Dante's 2 / 0x14 (ec005_20v1)
    static float effect_tilt;                 // degrees the effect leans from the player's up toward the jump direction (level part)
    static bool  se_enabled;
    static int   se_id;                       // SE of the player's sound resource (+0x1D60): the air hike plays 40

    static Dir  pick_direction(uintptr_t player);
    static bool ready(uintptr_t player);          // the LMT is in this player's bank
    static void ensure_bank(uintptr_t player);    // loads it when the slot is empty (per player object)
    static void __stdcall move(uintptr_t player); // the per-tick state function

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
};
