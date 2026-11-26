// battle_snapshot - network-friendly battle snapshot for rollback netplay.
//
// This header is the cheap half: the constants, the wire records, and the API
// declarations. The capture / restore code and the zpp_bits wire templates live in
// BattleSnapshot.cpp, so a consumer that only calls the API compiles neither.
//
// Port of the Macro mod's training snapshot (src/mods/Macro.cpp: BattleEnemySnapshot /
// PlayerResourceSnapshot / PositionSnapshot, capture_enemy_snapshot / capture_player_resource_snapshot /
// capture_position_snapshot_unsafe and the apply_* twins) onto sUnit move lines and onto a
// pointer-free wire format, so a peer's battle state can be restored into a locally simulated
// one (GekkoNet GekkoSaveEvent / GekkoLoadEvent, see CoopOnline.cpp).
//
// Wire invariants (why a rollback restore is byte-exact across peers):
//   - no pointers, no std::vector, no struct padding: fixed-capacity records written field by
//     field in move-line order, so two peers that simulated identically agree byte-for-byte.
//   - unit identity on the wire is (move line, index in line order, unit id). Pointers are
//     re-bound when the state is applied and checked against the snapshot; they never travel.
//   - restore writes plain value fields only (floats / ints / bools), never pointers or object
//     graphs, so a restore cannot leave a dangling reference behind.
//
// Deliberately NOT in the snapshot (open work, not silently faked):
//   - collision groups / hitboxes (uCollisionMgr + cCollisionGroup): the collision pass
//     (sCollisionGame::checkInit / checkPushMain / checkAttackMain, sDevil4Collision::move)
//     rebuilds them every tick from the motion, so they are re-derived, not restored.
//   - effects, weapons, Devil Arms, Nero's stand, HUD / camera units: other sUnit lines.
//   - the full skeletal pose and skin matrices: root and per-joint MJOINT_WORK playback cursors
//     travel as byte offsets, but the evaluated joint transforms and skinning output do not (the
//     animation system rebuilds them from the restored cursors on the next tick).
//   - the sMain job system (sDevil4Main::mJobs): DeterministicSimulation forces it serial.
//   - any RNG the game keeps outside these units: none found in the sdk headers.
//   - state the move tick does not own. sDevil4Main::move keeps running after the block the
//     CoopOnline detour replaces (0x8AF04C onwards: the sound/camera/mediator/score systems),
//     so anything those advance runs once per PRESENTED frame, not once per simulated frame.
//     Snapshotting such a field makes every re-simulated frame pack the value from
//     check_distance frames earlier, which is a guaranteed desync and makes the rollback
//     restore stale values. Verified in a TTD trace: mediator.bpTimer read 7196.399 at frame 2
//     (live) and 7197.599 in the frame-2 re-run - exactly one presenting frame's 1.2 step.
//     Out: sMediator::bpTimer (BP countdown), uPlayer::releaseTimer, uPlayer::guardReq1/2.
//   - the model's shape and evaluated joints: the motion sampler travels (uModel::mBlendNum, the
//     blending MotionArray slots with their root and per-joint cursor offsets, and uActor's
//     now/old frame pose). The tick reads that to write a unit's position; the evaluated joint
//     transforms come back from uModel::updateWorldMatrix on the next tick.
#pragma once

#include <cstdint>

#include "sdk/MtMath.hpp"
#include "sdk/sUnit.hpp"
#include "sdk/sDevil4Main.hpp"

