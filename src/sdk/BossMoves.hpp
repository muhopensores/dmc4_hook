#pragma once

#include <cstdint>

// Moves for the boss Dante (uPlayerDanteBoss), registered by the mods that add them.
//
// The boss plays Dante's moves by pressing buttons: each frame his current boss action sets his buttons and stick, and
// the player's own command checker turns them into moves through the attack table. A mod that adds or changes a Dante
// move registers how the boss performs it (weapon, the cancel window to wait for, a short input script relative to
// his target, the attack id that confirms it) and when he should (ground / air, distance and height to the target,
// which of his own actions it may stand in for, chance, cooldown). The registry is plain data, like MoveTable's rows
// and MoveDef's files; BossDante (mods/BossDante.cpp) runs it in Enhanced mode:
// - every move becomes a boss action after the game's 60, run by one generic action;
// - when his think picks one of his own actions (uPlayerDanteBoss::setBossAction 0x7C8940), each move that may stand
//   in for it and whose situation holds rolls its chance; the first that passes is played instead.
//
// A mod registers once, from its on_initialize, and points `enabled` at its own switch:
//     boss_moves::Move m;
//     m.name = "Air Stinger"; m.enabled = &AerialStinger::mod_enabled;
//     m.sword = boss_moves::SWORD_REBELLION; m.cancel = boss_moves::CANCEL_COMMAND;
//     m.input(boss_moves::BTN_LOCK_ON, boss_moves::STICK_TOWARD, 2).input(boss_moves::BTN_LOCK_ON | boss_moves::BTN_MELEE, boss_moves::STICK_TOWARD, 1);
//     m.atck_id = 9; m.where = boss_moves::WHERE_AIR; ...
//     boss_moves::add(m);
namespace boss_moves {

// The boss's own actions (DANTE_BOSS, the SE enum), for stands_in.
enum Action : int {
    ACTION_WAIT = 0, ACTION_FOOTWORK, ACTION_NEAR_PLAYER, ACTION_POSE_FOOTWORK, ACTION_POSE_WALK_F, ACTION_POSE_WALK_B,
    ACTION_POSE_WALK_L, ACTION_POSE_WALK_R, ACTION_STEP_F, ACTION_STEP_B, ACTION_STEP_L, ACTION_STEP_R, ACTION_STYLE,
    ACTION_STYLE_F, ACTION_STYLE_B, ACTION_STYLE_L, ACTION_STYLE_R, ACTION_PROVOKE, ACTION_PROVOKE_WALK, ACTION_JUMP,
    ACTION_W_JUMP, ACTION_JUMP_LAND_WAIT, ACTION_STINGER_JUMP, ACTION_AIR_ESC, ACTION_COVER_OUT, ACTION_WAIT_ACT_FRAME,
    ACTION_WAIT_FRAME, ACTION_TUTO_NEXT, ACTION_ATK_SHORT_NML0, ACTION_ATK_SHORT_NML1, ACTION_ATK_SHORT_NML2,
    ACTION_ATK_LONG, ACTION_ATK_LONG_SHOT_GUN, ACTION_ATK_LONG_PANDORA3, ACTION_ATK_PANDORA_BOX, ACTION_ATK_SHOOT_DOWN,
    ACTION_ATK_SHOOT_DOWN_F, ACTION_ATK_SHOOT_DOWN_B, ACTION_ATK_SHOOT_DOWN_L, ACTION_ATK_SHOOT_DOWN_R,
    ACTION_ATK_LONG_TUTO_B, ACTION_ATK_LONG_TUTO_C, ACTION_ATK_HIGHTIME, ACTION_ATK_STINGER, ACTION_ATK_STYLE,
    ACTION_ATK_STYLE_HIGHTIME, ACTION_ATK_STYLE_STINGER, ACTION_ATK_STYLE_AND_SHORT, ACTION_ATK_KABUTO,
    ACTION_ATK_DECENT_KICK, ACTION_DMG_INTERRUPT, ACTION_DMG_BLOWN, ACTION_DEAD, ACTION_REVERSE, ACTION_SNATCH,
    ACTION_BUSTER, ACTION_CHANGE_STYLE, ACTION_CHANGE_STYLE_LONG, ACTION_END_TUTORIAL, ACTION_RESET_TUTO_SUCC,
    ACTION_NUM
};
constexpr uint64_t bit(Action a) { return 1ull << a; }

// Interface buttons (uPlayer::getInterfaceBtnInfo 0x7A96D0, Dante).
constexpr uint32_t BTN_MELEE = 0x1, BTN_GUN = 0x2, BTN_JUMP = 0x4, BTN_TAUNT = 0x8, BTN_LOCK_ON = 0x10,
                   BTN_STYLE = 0x200, BTN_DT = 0x400;
// Weapons (setWepId 0x7C1D10 writes the same ids into the next-weapon slots +0x1DB4 / +0x1DB8).
constexpr int SWORD_REBELLION = 4, SWORD_GILGAMESH = 5, SWORD_LUCIFER = 6;
constexpr int GUN_COYOTE = 7, GUN_PANDORA = 8, GUN_EBONY_IVORY = 9;
// Styles (uPlayer::mStyle.kind +0x156C, what the attack table's style column is compared with).
constexpr int STYLE_SWORDMASTER = 0, STYLE_GUNSLINGER = 1, STYLE_TRICKSTER = 2, STYLE_ROYAL_GUARD = 3, STYLE_DARKSLAYER = 4;
// Cancel windows (uPlayer::mMotCncl[8], +0x30C0, 0x2C each; a window is open when its mStat is 2).
constexpr int CANCEL_NONE = -1, CANCEL_MELEE = 0, CANCEL_COMMAND = 3, CANCEL_JUMP = 4, CANCEL_GUN = 6;

enum Stick : uint8_t {
    STICK_FREE,     // left as the last action set it
    STICK_CENTRE,
    STICK_TOWARD,   // toward his target
    STICK_AWAY,
    STICK_LEFT,     // a quarter turn from toward
    STICK_RIGHT,
};
enum Where : uint8_t { WHERE_ANY, WHERE_GROUND, WHERE_AIR };

// One step of the input script. Several steps make a string (Stinger, jump cancel, Helm Breaker): a step can wait for
// a cancel window first, as a Macro clip's wait lines do, so recorded combos can use the same format later.
struct Input {
    uint32_t buttons = 0;     // held while this step lasts (a press when the previous step didn't hold them)
    Stick stick = STICK_FREE;
    uint8_t frames = 1;
    int8_t wait = CANCEL_NONE;   // a cancel window to wait for before this step (the move's start_timeout applies)
};
constexpr int kMaxInputs = 16;

struct Move {
    const char* name = nullptr;
    const bool* enabled = nullptr;   // the mod's switch; off = never picked (null = always on)

