#pragma once
#include <cstdint>

// CoopContext: which player a unit's update runs for. sMediator::mpPlayer (+0x24) is the one player most of the game
// reads, so a unit acting for player 2-4 gets that player written there for the length of its update and player 1 put
// back after, with mpCamera (+0xD0, the active main camera: uCameraCtrl::resetCamera 0x4F7AF5 / resumeCamera 0x4F7B60,
// the boss cameras' setup) = that player's camera and sDevil4Pad::mpInstance (0xE559C4) = his pad (CoopPads' stand-in).
// The keyboard is left alone: CoopPads already gates its three reads in the player code. Done at sUnit's serial call
// sites only: init 0x8DC324 (esi = unit, vtbl +0x14), tick 0x8DC39C (esi, +0x18), pass 2 0x8DC44B (edi, +0x1C) and
// pass 3 0x8DC4C4 (esi, +0x20); a unit ticked as a job never swaps (jobs go through 0x8DC7C0 / 0x8DC7D0), so the swap
// can't leak into units running beside it on other threads. While players 2-4 are in the room the context lines run
// serially (coop_diag).
//
// Owners:
// - A player 2-4 himself, his HUD and lock-on reticle (coop::Player::hud / reticle) and his camera (coop::Player::cam;
//   mpCamera = the camera itself).
//   The player code reads mpCamera for the stick's direction (setInterfaceAnlgInfo 0x7A9553, setCameraAngle 0x7ABCB6),
//   moveEx and a few moves, the HUDs read mpPlayer (uCockpit::checkPtr and the Nero / Dante ones), the camera reads
//   mpPlayer's lock-on and the pad's right stick.
// - Units made for him, by class (getDTI, vtbl +0x10, walked up mp_parent): uShell mpSrcActor +0x1370 (uPlNeroShl001
//   / 003 read mpPlayer), uPlShl mpParent +0x17B0, uPlWeapon mpParent +0x1378, uPlNeroDevil mpParent +0x1370.
// - An enemy held by an extra Nero. Nero's grab moves (moveAtckRightHand{Long,Short}{,Air}Grab, mAtckId
// 1 / 2 Snatch, 5 / 6 Buster, 7 / 8 Buster on a blocking enemy) clear mGrabHitInfo.actor (+0xCCD0) at their start and
// uPlayerNero::setGrabHitInfo 0x83B04F sets it on the hit; the held enemy's own grab actions (uEm000Base::grabNear
// 0x5532F0 ...) place it from mpPlayer / sMediator::getPlayerMat, which put it on player 1. The owner stays for kGrace
// game ticks after the grab move ends, for the throw and the blown flight set up at the release.
// The grab hit itself is sent outside the enemy's update: sDevil4Main::move 0x8AF03A -> sCollisionGame::checkAttackMain
// -> checkGrabProcess -> checkGrabObj -> uPlWpRightHand::uCollisionCtrl::grabHitMessage 0x83DFE0 (main thread), which
// calls the enemy's grab / Snatch messages; those start the grab from mpPlayer (uEm000Base::setGrabBlown 0x552B40 takes
// the Snatch's end from mpPlayer's right hand mRetPos +0x33E0: player 1's is zero, so the room origin). Coop wraps that
// call in the context of the hand's Nero (uPlWpRightHand::mpParentPl +0x2F34 = the collision controller +0x1B84).