namespace netplay_snapshot {

// ---------------------------------------------------------------- identity

constexpr int kLinePlayer = (int)sUnit::ML_PLAYERS;   // 13, Coop.cpp kLinePlayer
constexpr int kLineEnemy  = (int)sUnit::ML_ENEMIES;   // 15, the chain Macro.cpp walked
constexpr int kLineCamera = 0x17;                     // the game camera, Coop.cpp kLineCamera

constexpr uint32_t kMaxPlayers = 8;    // Coop.cpp spawns up to 4 players; headroom for extras
constexpr uint32_t kMaxEnemies = 64;   // Macro.cpp scanned 128; 64 caps the worst-case rollback
                                       // state at ~4 MiB (see below)

constexpr uint32_t kWireMagic   = 0x444D4334u;   // "DMC4"
// 14 removed the derived lock-on cache; 15 adds the per-joint sampler cursors that each motion
// slot carries now that the rollback window has to reproduce joint playback, not just the root.
constexpr uint32_t kWireVersion = 16u;   // 16: pad38 / prev_trans[0] (live pointers) left the wire

// uModel::MotionArray slots carried per unit. Root motion is computed from them, so a restore
// that leaves them at the live frame replays the wrong translation delta (TTD: the position a
// re-simulated tick writes equals uActor::mNowFramePos, which the unrestored sampler recomputes).
constexpr uint32_t kMaxMotionSlots = 8u;   // sizeof(uModel::MotionArray) / sizeof(Motion)
constexpr uint32_t kMaxMotionJoints = 192u; // UModelJointArr capacity is 200

// The player line carries other units too, so the walk only takes uPlayerNero / uPlayerDante
// (ChargeChecker.cpp, CoopContext.cpp).
constexpr uintptr_t kNeroVtbl  = 0x00BE4FA0;
constexpr uintptr_t kDanteVtbl = 0x00BE3C50;

constexpr uint32_t kMaxLineWalk = 8192;   // Coop.cpp / ChargeChecker.cpp line-walk guard

// ---------------------------------------------------------------- records

struct TimingState {
    uint64_t sim_time_us = 0;         // CoopOnline g_simTimeUs, the lockstep clock
    uint64_t main_timer = 0;          // sDevil4Main::mTimer
    int32_t frame_timer = 0;          // sDevil4Main::mFrameTimer
    float fps = 0.0f;
    float fps_lock = 0.0f;
    float fps_actual = 0.0f;
    uint32_t wanna_skip_frame = 0;
    uint32_t skip = 0;                // sDevil4Main::mSkip, the frame-advance counter
    uint8_t pause_flags = 0;          // mPause / mUnpause / mJobInit / mFinalize
    float delta_time_norm = 0.0f;
    float delta_time_max = 0.0f;
    float delta_time_max2 = 0.0f;
    float speed_global = 0.0f;
    float delay_frame = 0.0f;
};

struct MediatorState {
    uint32_t mission_id = 0;
    uint32_t room_id = 0;
    uint32_t enemy_count[3] = {};
    uint32_t orb_count = 0;
    uint32_t orb_mission_current = 0;
    uint32_t orb_mission_potential = 0;
    uint32_t hell_gold_orb_count = 0;
    uint32_t game_difficulty = 0;
    uint32_t game_mode = 0;
    float enemy_difficulty = 0.0f;
    float style_points = 0.0f;
    // No bp_timer: the BP countdown is stepped once per presented frame outside the tick.
    uint8_t unlock_flags = 0;         // hasUnlockedDT / AirHike / Sprint / Hold / MaxAct
    uint8_t disaster_gauge_display = 0;
    uint8_t turbo_enabled = 0;
    // sWorkRate: hitstop and slow motion, devil4_sdk::get_work_rate()
    float game_speed = 0.0f;
    float work_unk2 = 0.0f;
    float global_speed = 0.0f;
    float player_speed = 0.0f;
    float enemy_speed = 0.0f;
    float room_speed = 0.0f;
    float turbo_speed = 0.0f;
    float work_unk3 = 0.0f;
    uint32_t stylish_tier = 0;        // sStylishCount::current_style_tier
};

struct CameraState {
    bool camera_valid = false;
    bool player_camera_valid = false;
    MtVector3 camera_position{}, camera_up{}, camera_lookat{};
    MtVector3 camera_old_position{}, camera_old_lookat{};
    float near_plane = 0.0f, far_plane = 0.0f, aspect = 0.0f, fov = 0.0f, old_fov = 0.0f;
    float roll = 0.0f, old_roll = 0.0f;
    int32_t transition_frame = 0;
    float transition_timer = 0.0f;
    // cCameraPlayer's follow-camera parameters (mLen/mRotX/mRotY/mHoseiY, battle FOV defaults)
    // are private in the reclass dump, so they stay out of the snapshot; cCameraBase's public
    // fields cover the camera transform itself.
    MtVector3 player_camera_position{}, player_camera_lookat{}, player_camera_up{};
    float player_camera_near_plane = 0.0f, player_camera_far_plane = 0.0f;
    float player_camera_aspect = 0.0f, player_camera_fov = 0.0f, player_camera_roll = 0.0f;
};

// uModel::Motion (uActor.hpp:190) minus its pointers: the sampler state that produces the root
// translation/rotation the move tick writes into the unit. Asset pointers stay local. The two
// root keyframe cursors are byte offsets from their TrackHeader's BufferOfs (pparam + 0xC),
// because pcur_param is playback state, not a rebinding-only pointer.
constexpr uint32_t kNoMotionTrack = 0xFFFFFFFFu;
constexpr uint32_t kInvalidMotionCursor = 0xFFFFFFFEu;

struct JointCursorState {
    // No in-class defaults: BattleSnapshot owns 64 x 8 x 192 of these, and a
    // default constructor that stamps kNoMotionTrack on every cursor is another
    // 10^6 stores that pushes cl.exe out of its heap (/ZI /RTC1, "C1060").
    // capture_motion_cursor writes all three offsets for every cursor it walks, and
    // BattleSnapshot's constructor memsets the rest, so an untouched cursor is 0.
    uint32_t quat_offset, trans_offset;
    uint32_t scale_offset;
    float quat_frame = 0.0f, trans_frame = 0.0f, scale_frame = 0.0f;
    float quat_weight = 0.0f, trans_weight = 0.0f, scale_weight = 0.0f;
};

struct MotionSlotState {
    uint16_t motion_no = 0;
    uint16_t reserved = 0;            // Motion::mReserved
    uint16_t state = 0;
    uint16_t attr = 0;
    float inter_frame = 0.0f, inter_count = 0.0f, blend = 0.0f;
    float frame = 0.0f, frame_max = 0.0f, loop_frame = 0.0f, speed = 0.0f;
    float inter_rate = 0.0f, next_frame = 0.0f;
    float inter_curve[2] = {};
    // pad38[0..1] and prev_trans[0] hold live per-process pointers inside the game's Motion
    // block (the netdump cross-machine diff was exactly those words). They do not travel and
    // capture/apply never touch them - the ctor leaves them zero. See wire(MotionSlotState).
    int32_t pad38[2] = {};
    float prev_quat[4] = {}, base_quat[4] = {}, null_quat[4] = {};
    float prev_trans[3] = {}, base_trans[3] = {}, null_trans[3] = {};   // [0] not on the wire
    float trans_frame = 0.0f, trans_weight = 0.0f, quat_frame = 0.0f, quat_weight = 0.0f;
    uint32_t trans_cursor_offset = kNoMotionTrack, quat_cursor_offset = kNoMotionTrack;
    int32_t sequence[2] = {}, prev_sequence[2] = {};
    JointCursorState joints[kMaxMotionJoints];
};

// The model half of a unit: uActor's root frame pair plus the motion slots the unit is blending.
// uActor : uDevil4Model : uModel : uCoord, all at the same address as the unit, so the unit
// pointer is also the model pointer (verified on uPlayerNero and uEm000 units).
struct ModelState {
    uint8_t motion_count = 0;         // uModel::mBlendNum, clamped to kMaxMotionSlots
    uint16_t joint_count = 0;         // uModel::mJointNum; capture REFUSES a model with more
                                      // than kMaxMotionJoints joints (a save that cannot be
                                      // restored must never be written)
    MtVector3 now_frame_pos{}, old_frame_pos{};
    MtVector4 now_frame_quat{}, old_frame_quat{};
    // uModel::mContinueCount plus the 15 bytes the SDK declares as padding0 right after it (the
    // block sits immediately before mPrevWmat). Our code does not read them; they travel because
    // they are the only uModel bytes between the coord transform and the world matrix that a
    // per-frame motion update can write, and the one window that froze the sampler had a
    // root-motion delta of exactly 0 with every captured sampler field still matching.
    uint32_t continue_words[4] = {};
    MotionSlotState motion[kMaxMotionSlots];
};

// uEm000Base::checkBattle (0x55DF30) sets inBattle from the unit's own status/motion, the
// cLockOnTarget block at +0x1D80 and the flag at +0x28D6. None of that travels in the unit record,
// so a rewind can flip inBattle (and restart the motion, which rewrites base/prev/null trans and
// the position) where the original pass did not.
constexpr uintptr_t kActorStatusOffset = 0xEA8;   // u32, bit 0x80 short-circuits checkBattle
constexpr uintptr_t kBattleModeOffset = 0x1500;   // u32, compared against 2
constexpr uintptr_t kBattleFlagOffset = 0x28D6;   // u8, gate on the target test
constexpr uintptr_t kLockOnOffset = 0x1D80;       // cLockOnTarget
constexpr uintptr_t kEnemyTargetOffset = 0x14B4;  // mpTargetEnemy, direct [edx+14B4h] at 0x7360C0
constexpr uintptr_t kEnemyTargetPos = 0x14D0;
constexpr uintptr_t kActorVelocity = 0xEC0;
constexpr uintptr_t kActorAcceleration = 0xED0;
constexpr uintptr_t kEnemyDtOk = 0x148C;
constexpr uintptr_t kEnemyDtTimerEnable = 0x148D;
constexpr uintptr_t kEnemyDt = 0x148E;
constexpr uintptr_t kEnemyDtSign = 0x148F;
constexpr uintptr_t kEnemyTickTimer = 0x1458;     // float, +delta per tick (uEm000Base)
// uCoord::mQuat is a 4-float quaternion but the layout exposes rotation as xyz at 0x40, so its w
// sits in the 4 bytes the SDK calls pad_4c. uEm000Base::moveEx passes &mQuat.x to
// uCollisionMgr::updatePosQuat, which consumes all four components.
constexpr uintptr_t kEnemyQuatW = 0x4C;

// uEm000Base's per-unit collision manager. uEm000Base::moveEx reads [unit+0x1670] as
// mCollMgr.mCorrectPos and writes [unit+0x30] as mPos; offsetof(uCollisionMgr, mCorrectPos) is
// 0x70 and sizeof(uCollisionMgr) is 0x3F0, so the block starts at 0x1600. The carried range
// 0x70..0xCB (mCorrectPos, mCorrectMat, mAttackCorrectPos) is floats only - the rest of the
// struct holds model pointers that must not be written back. The manager's address in the enemy
// object is enemy-ID-specific and is resolved through EnemyTracker, never assumed to be 0x1600.
constexpr uintptr_t kCollCorrectOffset = 0x70;
constexpr uintptr_t kCollLandOffset = 0x1C;
constexpr uintptr_t kCollFlagsOffset = 0x3C;      // mFall / mIsBoss / mIsDevilTrigger
constexpr uint32_t kCollCorrectWords = 23u;       // 92 bytes: 0x70..0xCB

struct BattleGateState {
    uint32_t actor_status = 0;
    uint32_t mode = 0;
    uint8_t battle_flag = 0;
    // uEnemy::getTargetEnemyPos (0x7360C0) resolves the target through uActor* at +0x14B4 and
    // sMediator+0x24, then copies that unit's mPos into the cLockOnTarget block. The pointer cannot
    // travel, so the target is carried as an identity: 0 = none, 1 = a player (by unique pad id),
    // 0xFF = something else (left untouched).
    uint8_t target_kind = 0xFF;
    uint8_t target_player = 0;
    float lockon_pos[3] = {};
    float lockon_range = 0.0f, lockon_max_hp = 0.0f, lockon_hp = 0.0f, lockon_pi_length = 0.0f;
    uint8_t lockon_is_in = 0, lockon_is_target = 0, lockon_is_catch = 0, lockon_camera_in = 0;
    uint8_t lockon_no_target = 0, lockon_inter_target = 0, lockon_no_inter = 0;
};

struct PlayerState {
    uint32_t controller_id = 0;       // uPlayer::controllerID (+0x1494), 0 Dante / 1 Nero
    uint8_t pad_id = 0;               // uPlayer::padID (+0x1405), the coop control number
    bool is_active = false;           // uPlayer::isActive (+0x1509)
    uint32_t unit_flags = 0;          // cUnit::flags.raw: beFlag / moveLine / unitAttr / trans
    float unit_delta_time = 0.0f;     // cUnit::m_delta_time