    // How
    int sword = -1;                  // switched to before the inputs (SWORD_*; waited for until it's out); -1 = whatever he holds
    int gun = -1;                    // GUN_*; -1 = whatever he holds
    int style = -1;                  // the style the move needs (STYLE_*): only picked while he is in it; -1 = any
    int cancel = CANCEL_NONE;        // the window to wait for before the inputs
    Input inputs[kMaxInputs] = {};
    int num_inputs = 0;
    int atck_id = -1;                // the attack id that confirms the move (lower-body action 12 with this mAtckId); -1 = none
    float start_timeout = 60.0f;     // frames to wait for the window or the confirmation before giving up
    float min_run = 0.0f;            // frames the move runs at least before he hands back (moves without atck_id: how long)

    // When
    Where where = WHERE_ANY;
    float min_dist = 0.0f, max_dist = 100000.0f;        // cm to his target, on the ground plane
    float min_rise = -100000.0f, max_rise = 100000.0f;  // the target's height minus his, cm
    uint64_t stands_in = 0;          // bit() of each of his actions this move may replace; 0 = never picked by his think
    float chance = 0.5f;             // per pick it qualifies for
    float cooldown = 300.0f;         // frames before it can be picked again
    bool (*check)(uintptr_t boss, uintptr_t target) = nullptr;   // an extra condition; null = none

    Move& input(uint32_t buttons, Stick stick, uint8_t frames = 1, int8_t wait = CANCEL_NONE) {
        if (num_inputs < kMaxInputs) inputs[num_inputs++] = Input{buttons, stick, frames, wait};
        return *this;
    }
};

constexpr int kFirstAction = 60;     // DANTE_BOSS ACTION_NUM: the first boss action id after the game's
constexpr int kMaxMoves = 60;

int add(const Move& m);              // the boss action id it got (kFirstAction + index), -1 when full or unnamed
int count();
const Move* get(int index);
int index_of_action(int action);     // the move a boss action id runs, -1 for the game's own

} // namespace boss_moves
