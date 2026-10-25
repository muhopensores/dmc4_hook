#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"

// SpineAim: procedural rotations of the player's spine and hips, applied after the motion blend and before the world
// matrices are built, so they compose with whatever the body plays. A standalone tool (2026-10-25): the wall run asks
// it for the torso lean while shooting; the aim / hips / return parts are kept for a future third-person / aiming mode.
// What it offers, all asked per player per pass through PROVIDERS (no request state crosses the tick, because
// uPlayer::updateMatrix also runs for every shell the player fires):
//   - aim: joints 1 / 2 / 3 (spine_lower, spine_upper, neck) carry a yaw toward a wanted direction, split in shares
//     within per-joint limits, as a rotation about WORLD up expressed in each joint's own frame (`axis_mode` 1; a
//     rotation about up leaves up invariant, so the chain's frames give every joint's axis exactly) or about each
//     bone's own axis (`axis_mode` 0). The wanted yaw comes from the provider (Request::wanted_yaw, e.g. what the
//     game's setLockOnTargetAngle asked for) or, as the fallback, from the lock-on point; it is measured against the
//     actor's facing or against the chest's own yaw this tick (`ref_mode` 1: the clavicle line by forward kinematics
//     before any poke, which absorbs whatever torso angle the playing motion holds);
//   - hips / return: joint 0 (the root: legs and spine hang off it) turned about world up by Request::hips_deg, and
//     the same angle given back along the spine (Request::return_deg), so the legs can face one way and the upper
//     body another: the game turns the whole actor toward a target, the hips turn back to the movement direction;
//   - lean: joint 1 about a world axis (Request::lean_axis / lean_deg).
// The hook is the 7-byte window in uPlayer::updateMatrix 0x7A8D40 between its two virtual calls: 0x7A8D4A
// `mov eax,[esi]; mov edx,[eax+38h]; mov ecx,esi` (esi = the player) sits after uModel::updateLocalMatrix (vtbl
// +0x34, which rewrites every joint's mQuat each tick) and before uModel::updateWorldMatrix (+0x38, the FK pass
// and the leg IK). RE: Vibed\RE\player_move_pipeline.md "Procedural spine aim" and the sections after it. Both
// characters share the joint numbers (checked in pl000.mod / pl006.mod); Nero's coat and Dante's head / hair / coat
// hang on body joints and follow for free.
//
// Settled in-game: the root poke yaws the whole character (`Test hips`); quaternion order 0 (local = L * q) and the
// standard (Hamilton) conventions, confirmed against updateWorldMatrix's quaternion -> matrix code (rows = the
// standard rotation's columns, child = parent * local), so a positive rotation about up increases the game's yaw
// (atan2(x, z)). `Flip` / `Hips flip` stay as toggles; `Test angle` / `Test hips` apply constants for calibration.
// Known trap: mpLockTarget (+0x2000) is a cLockOnTarget (+8 the uActor*, +0x10 the lock-on point), not an actor.

namespace spine_aim {

extern bool  mod_enabled;
extern bool  aim_always;       // debug: aim on every gun pose with no provider asking
extern float max_deg;          // the aim twist is clamped to +-max_deg
extern float slew_deg;         // degrees per frame the aim twist may change
extern float share[3];         // how a twist splits over joints 1, 2, 3 (sum ~1)
extern float joint_max[3];     // the spine constraint: no joint turns past its own limit, whatever the shares ask
extern float aim_gain;         // the yaw error is multiplied by this before the clamp
extern float aim_offset_deg;   // added to the error on the target's side
extern float unwind_deg;       // an error further round than this makes the aim return to neutral instead of sitting at the clamp
extern int   axis_mode;        // 0 = each joint's bone axis, 1 = world up in each joint's frame (a true yaw)
extern int   ref_mode;         // the wanted yaw is measured against: 0 = the actor's facing (mRotate.mNow), 1 = the chest's own yaw this tick
extern float return_slew_deg;  // degrees per frame the spine's return twist may change (the game turns the actor at 24)
extern int   order;            // quaternion composition variant: 0 local = L * q, 1 local = q * L
extern bool  flip;             // negate the aim twist
extern bool  hips_flip;        // negate the hips counter-yaw and the spine's return together
extern float test_deg;         // non-zero: a constant aim twist (calibration)
extern float test_hips_deg;    // non-zero: a constant rotation of joint 0 about world up (the whole character must yaw)
extern uintptr_t jmp_ret;      // 0x7A8D51

// What a provider answers for one player on one pass.
struct Request {
    bool  aim = false;                           // twist toward the lock-on point (SpineAim computes the yaw unless wanted_set)
    bool  wanted_set = false;                    // the provider knows where the body should face
    float wanted_yaw = 0.0f;                     // that yaw, radians, the game's convention (atan2(x, z))
    bool  hips_set = false;                      // turn joint 0 about world up by hips_deg (exact, every tick)
    float hips_deg = 0.0f;
    float return_deg = 0.0f;                     // the twist to give back along the spine (shares, joint limits), slewed
    float lean_deg = 0.0f;                       // rotate joint 1 about lean_axis by this (0 = none)
    glm::vec3 lean_axis{ 0.0f, 0.0f, 1.0f };     // world, unit
};
typedef bool (*Provider)(uintptr_t player, Request& out);   // true = `out` filled
bool add_provider(Provider fn);                              // register from on_initialize; at most 8

void __stdcall tick(uintptr_t player);   // runs between updateLocalMatrix and updateWorldMatrix
void detour();
// runs right after updateWorldMatrix: the world matrices (what is drawn, what the weapons and the coat follow) carry the
// pokes, and the joints' local quaternions get their pre-poke values back, so nothing poked survives into the next
// tick. Two engine paths start from the joints' current values: a slot given a new motion snapshots them in
// uModel::moveMotion (top of the next tick) and crossfades from that, and a slot interpolating lerps from them in
// updateLocalMatrix. With the poke still in, the lean was carried over and added again, a pile-up that spun Dante's hips
// through his rapid E&I shots (every shot a new motion, a slot always interpolating).
void __stdcall unpoke(uintptr_t player);
void detour_unpoke();
extern uintptr_t jmp_ret_unpoke;  // 0x7A8D59

} // namespace spine_aim

class SpineAim : public Mod {
public:
    SpineAim() = default;
    std::string get_mod_name() override { return "SpineAim"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook, hook_unpoke;
};