    MtVector3 position{};             // uCoord::mPos
    MtVector3 velocity{};             // uPlayer::m_d_velocity
    float rotation2 = 0.0f, rotation3 = 0.0f, facing_direction = 0.0f;
    float ground_inertia_x = 0.0f, inertia_y = 0.0f, ground_inertia_z = 0.0f;
    float ground_inertia = 0.0f, inertia = 0.0f, inertia_direction_real = 0.0f;
    float vel_magnitude = 0.0f, direction_of_magnitude = 0.0f;

    uint16_t anim_id = 0;
    float anim_frame = 0.0f, anim_frame_max = 0.0f;
    uint32_t move_bank = 0, move_part = 0;
    uint32_t move_id2 = 0, move_id3 = 0, move_id_best = 0, walk_type = 0;
    uint8_t grounded = 0, grounded2_flag = 0;      // uPlayer::grounded (+0xEA8) / grounded2 (+0x2008)
    uint8_t air_attack_count = 0, air_hike_count = 0, weight = 0;
    bool can_air_hike = false, can_wall_hike = false;
    float jc_timer = 0.0f, round_trip_charge = 0.0f, character_speed = 0.0f;
    bool hitstop = false;
    float hitstop_timer = 0.0f;

    // uPlayer::damageStruct (uDamage_Old)
    float hp = 0.0f, hp_max = 0.0f, hp_taken = 0.0f, prev_damage_resist = 0.0f;
    int32_t stun[5] = {}, displacement[5] = {};
    bool is_dead = false;
    float stun_reset_timer = 0.0f;

