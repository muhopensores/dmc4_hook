#pragma once

#include "../mod.hpp"
#include "../sdk/MtMath.hpp"

// ThirdPersonAim: an over-the-shoulder aim mode for the guns (2026-10-25). Mouse: hold (or toggle) the right button to
// aim, the mouse turns the camera, the left button fires and charges. Gamepad: a captured button combination aims, the
// right stick turns, the gun button fires and charges as usual. The local player only; melee waits while aiming.
// How (RE: Vibed\RE\player_move_pipeline.md "Where a gun's target comes from" and the notes after it):
//   - camera, by camera kind: the normal rooms' controller cCameraPlayer::Main 0x417A10 and the fixed / rail rooms'
//     cCameraRailBase::Main 0x428400 (six controller vtables) are wrapped in their vtable slots and their output
//     (cCameraBase +0x10 / +0x20 / +0x30) replaced, so their smoothing carries on from it and letting go glides back.
//     The boss cameras get it at the entry of uDevilCamera::sync 0x52EAC0 (uCamera position +0x30, up +0x40, target
//     +0x50), lent for the build and given back on return (they compute from their own previous position). A camera
//     of our own in sMediator::mpCamera would need a uDevilCamera-sized object (DebugCam's uFreeCamera is a uCamera).
//     Camera collision: the pivot -> camera line in COLL_CAMERA | COLL_CAMERASTOP;
//   - target: every gun's target comes from uPlayer::getLockOnTarget 0x820AE0 (vtbl 170): the body turn, the elevation
//     layer and the bullets (*(player + 0x2000) + 0x10). While aiming it returns a fake cLockOnTarget at the crosshair
//     ray's hit (COLL_SHELL); uPlayer::updatePtr drops it (no live parent), so the ticks put it back. Holding lock-on
//     hands everything back to the game (its lock-on code expects an actor behind the target);
//   - input: DebugCam's mouse deltas (sMain + 0x10384 -> +0x2C / +0x30), the buttons through GetAsyncKeyState (game in
//     front, dmc4_hook menu closed), the pad from mPeripheral (mPadBtnOn +0x1374, mAnlgR +0x13D4). The left button is
//     injected as the gun button in the pre-check tick (registered before WallRun's); its hold charges by writing the
//     charge input commands' state (mPlCmd 0x21 / 0x24 / 0x26) and setting the level's command bit on release. The
//     stick's world direction is rebuilt from the shoulder camera's yaw (the game's camera-cut latch dropped);
//   - body: a SpineAim provider twists spine joints 1 / 2 toward the aim against the facing, and the body turns only
//     by what exceeds body_turn_deg: uPlayer::setLockOnTargetAngle_2 0x820CE0 (the shots' and the gun pose's turn) gets
//     a turn point instead of the fake, and the post-action tick holds any other turn within the limit;
//   - gun pose: mPose.stat / holdTimer kept up while aiming, so the game's own checkPoseBegin / checkPoseRunBegin raise
//     the gun and walk in the strafing pose walk (lower 15-17, which faces the aim itself); its velocity and leg
//     motion are scaled by strafe_speed.
// Seen in-game: the game's yaw (atan2(x, z)) grows to the LEFT on screen, so mouse right lowers it and the right
// shoulder is (-cos, 0, sin).

namespace tps {

extern bool  mod_enabled;
extern float sensitivity;     // radians per mouse count
extern bool  invert_x, invert_y;
extern float distance;        // cm behind the pivot
extern float height;          // cm above the feet: the pivot
extern float shoulder;        // cm to the right of the pivot (negative = left shoulder)
extern float range;           // cm the aim ray reaches
extern float pitch_up_deg, pitch_down_deg;
extern bool  crosshair;
extern bool  toggle_aim;         // the right button toggles aiming instead of being held
extern bool  readout;            // the debug lines in the GUI
extern bool  charge_shots;       // holding the left button charges the gun (the charge input commands' state, written by hand)
extern bool  use_mouse, use_pad; // the inputs that aim
extern bool  cam_collision;      // the camera comes in before walls (pivot -> camera line, COLL_CAMERA | COLL_CAMERASTOP)
extern float collision_margin;   // cm kept between the camera and the wall
extern bool  gun_pose;           // aiming holds the game's gun pose (mPose.stat / holdTimer kept up; the pose walk strafes)
extern float strafe_speed;       // the pose walk's velocity and leg motion are scaled by this while aiming
extern uint32_t pad_combo;       // mPeripheral.mPadBtnOn bits that, all held, aim (captured in the GUI)
extern float pad_speed_deg;      // right stick: degrees per frame at full deflection
extern float pad_deadzone;       // right stick length (0-128) below which nothing turns
extern bool  pad_invert_x, pad_invert_y;
extern bool  spine_twist;        // the chest leads the aim (a SpineAim provider)
extern float body_turn_deg;      // the body turns only when the aim is further than this off its facing
extern float turn_speed_deg;     // degrees per frame the body turns while standing still
extern uintptr_t jmp_ret_lock;   // 0x820AE6
extern uintptr_t jmp_ret_turn;   // 0x820CE6
extern uintptr_t jmp_ret_sync;   // 0x52EAC6

bool __stdcall aim_camera(uintptr_t cam);   // the shoulder camera, from the uDevilCamera::sync hook (the boss cameras): lends their fields
void __stdcall give_back_camera();          // ... given back when sync returns
void __fastcall camera_player(uintptr_t ctl, uintptr_t edx);   // cCameraPlayer::Main's vtable slot: the normal rooms
void __fastcall camera_rail(uintptr_t ctl, uintptr_t edx);     // cCameraRailBase::Main's slots: the fixed / rail rooms
void detour_lock();

} // namespace tps

class ThirdPersonAim : public Mod {
public:
    ThirdPersonAim() = default;
    std::string get_mod_name() override { return "ThirdPersonAim"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook_sync, hook_lock, hook_turn, hook_pre, hook_post;
    std::unique_ptr<Patch> patch_player, patch_rail[6];
};
