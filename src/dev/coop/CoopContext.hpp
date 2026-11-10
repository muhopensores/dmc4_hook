#pragma once
#include <cstdint>

// CoopContext: which player a unit's update runs for. sMediator::mpPlayer (+0x24) is the one player most of the game
// reads, so a unit acting for player 2-4 gets that player written there for the length of its update and player 1 put
// back after. Done at sUnit's serial call sites only, tick 0x8DC39C (esi = unit, vtbl +0x18), pass 2 0x8DC44B (edi,
// +0x1C) and pass 3 0x8DC4C4 (esi, +0x20, always serial); a unit ticked as a job never swaps (jobs go through 0x8DC7C0 /
// 0x8DC7D0), so the swap can't leak into units running beside it on other threads. While players 2-4 are in the room
// the context lines run serially (coop_diag).
//
// Owners so far:
// - A player 2-4's camera (coop::Player::cam): mpPlayer = that player and mpCamera (+0xD0, the active main camera, set
//   by uCameraCtrl::resetCamera 0x4F7AF5 / resumeCamera 0x4F7B60 and the boss cameras' setup) = the camera itself, so
//   its modes, lock-on framing and throw cameras follow its player (player 1's lock-on used to swing it).
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
// mFrustum); lock_on_end puts player 1's values back. False = not a player 2-4, nothing done.
bool lock_on_begin(uintptr_t player);
void lock_on_end();

// Readout.
uint32_t swaps_last_frame();
// The grab hit wrapper reports each hit (game thread): the hand's Nero, whether it swapped, the target group.
void note_grab_hit(uintptr_t hand_owner, bool swapped, const void* target_group);
void draw_diag();

} // namespace coop_context