    // resources
    int32_t current_style = 0;
    float dt = 0.0f, max_dt = 0.0f;
    bool dt_active = false, dt_outfit = false;
    uint8_t exceed_level = 0;
    float exceed_timer = 0.0f;
    bool can_exceed = false, is_exceeding = false;
    float guard_timer = 0.0f;
    float revenge_gauge = 0.0f, disaster_gauge = 0.0f;
    int32_t dreadnaught = 0;
    int32_t next_sword = 0;
    uint32_t next_gun = 0, current_sword = 0, current_gun = 0;
    uint32_t can_weapon_change = 0, is_lockon_animation = 0;
    bool locked_on = false;

    // input records: the game rewrites them from the pad every tick, they are carried so the
    // checksum sees them and a rollback starts from the same values it confirmed.
    uint8_t input_hold[4] = {}, input_press[4] = {}, input_release[4] = {};
    uint16_t button_input_raw = 0, stick_input_raw = 0;
    float stick_direction = 0.0f;
    bool stick_past_threshold = false;
    uint8_t buffered_jump = 0;
    bool tilt_forward = false, tilt_back = false, tilt_back_forward = false;

    // cancel windows, recomputed with the motion but part of the tick's state
    uint8_t cancels1[4] = {};
    uint32_t cancels2 = 0;
    uint8_t melee_cancel_permissions1[4] = {}, jump_cancel_permissions1[4] = {};
    uint8_t melee_cancel_permissions2 = 0, jump_cancel_permissions2 = 0, gun_cancel_permissions2 = 0;
    uint8_t buffer_permissions = 0, directional_melee_permissions = 0;
    bool trigger_big_jump = false, trigger_stinger_jump = false;

