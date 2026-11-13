#pragma once

#include "MtMath.hpp"
#include <cstdint>

// SpineAim: procedural rotations of the player's spine and hips, applied after the motion blend and before the world
// matrices are built, so they compose with whatever the body plays. A service for mods, with no UI or settings of its
// own: ThirdPersonAim's chest-first aim, the wall run's lean while shooting, and the inertial blends of their shots
// (and UpperGun's). It installs its hooks on the first add_provider / install() and works only while a mod asks.
// What it offers, all asked per player per pass through PROVIDERS (no request state crosses the tick, because
// uPlayer::updateMatrix also runs for every shell the player fires):
//   - aim: joints 1 / 2 / 3 (spine_lower, spine_upper, neck) carry a yaw toward a wanted direction, split in shares
//     (0.30 / 0.45 / 0.25) within per-joint limits (20 / 25 / 40 deg), at most 70 deg, 6 deg per frame, as a rotation
//     about WORLD up expressed in each joint's own frame (a rotation about up leaves up invariant, so the chain's frames
//     give every joint's axis exactly; each bone's own axis turned a leaning spine's yaw partly into pitch). The wanted
//     yaw comes from the provider (Request::wanted_yaw, e.g. what the game's setLockOnTargetAngle asked for) or, as the
//     fallback, from the lock-on point; it is measured against the chest's own yaw this tick (the clavicle line by
//     forward kinematics before any poke, which absorbs whatever torso angle the playing motion holds), or against the
//     actor's facing with Request::ref_facing. Further round than 120 deg it goes back to neutral;
//   - hips / return: joint 0 (the root: legs and spine hang off it) turned about world up by Request::hips_deg, and
//     the same angle given back along the spine (Request::return_deg, 24 deg per frame), so the legs can face one way
//     and the upper body another: the game turns the whole actor toward a target, the hips turn back to the movement
//     direction;
//   - lean: joint 1 about a world axis (Request::lean_axis / lean_deg).
// The pokes go on right after every local pass of a player: uPlayerNero / uPlayerDante vtable slot 13 (+0x34,
// uModel::updateLocalMatrix 0x9E87E0, which rewrites every joint's mQuat) is replaced by local_pass, which runs it and
// then the tick, before any world pass (+0x38, the FK pass and the leg IK) uses the joints. (The first hook sat in the
// window between the two calls inside uPlayer::updateMatrix, 0x7A8D4A; during a shot the player's local pass also runs
// from elsewhere, and the drawn pose lost the twist.) RE: Vibed\RE\player_move_pipeline.md "Procedural spine aim" and
// the sections after it. Both
// characters share the joint numbers (checked in pl000.mod / pl006.mod); Nero's coat and Dante's head / hair / coat
// hang on body joints and follow for free.
//
// Settled in-game: the root poke yaws the whole character; the joint's local quaternion is post-multiplied (local =
// L * q; q * L bent the torso sideways) in the standard (Hamilton) conventions, confirmed against updateWorldMatrix's
// quaternion -> matrix code (rows = the standard rotation's columns, child = parent * local), so a positive rotation
// about up increases the game's yaw (atan2(x, z)).
// Known trap: mpLockTarget (+0x2000) is a cLockOnTarget (+8 the uActor*, +0x10 the lock-on point), not an actor.

namespace spine_aim {

// What a provider answers for one player on one pass.
struct Request {
    bool  aim = false;                           // twist toward the lock-on point (SpineAim computes the yaw unless wanted_set)
    bool  wanted_set = false;                    // the provider knows where the body should face
    float wanted_yaw = 0.0f;                     // that yaw, radians, the game's convention (atan2(x, z))
    bool  ref_facing = false;                    // measure the twist against the actor's facing, not the chest: what
                                                 // the motion aims along (the gun shots: their stances turn the chest side-on,
                                                 // so a chest reference swung the gun arm off by the stance angle)
    bool  aim_arms = false;                      // the whole aim twist on joints 1 and 2, which carry the arms (none on the neck,
                                                 // whose share only turned the head: a gun fell short by it)
    bool  hips_set = false;                      // turn joint 0 about world up by hips_deg (exact, every tick)
    float hips_deg = 0.0f;
    float return_deg = 0.0f;                     // the twist to give back along the spine (shares, joint limits), slewed
    float lean_deg = 0.0f;                       // rotate joint 1 about lean_axis by this (0 = none)
    glm::vec3 lean_axis{ 0.0f, 0.0f, 1.0f };     // world, unit
};
typedef bool (*Provider)(uintptr_t player, Request& out);   // true = `out` filled
bool add_provider(Provider fn);   // register from on_initialize (installs the hooks); at most 8. false = full, or a hook failed (logged)
bool install();                   // the hooks alone, for a mod that only calls inertialize; once, later calls return the result
void draw_readout(uintptr_t player);   // ImGui lines for a mod's readout: the last pass's aim / hips / lean, the player's fades

void __stdcall tick(uintptr_t player);   // runs between updateLocalMatrix and updateWorldMatrix
void __fastcall local_pass(uintptr_t model, uintptr_t edx);   // the players' vtable slot 13: the local pass, then tick
// runs at the entry of uModel::moveMotion 0x9F89D0 (through step_begin; top of the next tick, for every model; only
// players with a slot are touched): the joints' local quaternions get their pre-poke values back just before the engine reads them. Two
// engine paths start from the joints' current values: a slot given a new motion snapshots them there and crossfades
// from that, and a slot interpolating lerps from them in updateLocalMatrix. With the poke still in, the lean was
// carried over and added again, a pile-up that spun Dante's hips through his rapid E&I shots (every shot a new motion,
// a slot always interpolating). Undoing it right after the world pass instead (the first try) lost the twist while a
// shot played: something during a shot builds the player's world matrices again from the joints (the drawn pose then
// had no poke), so the pokes now stay in until the next tick's motion step.
void __stdcall unpoke(uintptr_t player);       // also takes off the inertialization offsets
void __stdcall step_begin(uintptr_t model);    // the moveMotion-entry hook: unpoke, and a new motion step for the fades
void detour_unpoke();
extern uintptr_t jmp_ret_unpoke;  // 0x9F89D6

// Inertialization (2026-10-26; David Bollo, GDC 2018, the Gears of War transitions): instead of the engine's crossfade
// (a frozen snapshot of the pose on screen lerped toward the new motion, which dilutes a short shot for most of its
// length), the caller cuts to the new motion (its request's interpolation set to 0) and calls inertialize(): at the
// next local pass, per joint, the rotation / translation from the new motion's pose to the pose shown last tick, and
// the speed it had, become an offset that a quintic fades to zero over `frames` (shorter when the speed would get there
// first). The new motion plays at full strength at once and the joints carry on at the speed they had. Every joint of
// the model; the offsets go on before SpineAim's pokes and come off with them. Players only (the vtable slot 13 wrapper).
bool inertialize(uintptr_t player, float frames);   // false = not available (not installed, no pose recorded yet): keep the crossfade

} // namespace spine_aim
