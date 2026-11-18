#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun, Gerb or the dodge

// FreeRide: DMC3's Free Ride for Nero and Dante. Lock-on + forward + jump next to a knocked-down enemy: the player hops
// onto it and surfs it along the ground, steering with the stick; jump steps off (the game's enemy step), melee
// launches it (a hand-off to the character's launcher), and the ride ends by itself after 240 frames. Every dismount
// but the launcher is the game's own enemy step (uPlayer action 25). The tuning is fixed in FreeRide.cpp; the panel keeps
// the on / off, the input's stick condition, Nero's Exceed rev and how the enemy is held.
//
// How the enemy is held (RE notes Vibed\RE\nero_snatch_grab_long.md / nero_charge_shot_launch.md):
//   - during the ride, by default (hit_mode HIT_DAMAGE), a hitbox shell on the enemy (FreeRideShell.hpp) hits it for a
//     little damage every 12 frames; each
//     interrupting hit replays its down flinch (action 97 -> motion 65 -> 60 frames of setDownLoop), so it stays down the
//     game's own way and takes damage, as DMC3's Free Ride does. The ride hops off if the enemy is off AS_DOWN for 20 frames.
//   - during the hop onto it, the hit stop: every enemy class's main runs its routine only while uEnemy::mEmHitStop
//     (+0x1438) is clear, and uEnemy::move (0x7308B0) sets that flag while the hit-stop timer mEmHitStopTimer (+0x1434) is
//     above 0. The mount tops the timer up every tick, so the enemy's AI, motion and down timer stand still in the pose it
//     was knocked into; the landing releases it and the hits take over. With HIT_STOP the ride keeps topping the timer up
//     instead, so the enemy stays frozen until the ride ends, then carries on from where it was, still down. Its position
//     follows the board (written each tick under the player).
//   - the push capsules are kept apart by putting the enemy on the player's uCollisionMgr ignore list for the ride
//     (sCollisionGame::checkPush 0x47BDA0 skips the pair; uEm013 does the same with the player while carrying him).
//     Every exit, and the pre-check tick after a ride cut short, takes it off again.
//   - the mount needs the lock-on target (an enemy, not a boss) to be lying down: uActor::mActorStatus (+0xEA8) AS_DOWN
//     0x4, the same bit the charge shot's blownMessage reads to pick High Time over a blow-back. So the melee exit's
//     launcher hits an enemy that is down, which every enemy turns into a launch (damage type 4 UPPER). And lying flat
//     (`enemy_flat`): AS_AIR 0x2 clear (a launch and its fall carry AS_DOWN too, and the hit stop would freeze the enemy in
//     mid-air), no vertical speed, within 12 cm of the floor under it, for 6 frames running (a bounce's touch-down doesn't
//     count). The status bits alone can't tell: each enemy class writes its own mix in its down states (uEm000Base 0xC
//     falling / 4 lying / 0xC getting up, uEm010 and uEm029 8, uEm017's loop 0).
//
// Rows (MoveTable::add_row, one per character, the same state function): Nero 0x42 (Gerb has 0x40 / 0x41), Dante 0x70
// (LuciAirThrow 0x6D, the dodge 0x6E, the MoveDef dodge example 0x6F). Ground only. Both rows trigger on one command bit
// (taken with input_commands::next_free_bit): lock-on held, the stick forward toward the target (optional), jump
// pressed, while `can_mount` holds. The ground jump check (uPlayer*::checkJumpBegin, vtbl +0x258) runs before checkAtck
// in uPlayer::checkBeforeAction 0x803500, so the pre-check tick eats the jump press once the command has fired: the
// row then gets it, the jump check doesn't.
//
// Motions: motion\pl000\freeride.lmt (Nero) / motion\pl006\freeride.lmt (Dante) when present, one motion_registry set
// that the registry merges into a free bank of the player's models (it picks the bank and hands out the words). The mount
// is always the game's hop (the enemy step's motion 0x11); block 0 is the start-up (played on the board, standing still
// over the enemy, between the mount and the ride, until the block ends) and block 1 the ride (looping). Without the file,
// or for a block it lacks, the jump motion (5) held still at frame 12 stands in for the ride and the start-up is skipped.
//
// Nero's Exceed gauge (uPlayerNero::cOverDriveGuage at +0xCCE4) revs by itself during the ride (`exceed_fill`): the bar is
// written each tick and cOverDriveGuage::addStock 0x7EA340 lights a stock every 80 frames, with the game's effect and sound.
//
// The gun during the ride: UpperGun (mods/UpperGun.hpp, the wall run's shooting made shareable) starts the
// character's ground gun row on the upper half over the riding legs. The ride is attack 12 with its own mAtckId, which the
// shot shares, so UpperGun swaps the word between the two halves' passes (move() ends with lower_pass_done) and keeps
// the cancel gate off while riding. Dante's Gunslinger moves (Twosome Time, Fireworks ...) are whole-body moves that run in
// the lower phase word, so they can't ride the legs: the style button hands the body to the game's own row ON the board:
// the ride action is suspended, its physics (the board's motion, the floor, the enemy under the player, the hits) go on
// from the post-action tick, and when the move ends in the footwork the ride action and its motion come back
// (FreeRide.cpp style_gun_move / style_post / resume_ride).
//
// Sounds: the custom motions carry no rMotionSe events of their own, so the mount, the landing on the
// enemy and the ride (repeated every 20 frames, stopped by the exits) each play a request (`SeRef`: a bank and an id)
// through sDevil4Sound::requestSe, like uPlayer::cSECtrl::request: the character's own bank (uPlayer +0x1D60 [0],
// pl00X_core) or the common footstep bank (sound\\se\\player\\common\\foot_default, loaded once with sResource::create).
// They are the game's foley: FT_Jump + the character's whoosh, FT_Land, FT_Sliding. The exits keep their own sounds (the
// enemy step's and the launcher's motions have events).
//
// Weapon and style changes work during the ride as on the ground: the game reads them in checkBeforeAction,
// which only runs with the cancel gate on, so the pre-check tick runs uPlayer::checkWeaponChange / uPlayerDante::checkStyleChange
// itself; the model swap (cWeaponCtrl::update) is withheld from attack actions, so the post-action tick sets its flag while
// the arms aren't in a shot. A press under a shot is kept until the shot is over: the gun shots end in a whole-body footwork
// when the chosen gun differs from the held one, which would cut the ride short. Pandora is disabled for the ride.
//
// The move is `namespace free_ride`; the FreeRide Mod class is the framework plumbing.

namespace free_ride {

constexpr uint32_t nero_move_id  = 0x42;
constexpr uint32_t dante_move_id = 0x70;
enum Character : int { NERO = 0, DANTE = 1 };

extern bool mod_enabled;
extern bool need_forward;         // the stick must lean toward the target (within 45 deg) before the jump press
extern bool exceed_fill;          // Nero: the ride revs the Exceed gauge (cOverDriveGuage) by itself
enum HitMode : int { HIT_STOP = 0, HIT_DAMAGE = 1 };
extern int  hit_mode;             // how the ride holds the enemy: the hit shell's hits, or the hit stop

bool can_mount(uintptr_t player);                      // the command's predicate
bool ride_active(uintptr_t player, uint32_t serial);   // the ride `serial` of `player` is still on the board (the hit shell's lifetime)
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

    std::unique_ptr<FunctionHook> hook_cmd, hook_post;
};