    ModelState model;                 // root motion: what the move tick writes into mPos
};

struct EnemyState {
    int32_t id = -1;                  // uEnemy_Old::ID (+0x1410), ENEMY_ID
    int32_t team = 0;
    bool is_active = false;
    bool in_battle = false;
    uint32_t unit_flags = 0;          // cUnit::flags.raw
    float unit_delta_time = 0.0f;
    uint8_t move_id = 0, move_part = 0;   // uEnemy_Old::moveID (+0x14) / movePart (+0x15)

    MtVector3 position{}, velocity{}, rotation{}, scale{};
    MtVector3 actor_velocity{}, actor_acceleration{};
    MtVector3 target_position{};
    float quat_w = 0.0f;              // uCoord::mQuat.w (see kEnemyQuatW)
    MtVector3 knockback_position{}, knockback_rotation{};
    uint16_t anim_id = 0;
    float anim_frame = 0.0f, anim_frame_max = 0.0f;

    bool dt = false;                  // uEnemy::mDevilTrigger (+0x148E)
    uint8_t dt_ok = 0, dt_timer_enabled = 0, dt_sign = 0;
    float dt_timer = 0.0f;

    float berial_fire = 0.0f, berial_fire_timer = 0.0f;
    float gladius_timer = 0.0f;
    bool gladius_buried = false;
    float faust_cloak = 0.0f, faust_cloak_timer = 0.0f;
    float blitz_electric = 0.0f, blitz_electric_timer = 0.0f, blitz_electric_suicide_timer = 0.0f;
    float angelo_shield = 0.0f;
    float credo_shield = 0.0f, credo_shield_timer = 0.0f;
    float sanctus_shield_m11 = 0.0f, sanctus_shield_timer_m11 = 0.0f;
    float sanctus_shield_m20 = 0.0f, sanctus_shield_timer_m20 = 0.0f;
    int32_t frost_heal_count = 0;