namespace coop_context {

// The unit's owner while it updates; true when mpPlayer was swapped (leave() puts player 1 back).
bool enter(uintptr_t unit);
// Same for code run on behalf of `player`: swaps only for players 2-4.
bool enter_player(uintptr_t player);
void leave();

// Once a frame on the game thread: serial context lines while extra players exist, the grace timers.
void game_tick(bool extra_players);
void reset();

// Lock-on for players 2-4, around uPlayer::updateLockOn 0x820840. Every cLockOnTarget (sLockOnTargetMgr 0xE558B0,
// lists mpTop +0x20 / mpSnatchTop +0x24) carries mPILength +0x24, mIsIn +0x34, mIsCameraIn +0x37 and mNearPos +0x40,
// which each enemy fills in its own update for mpPlayer only (uEm000Base::postMain 0x543A71 and 36 other callers of
// uActor::getNearLengthSpherIdx 0x4A8620: distance from the player's center, mIsIn = inside a cone from the player's
// mLockOn.quat (sub_4A8200), mIsCameraIn = sCamera::isDispSphere 0x8EE300, always viewport 0). updateLockOn's pickers
// (passive sub_451200 / sub_4512D0, the lock-on button's sort and sub_48B3F0) choose by them, so players 2-4 locked on
// to what was nearest player 1 and in his view. lock_on_begin swaps mpPlayer to the player, saves the fields, refills
// them for the player (getNearLengthSpherIdx from its getCenterPos, the camera test against its own viewport's
// mFrustum). They stay in for his whole context, not only updateLockOn: the moves that pick or turn to a target later
// in his update (the Buster turned to player 1's enemies) and his shells read them too. Computed once per game tick,
// written in when one of his units starts updating (swap_for), player 1's put back when it ends. lock_on_begin is
// for an updateLockOn outside his context (false: not a player 2-4, or his values are in already).
bool lock_on_begin(uintptr_t player);
void lock_on_report(uintptr_t player);   // readout: what updateLockOn picked, while his values are in
void lock_on_end();
// cLockOnTarget::mIsInterTarget (+0x39) is one flag per target for every player: uPlayer::setLockOnTargetAngle 0x820BD0
// (an attack turning to its target) clears it on all and sets it on its target (also kept in the player's mpLockTarget
// +0x2000); updateLockOn clears it unless mActStat (+0x1554) bit 4 (0x820880 / 0x820891, NOPed by MultiCharFixes'
// Targeting Fix) and, while a gun is held (mPose.holdTimer), makes the first flagged target the gun's (sub_48B450 ->
// pInterLongTarget). So one player's guns aimed where the other's last attack turned. Before each player's lock-on
// update (player 1 too, while players 2-4 exist) the flags are set from his own mpLockTarget, with the game's clear rule
// (kept while bit 4 is set, or always with the Targeting Fix).
void inter_flags_for(uintptr_t player);

// The viewport of the player whose context is in (1-3), -1 for player 1 / none. sCamera::isDispSphere 0x8EE300 tests
// viewport 0's frustum only, and the lock-on camera (cCameraPlayer::main01 0x41A5AE) adjusts by it: player 2's camera
// turned whenever his target left player 1's view, and snapped back. Coop's hook tests this viewport instead.
extern int view_slot;
// sCamera::isDispSphere against viewport `slot` (MtSphere: x y z r).
bool sphere_in_viewport(const void* camera_system, int slot, const float* sphere);

// Enemies (step 4a): each enemy unit, its shells and the boss Dante's shots update in the context of the player it is
// after (enter(); none for player 1). Kept per enemy, picked once a game tick: the nearest player, switching only to
// one clearly nearer (0.7 of the distance) and not within 60 ticks of the last switch; a hit sets it to the attacker
// for 180 ticks, a Nero's grab to the holder. The enemy's own lock-on targets are refilled for that player by its
// update; player 1's values for them are computed again when it ends.
// Hits (step 4e): uActor::damagePreMessage 0x4A7F30 (edi = the actor hit, stack: attacker group, target group; called
// only from sCollisionGame::checkAttackObj 0x47C4A7, main thread, outside unit updates) runs in the context of the
// player hit, else of the attacking player (cCollisionGroup +0x98 -> uCollisionMgr +0xD8 mpReportActor), so damage,
// Devil Trigger gain, style and the reactions read the right player. True when it swapped (leave() after).
bool hit_enter(uintptr_t attacker_group, uintptr_t target);

// Readout.
uint32_t swaps_last_frame();
// The grab hit wrapper reports each hit (game thread): the hand's Nero, whether it swapped, the target group.
void note_grab_hit(uintptr_t hand_owner, bool swapped, const void* target_group);
void draw_diag();

} // namespace coop_context
