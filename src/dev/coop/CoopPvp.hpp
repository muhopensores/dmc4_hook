#pragma once

#include <optional>
#include <string>

// CoopPvp: players fighting each other (Vibed\Coop\README.md, "Addendum: PvP mode").
//
// V1, hits:
// - Each collision group's mVsAttr (+0x0C) holds a nibble per side (0 player, 1 player shell, 2-7 enemies and the
//   rest): 2 = attacks that side's damage groups, 1 = hit by that side's attacks. sCollisionGame::checkAttackProcess
//   0x47BF90 walks the damage chain of every side whose 2 bit an attack group has, and checkAttackObj 0x47C300 hits a
//   damage group whose 1 bit has the attacker's side. uCollisionMgr::setVsAttr 0x50C650 builds the mask from the
//   manager's own tables (+0x2B0 player attack 0x22222200, +0x2B4 player damage 0x11111100 ...: no player bits);
//   it runs when a group is made (setCollision, shells too) and every frame for motion-driven managers (move 0x50CE8F).
//   With PvP hits on, after setVsAttr a group of a player or of something he owns gets 0x2 (attacks players) when it
//   attacks and 0x11 (hit by players and their shells) when it takes damage. Enemies' masks don't change.
// - checkAttackObj skips a pair that shares a manager, a model, an ignore entry or a manager ID, which a player's
//   shells and his own body don't. With PvP hits on, a pair whose two groups belong to the same player is skipped too
//   (0x47C45E, the mask test).
// - The hit itself is the game's: uActor::damagePreMessage, in the context of the player hit (CoopContext hit_enter).
//
// V2, grabs (Snatch, Buster and the hold, generic reactions only), after the boss Dante (uPlayerDanteBoss):
// - The test: checkGrabProcess 0x47C4E0 tests grab groups against sides 2-7 only and players have no grab-receiving
//   group, so after the game's grab pass (0x47BCB1) each grab group of a Nero's hand (uPlWpRightHand::uCollisionCtrl,
//   vtable 0xBEACC0, its Nero at +0x1B84, mode +0x137C: 1 Snatch, 3 Buster) is tested against the other players' body
//   damage groups with sCollisionGame::checkGroup_ (mode 2), and a contact calls uActor::grabPreMessage as the game does.
//   Only a stunned player is grabbed: in the damage action (mActionNo 20) with a hit reaction (DMAGE_ID 1, 2, 3, 5,
//   6, 10, 11), not invincible (the checks of uPlayer::canBeGrabbed 0x7AEF60).
// - grabHitMessageNormal 0x83E260 takes a player target (mActorType 1) down the enemy branch (3) at 0x83E2CB; there
//   the target's grab message (vtable +0xDC, every uPlayer's 0x8D4630 returns 0) answers 3 for a Snatch (Nero pulls
//   it) and 9 for a Buster, as uPlayerDanteBoss::grabMessage 0x7C5BC0, and starts the victim's reaction.
// - Nero: setShortGrabReaction 0x7EB06E accepts a held player like an enemy; setShortGrabEnemyReactionFunc 0x7EB1AE
//   gives him the boss Dante's set (grab bank 27, motion\pl000\pl006\pl000_pl006_00 and its coat / stand / arm files,
//   bossDantefunc 0xE16158) without the Devil Trigger bit: mvAtckSGScare / SGAScare. Those four LMTs are in em_dante.arc
//   only: while PvP hits is on, their rows and pl006_09 are put in the resource cache from nativePC\rom\enemy\
//   em_dante.arc (sdk/ArcRows); without them players aren't grabbed.
// - The victim: a damage action of our own type (0x20, uPlayer::moveActDamage 0x8238B0 hooked), from the boss's
//   moveActSnatch 0x7C3EE0 / moveActBuster 0x7C4630. Buster: motion 0x804 at Nero's position facing him, his frame
//   time, until Nero lets go; then the game's blow (DMG_ID_BUT, moveDmgButtobi). Snatch: 0x800, pulled to Nero over
//   10 frames, 0x801 / 0x80C landing, back to footwork. Dante plays his own bank 8 (pl006_09); a Nero gets the same
//   file in his bank 9 (reserved from the motion registry; his coat gets the empty pl000_03_pl006_00, as the Buster
//   loader does), with a .ctn script that only ends (his .ctn has no bank 9).
//
// V2, lock-on: while PvP hits is on each player is a lock-on target for the others (a cLockOnTarget of ours in
// sLockOnTargetMgr's list, the boss Dante's way); see CoopPvp.cpp.
//
// V4, flinching: a hit by another player uses PvP thresholds instead of the player's .dfd ones (uDamage::calc
// 0x51BE10, mpDefendStatus swapped for the call); see CoopPvp.cpp.

namespace coop_pvp {

extern bool hits;   // players' attacks hit the other players; Nero's Snatch / Buster grab them
extern bool players_first;   // lock-on picks another player over enemies while one is in reach and in view
extern bool dt_from_players; // hitting a player fills the attacker's Devil Trigger as hitting an enemy does
// Hits from another player needed to flinch / blow a player, per attack level 0-4 (-1 = never); [0] normal, [1] in
// Devil Trigger. Counters reset after the reaction or 3 s after the last hit.
extern int flinch[2][5];
extern int blow[2][5];

std::optional<std::string> install();   // the hooks; an error text on failure
void tick();                            // game thread, once a frame: the em_dante rows in while PvP hits is on
void retry();                           // try loading them again (PvP toggled)
void stage_end();
void before_lock_on(uintptr_t player);  // before a player's uPlayer::updateLockOn: the players' targets for him
void prefer_players(uintptr_t player);  // after his lock-on values are in, just before the game's updateLockOn
void after_lock_on();                   // after it: what prefer_players hid comes back
bool grabs_ready();                     // the rows are in: players can be grabbed
const char* grabs_error();              // why not
void draw_grab_diag();                   // the grab counters (Coop's Diagnostics)

} // namespace coop_pvp