    // uDamage_Old at enemy + EnemyTracker::get_enemy_specific_damage_offset(id); damage_valid
    // is false for ids the table has no slot for, and their HP is not restored.
    bool damage_valid = false;
    float hp = 0.0f, hp_max = 0.0f, hp_taken = 0.0f, prev_damage_resist = 0.0f;
    int32_t stun[5] = {}, displacement[5] = {};
    bool is_dead = false;
    float stun_reset_timer = 0.0f;

    // uEm000Base::moveEx (0x543DA0) does not integrate the position, it REBUILDS it from the
    // per-unit collision manager:
    //   this->mPos += this->mCollMgr.mCorrectPos;
    //   uCollisionMgr::checkScr(1, &this->mCollMgr, mat);          // matrix from mCollMgr
    //   uCollisionMgr::updatePosQuat(mat, &this->mPos, &this->mQuat.x);   // mPos = mat * pos
    // so a mCollMgr that was not rewound pushes the unit straight back to the pose it held when
    // the rollback started - which is exactly the frozen value observed in every re-run save.
    // Layout verified against the disassembly ([ebx+1670h] = mCorrectPos, [ebx+30h] = mPos) and
    // against the SDK (offsetof(uCollisionMgr, mCorrectPos) == 0x70, sizeof == 0x3F0).
    // Only the pointer-free range is carried: mCorrectPos, mCorrectMat, mAttackCorrectPos.
    bool coll_valid = false;
    uint32_t coll_land = 0;           // mLand / mNoLand / mMoveLand / mWall (+0x1C)
    uint32_t coll_flags = 0;          // mFall / mIsBoss / mIsDevilTrigger (+0x3C)
    uint32_t coll_correct[kCollCorrectWords] = {};

