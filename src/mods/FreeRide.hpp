#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun, Gerb or the dodge

// FreeRide: DMC3's Free Ride for Nero and Dante. Lock-on + forward + jump next to a knocked-down enemy: the player hops
// onto it and surfs it along the ground, steering with the stick; jump steps off (the game's enemy step), melee
// launches it (a hand-off to the character's launcher), and the ride ends by itself after `ride_frames`. Every dismount
// but the launcher is the game's own enemy step (uPlayer action 25).
//
// How the enemy is held (RE notes Vibed\RE\nero_snatch_grab_long.md / nero_charge_shot_launch.md, 2026-10-29):
//   - every enemy class's main runs its routine only while uEnemy::mEmHitStop (+0x1438) is clear, and uEnemy::move
//     (0x7308B0) sets that flag while the hit-stop timer mEmHitStopTimer (+0x1434) is above 0. The ride tops the timer
//     up every tick, so the enemy's AI, motion and down timer stand still in the pose it was knocked into, and its
//     position follows the board (written each tick under the player). When the ride stops topping it up, for any
//     reason (an exit, the player hit out of it), the enemy carries on from where it was, still down.
//   - the push capsules are kept apart by putting the enemy on the player's uCollisionMgr ignore list for the ride
//     (sCollisionGame::checkPush 0x47BDA0 skips the pair; uEm013 does the same with the player while carrying him).
//     Every exit, and the pre-check tick after a ride cut short, takes it off again.
//   - the mount needs the lock-on target to be lying down: uActor::mActorStatus (+0xEA8) AS_DOWN 0x4, the same bit
//     the charge shot's blownMessage reads to pick High Time over a blow-back. So the melee exit's launcher hits an
//     enemy that is down, which every enemy turns into a launch (damage type 4 UPPER).
//
// Rows (MoveTable::add_row, one per character, the same state function): Nero 0x42 (Gerb has 0x40 / 0x41), Dante 0x70
// (LuciAirThrow 0x6D, the dodge 0x6E, the MoveDef dodge example 0x6F). Ground only. Both rows trigger on one command bit
// (taken with input_commands::next_free_bit): lock-on held, the stick forward toward the target (optional), jump
// pressed, while `can_mount` holds. The ground jump check (uPlayer*::checkJumpBegin, vtbl +0x258) runs before checkAtck
// in uPlayer::checkBeforeAction 0x803500, so the pre-check tick eats the jump press once the command has fired: the
// row then gets it, the jump check doesn't.
//
// Motions: motion\pl000\freeride.lmt (Nero) / motion\pl006\freeride.lmt (Dante) when present, one motion_registry set
// that the registry merges into a free bank of the player's models (it picks the bank and hands out the words), slots
// by default -1 = the game's hop for the mount and 0 for the ride (looping). Without the file, or for a slot of -1, the
// game's own motions stand in: the enemy step's hop (0x11) for the mount and the jump motion (5) held still for the ride.
//
// The move is `namespace free_ride`; the FreeRide Mod class is the framework plumbing.

namespace free_ride {

constexpr uint32_t nero_move_id  = 0x42;
constexpr uint32_t dante_move_id = 0x70;
enum Character : int { NERO = 0, DANTE = 1 };

extern bool mod_enabled;

// the input
extern bool  need_forward;        // the stick must lean toward the target (within 45 deg) before the jump press
extern float mount_range;         // cm, level distance to the target
extern float mount_height;        // cm, the target's height difference allowed either way
extern bool  allow_bosses;        // lock-on type 1 (bosses) as well as 0 (enemies)

// the LMT, per character
extern int  lmt_set;              // the motion registry's handle of our set (a body LMT per character), -1 = none
extern char lmt_path[2][128];     // game path without extension
extern int  slot_mount, slot_ride;   // LMT blocks, -1 = the stand-in (default: mount -1, ride 0)
extern int  ctn_bank, ctn_slot;   // the .ctn script borrowed for the LMT motions (runtime banks have none)
// the stand-ins without the LMT (bank 0 motions both characters share)
extern int   vanilla_mount_motion;   // 0x11 = the enemy step (uPlayer::moveActEmJump 0x827890)
extern int   vanilla_ride_motion;    // 5 = the jump (uPlayer::moveActJumpBegin 0x8221D0)
extern float vanilla_ride_frame;     // the frame the ride motion is held at (speed 0)

// the ride (cm, cm per frame, frames)
extern float mount_frames;        // the hop onto the enemy
extern float hop_height;          // the hop's apex above the straight line
extern float ride_height;         // the player's feet above the floor under the enemy
extern float ride_speed;
extern float turn_rate;           // degrees per frame the board turns toward the stick
extern float ride_frames;         // the ride's length (0 = until an exit)
extern float step_down;           // how far below the board the floor may drop before the ride leaves the ledge
extern float hold_frames;         // the enemy's hit-stop timer is kept at least this high
extern bool  align_enemy;         // turn the enemy with the board
extern float enemy_yaw_offset;    // degrees added to the board's direction for the enemy's yaw

// the exits
extern int   nero_finisher;       // melee: the atckId handed off to (0x1F High Roller), -1 = melee hops off
extern int   dante_finisher[3];   // per sword (4 Rebellion, 5 Gilgamesh, 6 Lucifer): 0xB = Rebellion's High Time, -1 = hop off
extern float finish_back;         // cm the player is put back from the enemy before the launcher, on the floor

bool can_mount(uintptr_t player);                      // the command's predicate
void __stdcall move(uintptr_t player, uint32_t atck_id);
void apply_rows();                                     // writes the command bit and the enable byte into both rows

} // namespace free_ride

class FreeRide : public Mod {
public:
    FreeRide() = default;
    std::string get_mod_name() override { return "FreeRide"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_cmd;
};