    ModelState model;                 // root motion: enemy translation comes from the sampler
    BattleGateState gate;             // what checkBattle reads to decide inBattle
    // uEm000Base +0x1458: a float accumulator the tick advances by the unit delta (1.2) per tick.
    // Found by diffing the whole enemy object at frame 16's original save against its rollback
    // re-save - the only bytes that differed were this field (46.8002815 vs 38.4002762 = 1.2 x 39
    // vs 1.2 x 32). Restoring it is what stops the motion drifting a little further every cycle.
    float tick_timer = 0.0f;
};

// MtRandom (0x00EAC6B0, see sdk/MtRandom.hpp for the layout and the non-standard seeding): the
// game's one global generator, seeded once per stage and consumed inside the tick. That makes its
// state part of the simulation - a re-simulated frame has to draw the same numbers the original
// pass drew, or the two diverge on the first call, no matter how exact the unit state is.
constexpr uintptr_t kGameRngAddress = 0x00EAC6B0;
constexpr uintptr_t kGamePadAddress = 0x00E559C4;
constexpr uint32_t kRngWords = 624;

struct RngState {
    int32_t mti = 0;
    uint32_t mt[kRngWords] = {};
};

struct PadState {
    uint32_t buttons[4] = {};         // on / trg / rel / rep after the tick consumed them
    float analog[4] = {};             // left x/y, right x/y
};

// The whole rollback state. Every member is POD so the record is trivially copyable;
// the ctor (declared here, defined in BattleSnapshot.cpp) clears it with one memset:
// an implicit ctor value-initializes ~10^6 members - 64 enemies x 8 motion slots x 192
// joint cursors - and under /ZI /RTC1 that costs seconds and gigabytes in every TU
// that owns such an object (Devil4State does), which is where the compiler's
// "fatal error C1060: out of heap space" came from. magic/version are stamped back
// after the clear, so a default-constructed snapshot is a valid empty battle.
struct BattleSnapshot {
    uint32_t magic;                   // = kWireMagic, stamped by the ctor
    uint32_t version;                 // = kWireVersion, stamped by the ctor
    TimingState timing;
    MediatorState mediator;
    RngState rng;
    CameraState camera;
    PadState pad;
    uint32_t player_count;            // filled in by capture_battle
    uint32_t enemy_count;
    PlayerState players[kMaxPlayers];
    EnemyState enemies[kMaxEnemies];
    BattleSnapshot();                 // BattleSnapshot.cpp: one memset
};

// ---------------------------------------------------------------- the API (BattleSnapshot.cpp)
//
// One entry point per direction. The zpp_bits wire() field lists that define the byte
// layout, and the capture / restore code that walks the game objects, are defined in
// BattleSnapshot.cpp; including this header costs neither. Failure is reported through
// false plus a `reason` string literal.
//
// Worst case bytes: kMaxPlayers + kMaxEnemies units, every slot filled, every slot
// carrying a joint cursor per joint. Each JointCursorState is 36 wire bytes (3 u32
// cursors + 6 floats), so the envelope is ~4 MiB. Feed the result to
// GekkoConfig::state_size; the scratch itself is only the sizing probe.
constexpr uint32_t kWireScratchBytes = 8u * 1024u * 1024u;

uint32_t state_bytes();

bool capture_battle(sDevil4Main* main, uint64_t sim_time_us, BattleSnapshot& out,
    const char*& reason);
bool restore_battle(sDevil4Main* main, const BattleSnapshot& snap, uint64_t& sim_time_us,
    const char*& reason);

bool pack_into(unsigned char* buffer, uint32_t capacity, uint32_t& written,
    BattleSnapshot& snap, const char*& reason);
bool unpack_from(const unsigned char* buffer, uint32_t length, BattleSnapshot& snap,
    const char*& reason);

// The checksum covers the packed bytes with the derived copies cleared: the lock-on
// numbers are another unit's state and the camera is stepped once per presented frame.
void scrub_scratch_gate(BattleGateState& g);
void scrub_scratch_camera(CameraState& c);
// Zeros the per-process garbage the wire still carries inside the motion blocks: the uModel
// continue_words and every slot's quat/trans blocks. The netdump cross-machine diff showed
// those are heap words and denormals (not quaternion data) while the sampler scalars, joint
// cursors and active-slot data match everywhere. Checksum copy only - the ring copy keeps
// the live bytes for restore.
void scrub_desync_noise(BattleSnapshot& snap);

uint32_t checksum_of(const unsigned char* bytes, uint32_t length);

} // namespace netplay_snapshot
