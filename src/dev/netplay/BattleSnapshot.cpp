// BattleSnapshot.cpp - capture / restore / wire format for the rollback snapshot.
//
// This is the whole cost centre of the netplay snapshot: the zpp_bits wire templates
// (a field list per record, walked by pack_into / unpack_from), the capture and apply
// code for every record, and the construction of the ~4 MiB BattleSnapshot record tree.
// It lives in its own translation unit so that consumers which only call the API
// declared in BattleSnapshot.hpp (CoopOnline.cpp) compile neither the templates nor the
// record-tree constructor. That is what keeps cl.exe off the multi-gigabyte path:
// the implicit value-init of BattleSnapshot writes every one of its ~10^6 members
// (64 enemies x 8 motion slots x 192 joint cursors x 9 fields), and with /ZI /RTC1 the
// generated function took cl.exe past its heap ("fatal error C1060: compiler is out of
// heap space"). The default constructor below zeroes the same bytes with one memset,
// and the records keep no in-class defaults on the deep arrays, so nothing else in
// either direction has to walk them.

#include "dev/netplay/BattleSnapshot.hpp"

#include <array>
#include <cstring>
#include <memory>
#include <span>

#include "sdk/MtMath.hpp"
#include "sdk/cUnit.hpp"
#include "sdk/sUnit.hpp"
#include "sdk/sDevil4Main.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/sWorkRate.hpp"
#include "sdk/sStylishCount.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/uActor.hpp"
#include "sdk/uDamage.hpp"
#include "sdk/Cam.hpp"
#include "sdk/Pad.hpp"
#include "sdk/Devil4.hpp"
#include "mods/EnemyTracker.hpp"

#include "zpp_bits.h"

namespace netplay_snapshot {

// Default ctor: one memset instead of an implicit one. Walking the ~10^6 members below
// (every cursor of every slot of every unit) costs seconds and gigabytes under /ZI /RTC1,
// and the in-class defaults on the deep arrays are gone for the same reason.
// magic/version are the wire header, so the ctor restores them after clearing.
BattleSnapshot::BattleSnapshot()
{
    std::memset(this, 0, sizeof *this);
    magic = kWireMagic;
    version = kWireVersion;
}

// ---------------------------------------------------------------- wire format
// One field list per record, used by both directions, so the reader and the writer cannot
// drift apart. Padding never reaches the wire, which keeps the checksum stable.

template <class Archive>
constexpr auto wire(TimingState& s, Archive& archive)
{
    return archive(s.sim_time_us, s.main_timer, s.frame_timer, s.fps, s.fps_lock, s.fps_actual,
        s.wanna_skip_frame, s.skip, s.pause_flags, s.delta_time_norm, s.delta_time_max,
        s.delta_time_max2, s.speed_global, s.delay_frame);
}

template <class Archive>
constexpr auto wire(MediatorState& s, Archive& archive)
{
    return archive(s.mission_id, s.room_id, s.enemy_count[0], s.enemy_count[1], s.enemy_count[2],
        s.orb_count, s.orb_mission_current, s.orb_mission_potential, s.hell_gold_orb_count,
        s.game_difficulty, s.game_mode, s.enemy_difficulty, s.style_points,
        s.unlock_flags, s.disaster_gauge_display, s.turbo_enabled, s.game_speed, s.work_unk2,
        s.global_speed, s.player_speed, s.enemy_speed, s.room_speed, s.turbo_speed, s.work_unk3,
        s.stylish_tier);
}

template <class Archive>
constexpr auto wire(CameraState& s, Archive& archive)
{
    return archive(s.camera_valid, s.camera_position.x, s.camera_position.y, s.camera_position.z,
        s.camera_up.x, s.camera_up.y, s.camera_up.z, s.camera_lookat.x, s.camera_lookat.y,
        s.camera_lookat.z, s.camera_old_position.x, s.camera_old_position.y, s.camera_old_position.z,
        s.camera_old_lookat.x, s.camera_old_lookat.y, s.camera_old_lookat.z, s.near_plane,
        s.far_plane, s.aspect, s.fov, s.old_fov, s.roll, s.old_roll, s.transition_frame,
        s.transition_timer, s.player_camera_valid, s.player_camera_position.x,
        s.player_camera_position.y, s.player_camera_position.z, s.player_camera_lookat.x,
        s.player_camera_lookat.y, s.player_camera_lookat.z, s.player_camera_up.x,
        s.player_camera_up.y, s.player_camera_up.z, s.player_camera_near_plane,
        s.player_camera_far_plane, s.player_camera_aspect, s.player_camera_fov,
        s.player_camera_roll);
}

template <class Archive>
constexpr auto wire(JointCursorState& j, Archive& archive)
{
    return archive(j.quat_offset, j.trans_offset, j.scale_offset,
        j.quat_frame, j.trans_frame, j.scale_frame,
        j.quat_weight, j.trans_weight, j.scale_weight);
}

template <class Archive>
constexpr auto wire(MotionSlotState& s, Archive& archive)
{
    // pad38[0..1] and prev_trans[0] do not travel: capture shows the game's Motion block holds
    // live per-process pointers there (the netdump cross-machine diff was exactly those two
    // words - a heap pointer at +0x38 and one at +0x6C), and a rollback restore must never
    // write a stale captured pointer over the live one. They stay in the struct zeroed; the
    // game keeps binding its own Motion to whatever it points at.
    return archive(s.motion_no, s.reserved, s.state, s.attr, s.inter_frame, s.inter_count, s.blend,
        s.frame, s.frame_max, s.loop_frame, s.speed, s.inter_rate, s.next_frame,
        s.inter_curve[0], s.inter_curve[1],
        s.prev_quat[0], s.prev_quat[1], s.prev_quat[2], s.prev_quat[3],
        s.base_quat[0], s.base_quat[1], s.base_quat[2], s.base_quat[3],
        s.null_quat[0], s.null_quat[1], s.null_quat[2], s.null_quat[3],
        s.prev_trans[1], s.prev_trans[2],
        s.base_trans[0], s.base_trans[1], s.base_trans[2],
        s.null_trans[0], s.null_trans[1], s.null_trans[2],
        s.trans_frame, s.trans_weight, s.quat_frame, s.quat_weight,
        s.trans_cursor_offset, s.quat_cursor_offset,
        s.sequence[0], s.sequence[1], s.prev_sequence[0], s.prev_sequence[1]);
}

template <class Archive>
constexpr auto wire(ModelState& m, Archive& archive)
{
    auto result = archive(m.motion_count, m.joint_count, m.now_frame_pos.x, m.now_frame_pos.y,
        m.now_frame_pos.z, m.old_frame_pos.x, m.old_frame_pos.y, m.old_frame_pos.z,
        m.now_frame_quat.x, m.now_frame_quat.y, m.now_frame_quat.z, m.now_frame_quat.w,
        m.old_frame_quat.x, m.old_frame_quat.y, m.old_frame_quat.z, m.old_frame_quat.w,
        m.continue_words[0], m.continue_words[1], m.continue_words[2], m.continue_words[3]);
    if (zpp::bits::failure(result)) {
        return result;
    }
    if (m.motion_count > kMaxMotionSlots || m.joint_count > kMaxMotionJoints) {
        return zpp::bits::errc{std::errc::invalid_argument};
    }
    // only the slots the unit is blending travel; the reader takes the counts from the wire, so
    // the writer and the reader walk the same number of records.
    for (uint32_t i = 0; i < m.motion_count; ++i) {
        result = wire(m.motion[i], archive);
        if (zpp::bits::failure(result)) {
            return result;
        }
        for (uint32_t j = 0; j < m.joint_count; ++j) {
            result = wire(m.motion[i].joints[j], archive);
            if (zpp::bits::failure(result)) {
                return result;
            }
        }
    }
    return zpp::bits::errc{};
}

template <class Archive>
constexpr auto wire(PlayerState& p, Archive& archive)
{
    auto result = archive(p.controller_id, p.pad_id, p.is_active, p.unit_flags, p.unit_delta_time,
        p.position.x, p.position.y, p.position.z, p.velocity.x, p.velocity.y, p.velocity.z,
        p.rotation2, p.rotation3, p.facing_direction, p.ground_inertia_x, p.inertia_y,
        p.ground_inertia_z, p.ground_inertia, p.inertia, p.inertia_direction_real,
        p.vel_magnitude, p.direction_of_magnitude, p.anim_id, p.anim_frame, p.anim_frame_max,
        p.move_bank, p.move_part, p.move_id2, p.move_id3, p.move_id_best, p.walk_type,
        p.grounded, p.grounded2_flag, p.air_attack_count, p.air_hike_count, p.weight,
        p.can_air_hike, p.can_wall_hike, p.jc_timer, p.round_trip_charge,
        p.character_speed, p.hitstop, p.hitstop_timer, p.hp, p.hp_max, p.hp_taken,
        p.prev_damage_resist, p.stun[0], p.stun[1], p.stun[2], p.stun[3], p.stun[4],
        p.displacement[0], p.displacement[1], p.displacement[2], p.displacement[3],
        p.displacement[4], p.is_dead, p.stun_reset_timer, p.current_style, p.dt, p.max_dt,
        p.dt_active, p.dt_outfit, p.exceed_level, p.exceed_timer, p.can_exceed, p.is_exceeding,
        p.guard_timer, p.revenge_gauge, p.disaster_gauge,
        p.dreadnaught, p.next_sword, p.next_gun, p.current_sword, p.current_gun,
        p.can_weapon_change, p.is_lockon_animation, p.locked_on, p.input_hold[0],
        p.input_hold[1], p.input_hold[2], p.input_hold[3], p.input_press[0], p.input_press[1],
        p.input_press[2], p.input_press[3], p.input_release[0], p.input_release[1],
        p.input_release[2], p.input_release[3], p.button_input_raw, p.stick_input_raw,
        p.stick_direction, p.stick_past_threshold, p.buffered_jump, p.tilt_forward, p.tilt_back,
        p.tilt_back_forward, p.cancels1[0], p.cancels1[1], p.cancels1[2], p.cancels1[3],
        p.cancels2, p.melee_cancel_permissions1[0], p.melee_cancel_permissions1[1],
        p.melee_cancel_permissions1[2], p.melee_cancel_permissions1[3],
        p.jump_cancel_permissions1[0], p.jump_cancel_permissions1[1], p.jump_cancel_permissions1[2],
        p.jump_cancel_permissions1[3], p.melee_cancel_permissions2, p.jump_cancel_permissions2,
        p.gun_cancel_permissions2, p.buffer_permissions, p.directional_melee_permissions,
        p.trigger_big_jump, p.trigger_stinger_jump);
    if (zpp::bits::failure(result)) {
        return result;
    }
    return wire(p.model, archive);
}

template <class Archive>
constexpr auto wire(BattleGateState& g, Archive& archive)
{
    return archive(g.actor_status, g.mode, g.battle_flag, g.target_kind, g.target_player,
        g.lockon_pos[0], g.lockon_pos[1], g.lockon_pos[2], g.lockon_range, g.lockon_max_hp,
        g.lockon_hp, g.lockon_pi_length, g.lockon_is_in, g.lockon_is_target, g.lockon_is_catch,
        g.lockon_camera_in, g.lockon_no_target, g.lockon_inter_target, g.lockon_no_inter);
}

template <class Archive>
constexpr auto wire(EnemyState& e, Archive& archive)
{
    auto result = archive(e.id, e.team, e.is_active, e.in_battle, e.unit_flags, e.unit_delta_time,
        e.move_id, e.move_part, e.position.x, e.position.y, e.position.z, e.velocity.x,
        e.velocity.y, e.velocity.z, e.actor_velocity.x, e.actor_velocity.y, e.actor_velocity.z,
        e.actor_acceleration.x, e.actor_acceleration.y, e.actor_acceleration.z,
        e.target_position.x, e.target_position.y, e.target_position.z,
        e.rotation.x, e.rotation.y, e.rotation.z, e.scale.x,
        e.scale.y, e.scale.z, e.quat_w, e.knockback_position.x, e.knockback_position.y,
        e.knockback_position.z, e.knockback_rotation.x, e.knockback_rotation.y,
        e.knockback_rotation.z, e.anim_id, e.anim_frame, e.anim_frame_max,
        e.dt, e.dt_ok, e.dt_timer_enabled, e.dt_sign, e.dt_timer,
        e.berial_fire, e.berial_fire_timer, e.gladius_timer, e.gladius_buried, e.faust_cloak,
        e.faust_cloak_timer, e.blitz_electric, e.blitz_electric_timer,
        e.blitz_electric_suicide_timer, e.angelo_shield, e.credo_shield, e.credo_shield_timer,
        e.sanctus_shield_m11, e.sanctus_shield_timer_m11, e.sanctus_shield_m20,
        e.sanctus_shield_timer_m20, e.frost_heal_count, e.damage_valid, e.hp, e.hp_max,
        e.hp_taken, e.prev_damage_resist, e.stun[0], e.stun[1], e.stun[2], e.stun[3], e.stun[4],
        e.displacement[0], e.displacement[1], e.displacement[2], e.displacement[3],
        e.displacement[4], e.is_dead, e.stun_reset_timer,
        e.coll_valid, e.coll_land, e.coll_flags);
    if (zpp::bits::failure(result)) {
        return result;
    }
    for (uint32_t i = 0; i < kCollCorrectWords; ++i) {
        result = archive(e.coll_correct[i]);
        if (zpp::bits::failure(result)) {
            return result;
        }
    }
    result = wire(e.model, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    result = wire(e.gate, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    return archive(e.tick_timer);
}

template <class Archive>
constexpr auto wire(RngState& r, Archive& archive)
{
    auto result = archive(r.mti);
    if (zpp::bits::failure(result)) {
        return result;
    }
    for (uint32_t i = 0; i < kRngWords; ++i) {
        result = archive(r.mt[i]);
        if (zpp::bits::failure(result)) {
            return result;
        }
    }
    return zpp::bits::errc{};
}

template <class Archive>
constexpr auto wire(PadState& p, Archive& archive)
{
    return archive(p.buttons[0], p.buttons[1], p.buttons[2], p.buttons[3],
        p.analog[0], p.analog[1], p.analog[2], p.analog[3]);
}

template <class Archive>
constexpr auto wire(BattleSnapshot& s, Archive& archive)
{
    if (s.player_count > kMaxPlayers || s.enemy_count > kMaxEnemies) {
        return zpp::bits::errc{std::errc::invalid_argument};
    }
    auto result = archive(s.magic, s.version, s.player_count, s.enemy_count);
    if (zpp::bits::failure(result)) {
        return result;
    }
    // On input these fields came from the wire, so validate them before indexing fixed arrays.
    if (s.magic != kWireMagic || s.version != kWireVersion ||
        s.player_count > kMaxPlayers || s.enemy_count > kMaxEnemies) {
        return zpp::bits::errc{std::errc::invalid_argument};
    }
    result = wire(s.timing, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    result = wire(s.mediator, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    result = wire(s.rng, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    result = wire(s.camera, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    result = wire(s.pad, archive);
    if (zpp::bits::failure(result)) {
        return result;
    }
    for (uint32_t index = 0; index < s.player_count; ++index) {
        result = wire(s.players[index], archive);
        if (zpp::bits::failure(result)) {
            return result;
        }
    }
    for (uint32_t index = 0; index < s.enemy_count; ++index) {
        result = wire(s.enemies[index], archive);
        if (zpp::bits::failure(result)) {
            return result;
        }
    }
    return zpp::bits::errc{};
}

uint32_t state_bytes()
{
    static uint32_t cached = 0;
    if (cached) {
        return cached;
    }
    // Heap allocations freed after the one-time probe: keeping the worst-case snapshot and its
    // scratch as statics would pin ~12 MiB for the life of the process.
    const auto worst = std::make_unique<BattleSnapshot>();
    worst->player_count = kMaxPlayers;
    worst->enemy_count = kMaxEnemies;
    // a unit blending every slot is the wide case; motion_count is part of the wire, so leaving
    // it at 0 would under-size the session buffer and every real save would refuse
    for (uint32_t i = 0; i < kMaxPlayers; ++i) {
        worst->players[i].model.motion_count = (uint8_t)kMaxMotionSlots;
        worst->players[i].model.joint_count = (uint16_t)kMaxMotionJoints;
    }
    for (uint32_t i = 0; i < kMaxEnemies; ++i) {
        worst->enemies[i].model.motion_count = (uint8_t)kMaxMotionSlots;
        worst->enemies[i].model.joint_count = (uint16_t)kMaxMotionJoints;
    }
    const auto scratch = std::make_unique_for_overwrite<unsigned char[]>(kWireScratchBytes);
    zpp::bits::out<std::span<unsigned char>> out{std::span<unsigned char>{scratch.get(), kWireScratchBytes}};
    const auto result = wire(*worst, out);
    if (zpp::bits::failure(result)) {
        return 0;   // the format outgrew the scratch: state_size must be refused
    }
    cached = (uint32_t)out.position();
    return cached;
}

// ---------------------------------------------------------------- capture / restore

static uDamage_Old* get_enemy_damage_block(uEnemy_Old* enemy)
{
    if (!enemy) {
        return nullptr;
    }
    const int offset = EnemyTracker::get_enemy_specific_damage_offset((int)enemy->ID);
    if (offset == 0) {
        return nullptr;
    }
    return (uDamage_Old*)((char*)enemy + offset);
}

static uCollisionMgr* get_enemy_collision_manager(uEnemy_Old* enemy)
{
    if (!enemy) {
        return nullptr;
    }
    const int offset = EnemyTracker::get_enemy_specific_uCollision_offset((int)enemy->ID);
    return offset ? (uCollisionMgr*)((char*)enemy + offset) : nullptr;
}

static bool read_unit_vtable(const cUnit* unit, uintptr_t& vtable)
{
    vtable = 0;
    if (!unit) {
        return false;
    }
    __try {
        vtable = *(const uintptr_t*)unit;
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}

static bool is_player_unit(cUnit* unit)
{
    uintptr_t vtable = 0;
    return read_unit_vtable(unit, vtable) && (vtable == kNeroVtbl || vtable == kDanteVtbl);
}

// A stage transition swaps motion banks between frames, and an MPARAM_WORK can then still
// carry a stale pparam that is a small integer (0x1F was caught in the debugger on the save
// that advanced to the new stage) rather than a TrackHeader*. Non-null is not enough to
// dereference +1 / +0xC, so read both fields the way read_unit_vtable reads a vtable and
// report an access violation as "no track" - the save must not fault.
static bool read_motion_param(const MPARAM_WORK& param, uint8_t& track_type, uintptr_t& base)
{
    track_type = 0;
    base = 0;
    if (!param.pparam) {
        return false;
    }
    __try {
        track_type = *(const uint8_t*)(param.pparam + 1);
        base = *(const uintptr_t*)(param.pparam + 0xC);
        return true;
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
        EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }
}



// uModel::MotionArray is the root-motion sampler: the move tick writes uActor::mNowFramePos into
// the unit's position, and the sampler computes that value from these slots (TTD: the packed
// position of a re-simulated frame equals mNowFramePos). Capturing the position alone is not
// enough - the next tick overwrites it from the model, so the sampler state has to travel too.
static uint32_t capture_motion_cursor(const MPARAM_WORK& param, uint32_t alignment,
    uint8_t expected_track_type)
{
    uint8_t track_type = 0;
    uintptr_t base = 0;
    if (!read_motion_param(param, track_type, base)) {
        // unusedQuatParam is genuinely unused on models without a root-rotation track: its
        // non-null garbage is not a TrackHeader and must not be checksummed or restored.
        return kNoMotionTrack;
    }
    if (track_type != expected_track_type) {
        return kNoMotionTrack;
    }
    if (!base || !param.pcur_param || param.pcur_param < base) {
        return kInvalidMotionCursor;
    }
    const uintptr_t displacement = param.pcur_param - base;
    if ((displacement % alignment) != 0 || displacement > UINT32_MAX) {
        return kInvalidMotionCursor;
    }
    return (uint32_t)displacement;
}

static void apply_motion_cursor(MPARAM_WORK& param, uint32_t displacement)
{
    if (displacement == kNoMotionTrack) {
        param.pparam = 0;
        param.pcur_param = 0;
        return;
    }
    uint8_t track_type = 0;
    uintptr_t base = 0;
    read_motion_param(param, track_type, base);
    if (!base || displacement == kInvalidMotionCursor) {
        // A malformed state must not leave a null cursor behind a live track header.
        param.pparam = 0;
        param.pcur_param = 0;
        return;
    }
    param.pcur_param = base + displacement;
}

static bool capture_model(uModel* model, ModelState& out)
{
    std::memset(&out, 0, sizeof out);
    if (!model) {
        return true;
    }
    const uActor* actor = (const uActor*)model;
    out.now_frame_pos = actor->mNowFramePos;
    out.old_frame_pos = actor->mOldFramePos;
    out.now_frame_quat = actor->mNowFrameQuat;
    out.old_frame_quat = actor->mOldFrameQuat;
    static_assert(sizeof out.continue_words == 16, "mContinueCount + padding0 is 16 bytes");
    std::memcpy(out.continue_words, &model->mContinueCount, sizeof out.continue_words);

    uint32_t count = model->mBlendNum;
    if (count > kMaxMotionSlots) {
        count = kMaxMotionSlots;
    }
    const uint32_t joint_count = model->mJointNum;
    if (joint_count > kMaxMotionJoints) {
        return false;
    }
    if (joint_count && !model->mpJoint) {
        return false;
    }
    out.motion_count = (uint8_t)count;
    out.joint_count = (uint16_t)joint_count;
    for (uint32_t i = 0; i < count; ++i) {
        const Motion& m = model->MotionArray[i];
        MotionSlotState& s = out.motion[i];
        s.motion_no = m.mMotionNo;
        s.reserved = m.mReserved;
        s.state = m.mState;
        s.attr = m.mAttr;
        s.inter_frame = m.mInterFrame;
        s.inter_count = m.mInterCount;
        s.blend = m.mBlend;
        s.frame = m.mFrame;
        s.frame_max = m.mFrameMax;
        s.loop_frame = m.mLoopFrame;
        s.speed = m.mSpeed;
        s.inter_rate = m.mInterRate;
        s.next_frame = m.mNextFrame;
        s.inter_curve[0] = m.mInterCurve.p1;
        s.inter_curve[1] = m.mInterCurve.p2;
        // padding38 / mPrevTrans.x hold live per-process pointers (netdump cross-machine diff):
        // not captured, the struct ctor leaves them zero. See wire(MotionSlotState).
        s.prev_quat[0] = m.mPrevQuat.x;
        s.prev_quat[1] = m.mPrevQuat.y;
        s.prev_quat[2] = m.mPrevQuat.z;
        s.prev_quat[3] = m.mPrevQuat.w;
        s.base_quat[0] = m.mBaseQuat.x;
        s.base_quat[1] = m.mBaseQuat.y;
        s.base_quat[2] = m.mBaseQuat.z;
        s.base_quat[3] = m.mBaseQuat.w;
        s.null_quat[0] = m.mNullQuat.x;
        s.null_quat[1] = m.mNullQuat.y;
        s.null_quat[2] = m.mNullQuat.z;
        s.null_quat[3] = m.mNullQuat.w;
        // prev_trans[0] = m.mPrevTrans.x stays uncaptured like pad38: the block holds a live
        // per-process pointer (see wire(MotionSlotState)), so only [1] and [2] travel.
        s.prev_trans[1] = m.mPrevTrans.y;
        s.prev_trans[2] = m.mPrevTrans.z;
        s.base_trans[0] = m.mBaseTrans.x;
        s.base_trans[1] = m.mBaseTrans.y;
        s.base_trans[2] = m.mBaseTrans.z;
        s.null_trans[0] = m.mNullTrans.x;
        s.null_trans[1] = m.mNullTrans.y;
        s.null_trans[2] = m.mNullTrans.z;
        s.trans_frame = m.mTransParam.cur_frame;
        s.trans_weight = m.mTransParam.weight;
        s.quat_frame = m.unusedQuatParam.cur_frame;
        s.quat_weight = m.unusedQuatParam.weight;
        // Vector keys are 16 bytes; interpolated quaternion keys are 8 bytes.
        s.trans_cursor_offset = capture_motion_cursor(m.mTransParam, 16, 4);
        s.quat_cursor_offset = capture_motion_cursor(m.unusedQuatParam, 8, 3);
        if (s.trans_cursor_offset == kNoMotionTrack) {
            s.trans_frame = 0.0f;
            s.trans_weight = 0.0f;
        }
        if (s.quat_cursor_offset == kNoMotionTrack) {
            s.quat_frame = 0.0f;
            s.quat_weight = 0.0f;
        }
        if (s.trans_cursor_offset == kInvalidMotionCursor ||
            s.quat_cursor_offset == kInvalidMotionCursor) {
            return false;
        }
        s.sequence[0] = m.mSequence[0];
        s.sequence[1] = m.mSequence[1];
        s.prev_sequence[0] = m.mPrevSequence[0];
        s.prev_sequence[1] = m.mPrevSequence[1];

        const uint8_t* work = (const uint8_t*)m.mJoint;
        if (!work) {
            // Root-only slot: no per-joint sampler exists to carry. The state was zeroed at
            // entry, so this slot stays at the default no-track cursors.
            continue;
        }
        for (uint32_t j = 0; j < joint_count; ++j, work += 0x60) {
            JointCursorState& c = s.joints[j];
            const MPARAM_WORK& quat = *(const MPARAM_WORK*)(work + 0x00);
            const MPARAM_WORK& trans = *(const MPARAM_WORK*)(work + 0x10);
            const MPARAM_WORK& scale = *(const MPARAM_WORK*)(work + 0x20);
            c.quat_offset = capture_motion_cursor(quat, 1, 0);
            c.trans_offset = capture_motion_cursor(trans, 1, 1);
            c.scale_offset = capture_motion_cursor(scale, 1, 2);
            if (c.quat_offset == kInvalidMotionCursor ||
                c.trans_offset == kInvalidMotionCursor ||
                c.scale_offset == kInvalidMotionCursor) {
                return false;
            }
            if (c.quat_offset != kNoMotionTrack) {
                c.quat_frame = quat.cur_frame;
                c.quat_weight = quat.weight;
            }
            if (c.trans_offset != kNoMotionTrack) {
                c.trans_frame = trans.cur_frame;
                c.trans_weight = trans.weight;
            }
            if (c.scale_offset != kNoMotionTrack) {
                c.scale_frame = scale.cur_frame;
                c.scale_weight = scale.weight;
            }
        }
    }
    return true;
}

static bool motion_asset_available(const uModel* model, uint16_t motion_no)
{
    if (motion_no == 0xFFFF) {
        return true;
    }
    const uintptr_t list = model->mpMotionListArr[(motion_no >> 8) & 0xF];
    if (!list) {
        return false;
    }
    const uintptr_t header = *(const uintptr_t*)(list + 0x60);
    if (!header || (motion_no & 0xFF) >= *(const uint16_t*)(header + 6)) {
        return false;
    }
    return *(const uintptr_t*)(header + (motion_no & 0xFF) * 4 + 8) != 0;
}

static bool model_assets_available(const uModel* model, const ModelState& snap)
{
    if (snap.motion_count > kMaxMotionSlots || snap.joint_count != model->mJointNum) {
        return false;
    }
    for (uint32_t i = 0; i < snap.motion_count; ++i) {
        if (!motion_asset_available(model, snap.motion[i].motion_no)) {
            return false;
        }
    }
    return true;
}

// setupMotionParam: eax = model, ecx = motion number, stack = slot; retn 4.
// Rebind before applying cursor offsets: a cursor relative to the live motion's stream would
// select the wrong track if the animation changed inside the rollback window.
static void rebind_motion(uModel* model, Motion& motion, uint32_t slot, const MotionSlotState& snap)
{
    if (motion.mJoint && model->mpJoint) {
        for (uint32_t joint_index = 0; joint_index < model->mJointNum; ++joint_index) {
            char* work = (char*)motion.mJoint + joint_index * 0x60;
            const char* joint = (const char*)model->mpJoint + joint_index * 0x90;
            std::memset(work, 0, 0x30);                 // three MPARAM_WORK cursors
            std::memcpy(work + 0x30, joint + 0x20, 16); // mHQuat = current mQuat
            std::memcpy(work + 0x40, joint + 0x40, 16); // mHTrans = current mTrans
            std::memcpy(work + 0x50, joint + 0x30, 16); // mHScale = current mScale
        }
        *(float*)((char*)motion.mJoint + 0x44) += model->mWaistAdjust;
    }
    motion.mMotionNo = snap.motion_no;
    motion.mAttr = snap.attr;
    motion.mFrame = snap.frame;
    const uint16_t motion_no = snap.motion_no;
    static constexpr auto func = 0x009F9060;
    _asm {
        push slot
        movzx ecx, motion_no
        mov eax, model
        call func
    }
}

static void apply_model(uModel* model, const ModelState& snap)
{
    if (!model) {
        return;
    }
    uActor* actor = (uActor*)model;
    actor->mNowFramePos = snap.now_frame_pos;
    actor->mOldFramePos = snap.old_frame_pos;
    actor->mNowFrameQuat = snap.now_frame_quat;
    actor->mOldFrameQuat = snap.old_frame_quat;
    std::memcpy(&model->mContinueCount, snap.continue_words, sizeof snap.continue_words);

    // the slot count is state too: the model iterates mBlendNum slots, so a restore that keeps
    // the live count would leave the sampler half rewound
    model->mBlendNum = snap.motion_count;
    for (uint32_t i = 0; i < snap.motion_count; ++i) {
        Motion& m = model->MotionArray[i];
        const MotionSlotState& s = snap.motion[i];
        if (m.mMotionNo != s.motion_no ||
            (s.trans_cursor_offset != kNoMotionTrack && !m.mTransParam.pparam) ||
            (s.quat_cursor_offset != kNoMotionTrack && !m.unusedQuatParam.pparam)) {
            rebind_motion(model, m, i, s);
        }
        m.mMotionNo = s.motion_no;
        m.mReserved = s.reserved;
        m.mState = s.state;
        m.mAttr = s.attr;
        m.mInterFrame = s.inter_frame;
        m.mInterCount = s.inter_count;
        m.mBlend = s.blend;
        m.mFrame = s.frame;
        m.mFrameMax = s.frame_max;
        m.mLoopFrame = s.loop_frame;
        m.mSpeed = s.speed;
        m.mInterRate = s.inter_rate;
        m.mNextFrame = s.next_frame;
        m.mInterCurve.p1 = s.inter_curve[0];
        m.mInterCurve.p2 = s.inter_curve[1];
        // padding38 / mPrevTrans.x stay as the live game left them: they hold pointers the
        // snapshot must not rebind (see wire(MotionSlotState)).
        m.mPrevQuat.x = s.prev_quat[0];
        m.mPrevQuat.y = s.prev_quat[1];
        m.mPrevQuat.z = s.prev_quat[2];
        m.mPrevQuat.w = s.prev_quat[3];
        m.mBaseQuat.x = s.base_quat[0];
        m.mBaseQuat.y = s.base_quat[1];
        m.mBaseQuat.z = s.base_quat[2];
        m.mBaseQuat.w = s.base_quat[3];
        m.mNullQuat.x = s.null_quat[0];
        m.mNullQuat.y = s.null_quat[1];
        m.mNullQuat.z = s.null_quat[2];
        m.mNullQuat.w = s.null_quat[3];
        m.mPrevTrans.y = s.prev_trans[1];
        m.mPrevTrans.z = s.prev_trans[2];
        m.mBaseTrans.x = s.base_trans[0];
        m.mBaseTrans.y = s.base_trans[1];
        m.mBaseTrans.z = s.base_trans[2];
        m.mNullTrans.x = s.null_trans[0];
        m.mNullTrans.y = s.null_trans[1];
        m.mNullTrans.z = s.null_trans[2];
        m.mTransParam.cur_frame = s.trans_frame;
        m.mTransParam.weight = s.trans_weight;
        m.unusedQuatParam.cur_frame = s.quat_frame;
        m.unusedQuatParam.weight = s.quat_weight;
        apply_motion_cursor(m.mTransParam, s.trans_cursor_offset);
        apply_motion_cursor(m.unusedQuatParam, s.quat_cursor_offset);
        m.mSequence[0] = s.sequence[0];
        m.mSequence[1] = s.sequence[1];
        m.mPrevSequence[0] = s.prev_sequence[0];
        m.mPrevSequence[1] = s.prev_sequence[1];

        uint8_t* work = (uint8_t*)m.mJoint;
        if (!work) {
            continue;   // the save was taken with the same root-only slot (see capture_model)
        }
        for (uint32_t j = 0; j < snap.joint_count; ++j, work += 0x60) {
            const JointCursorState& c = s.joints[j];
            MPARAM_WORK& quat = *(MPARAM_WORK*)(work + 0x00);
            MPARAM_WORK& trans = *(MPARAM_WORK*)(work + 0x10);
            MPARAM_WORK& scale = *(MPARAM_WORK*)(work + 0x20);
            apply_motion_cursor(quat, c.quat_offset);
            apply_motion_cursor(trans, c.trans_offset);
            apply_motion_cursor(scale, c.scale_offset);
            quat.cur_frame = c.quat_frame;
            quat.weight = c.quat_weight;
            trans.cur_frame = c.trans_frame;
            trans.weight = c.trans_weight;
            scale.cur_frame = c.scale_frame;
            scale.weight = c.scale_weight;
        }
    }
}

static void capture_damage_block(const uDamage_Old* damage, EnemyState& snap)
{
    snap.hp = damage->HP;
    snap.hp_max = damage->HPMax;
    snap.hp_taken = damage->HPTaken;
    snap.prev_damage_resist = damage->prevDamageResist;
    for (int i = 0; i < 5; ++i) {
        snap.stun[i] = damage->stun[i];
        snap.displacement[i] = damage->displacement[i];
    }
    snap.is_dead = damage->isDead;
    snap.stun_reset_timer = damage->stunResetTimer;
}

static void write_damage_block(uDamage_Old* damage, const EnemyState& snap)
{
    damage->HP = snap.hp;
    damage->HPMax = snap.hp_max;
    damage->HPTaken = snap.hp_taken;
    damage->prevDamageResist = snap.prev_damage_resist;
    for (int i = 0; i < 5; ++i) {
        damage->stun[i] = snap.stun[i];
        damage->displacement[i] = snap.displacement[i];
    }
    damage->isDead = snap.is_dead;
    damage->stunResetTimer = snap.stun_reset_timer;
}

// cLockOnTarget is declared with private members, so its value fields are read by offset. Layout
// from the sdk + the compiler's 16-byte MtVector3 alignment: vtable 0, mpNext 4, mpModel 8,
// mpDmgTbl 0xC, mPos 0x10, mType 0x20, mPILength 0x24, mMaxHP 0x28, mHp 0x2C, mRange 0x30,
// mIsIn 0x34 .. mIsNoInterTarget 0x3A, mNearPos 0x40 (sizeof 0x50).
constexpr uintptr_t kLockOnPI = 0x24, kLockOnMaxHp = 0x28, kLockOnHp = 0x2C, kLockOnRange = 0x30;
constexpr uintptr_t kLockOnIsIn = 0x34;

static void capture_gate(uEnemy_Old* enemy, BattleGateState& out)
{
    out = {};
    if (!enemy) {
        return;
    }
    const char* base = (const char*)enemy;
    out.actor_status = *(const uint32_t*)(base + kActorStatusOffset);
    out.mode = *(const uint32_t*)(base + kBattleModeOffset);
    out.battle_flag = *(const uint8_t*)(base + kBattleFlagOffset);
    const void* target = *(const void* const*)(base + kEnemyTargetOffset);
    uintptr_t target_vtable = 0;
    if (!target || !read_unit_vtable((const cUnit*)target, target_vtable)) {
        out.target_kind = 0;       // null or stale: restore clears it instead of preserving it
    } else if (target_vtable == kNeroVtbl || target_vtable == kDanteVtbl) {
        out.target_kind = 1;
        out.target_player = ((const uPlayer_old*)target)->padID;
    } else {
        out.target_kind = 0xFF;   // valid non-player: no identity to restore by
    }
    const char* lockon = base + kLockOnOffset;
    const float* lockon_pos = (const float*)(lockon + 0x10);
    out.lockon_pos[0] = lockon_pos[0];
    out.lockon_pos[1] = lockon_pos[1];
    out.lockon_pos[2] = lockon_pos[2];
    out.lockon_pi_length = *(const float*)(lockon + kLockOnPI);
    out.lockon_max_hp = *(const float*)(lockon + kLockOnMaxHp);
    out.lockon_hp = *(const float*)(lockon + kLockOnHp);
    out.lockon_range = *(const float*)(lockon + kLockOnRange);
    out.lockon_is_in = *(const uint8_t*)(lockon + kLockOnIsIn);
    out.lockon_is_target = *(const uint8_t*)(lockon + kLockOnIsIn + 1);
    out.lockon_is_catch = *(const uint8_t*)(lockon + kLockOnIsIn + 2);
    out.lockon_camera_in = *(const uint8_t*)(lockon + kLockOnIsIn + 3);
    out.lockon_no_target = *(const uint8_t*)(lockon + kLockOnIsIn + 4);
    out.lockon_inter_target = *(const uint8_t*)(lockon + kLockOnIsIn + 5);
    out.lockon_no_inter = *(const uint8_t*)(lockon + kLockOnIsIn + 6);
}

static void apply_gate(uEnemy_Old* enemy, const BattleGateState& snap)
{
    if (!enemy) {
        return;
    }
    char* base = (char*)enemy;
    *(uint32_t*)(base + kActorStatusOffset) = snap.actor_status;
    *(uint32_t*)(base + kBattleModeOffset) = snap.mode;
    *(uint8_t*)(base + kBattleFlagOffset) = snap.battle_flag;
    if (snap.target_kind == 0 || snap.target_kind == 1) {
        *(void**)(base + kEnemyTargetOffset) = nullptr;
    }
    if (snap.target_kind == 1) {
        if (sUnit* units = devil4_sdk::get_sUnit()) {
            int walked = 0;
            for (cUnit* unit = (cUnit*)units->mMoveLine[kLinePlayer].mTop;
                unit && walked < kMaxLineWalk; unit = unit->mp_next_unit, ++walked) {
                if (is_player_unit(unit) && ((uPlayer_old*)unit)->padID == snap.target_player) {
                    *(void**)(base + kEnemyTargetOffset) = unit;
                    break;
                }
            }
        }
    }
    char* lockon = base + kLockOnOffset;
    float* lockon_pos = (float*)(lockon + 0x10);
    lockon_pos[0] = snap.lockon_pos[0];
    lockon_pos[1] = snap.lockon_pos[1];
    lockon_pos[2] = snap.lockon_pos[2];
    *(float*)(lockon + kLockOnPI) = snap.lockon_pi_length;
    *(float*)(lockon + kLockOnMaxHp) = snap.lockon_max_hp;
    *(float*)(lockon + kLockOnHp) = snap.lockon_hp;
    *(float*)(lockon + kLockOnRange) = snap.lockon_range;
    *(uint8_t*)(lockon + kLockOnIsIn) = snap.lockon_is_in;
    *(uint8_t*)(lockon + kLockOnIsIn + 1) = snap.lockon_is_target;
    *(uint8_t*)(lockon + kLockOnIsIn + 2) = snap.lockon_is_catch;
    *(uint8_t*)(lockon + kLockOnIsIn + 3) = snap.lockon_camera_in;
    *(uint8_t*)(lockon + kLockOnIsIn + 4) = snap.lockon_no_target;
    *(uint8_t*)(lockon + kLockOnIsIn + 5) = snap.lockon_inter_target;
    *(uint8_t*)(lockon + kLockOnIsIn + 6) = snap.lockon_no_inter;
}

// The lock-on numbers are copies of the target unit's own state (its mPos at +0x30, its HP), which
// the snapshot already carries per player, so they are excluded from the compared bytes.
void scrub_scratch_gate(BattleGateState& g)
{
    g.lockon_pos[0] = 0.0f;
    g.lockon_pos[1] = 0.0f;
    g.lockon_pos[2] = 0.0f;
    g.lockon_range = 0.0f;
    g.lockon_max_hp = 0.0f;
    g.lockon_hp = 0.0f;
    g.lockon_pi_length = 0.0f;
}

// The merged LAN netdump\ diff (netdump_tool.exe, f0/f100 pairs): besides the two live
// pointers already off the wire (v16), the compared bytes still differ in
// model.continue_words[0]/[3] (heap words 0x05A0xxxx, different per process) and in the
// prev/base/null quat+trans blocks of the INACTIVE blend slots (values are denormals and
// 3.2e7 - garbage, not quaternion components; the active slot's data matched exactly).
// The sampler scalars, joint cursors, positions, HP, timing and RNG all match, so the
// simulation agrees: zero exactly this garbage in the compared copy. The ring copy keeps
// the live bytes - restore must reproduce what the game had. The scrub covers EVERY slot's
// blocks, the blending ones included (see the hpp note): a real divergence there surfaces
// through the positions and sampler scalars the checksum does cover.
void scrub_desync_noise(BattleSnapshot& snap)
{
    auto scrub_model = [](ModelState& m) {
        for (uint32_t& w : m.continue_words) {
            w = 0;
        }
        for (uint32_t s = 0; s < kMaxMotionSlots; ++s) {
            MotionSlotState& slot = m.motion[s];
            for (float& q : slot.prev_quat) q = 0.0f;
            for (float& q : slot.base_quat) q = 0.0f;
            for (float& q : slot.null_quat) q = 0.0f;
            slot.prev_trans[1] = 0.0f;
            slot.prev_trans[2] = 0.0f;
            for (float& t : slot.base_trans) t = 0.0f;
            for (float& t : slot.null_trans) t = 0.0f;
        }
    };
    for (uint32_t i = 0; i < snap.player_count; ++i) {
        scrub_model(snap.players[i].model);
    }
    for (uint32_t i = 0; i < snap.enemy_count; ++i) {
        scrub_model(snap.enemies[i].model);
    }
}

// The camera is stepped once per PRESENTED frame, outside the tick, and its smoothing consumes
// the presentation clock. A rollback re-run that executes several ticks back to back therefore
// cannot reproduce the exact camera float, even though the simulation state it is derived from
// matches. Restore still writes the camera (input mapping reads it), but the compared bytes do
// not: a camera difference that actually bends movement shows up in the player position/velocity
// that the checksum does cover.
void scrub_scratch_camera(CameraState& c)
{
    c = {};
}

static bool capture_enemy(uEnemy_Old* enemy, EnemyState& snap)
{
    std::memset(&snap, 0, sizeof snap);
    if (!enemy) {
        return false;
    }

    snap.id = (int32_t)enemy->ID;
    snap.team = enemy->team;
    snap.is_active = enemy->isActive;
    snap.in_battle = enemy->inBattle;
    snap.unit_flags = (uint32_t)enemy->flags;
    snap.unit_delta_time = enemy->delta;
    snap.move_id = enemy->moveID;
    snap.move_part = enemy->movePart;
    snap.position = enemy->position;
    snap.velocity = enemy->velocity;
    const char* enemy_base = (const char*)enemy;
    snap.actor_velocity = *(const MtVector3*)(enemy_base + kActorVelocity);
    snap.actor_acceleration = *(const MtVector3*)(enemy_base + kActorAcceleration);
    snap.target_position = *(const MtVector3*)(enemy_base + kEnemyTargetPos);
    snap.rotation = enemy->rotation;
    snap.scale = enemy->scale;
    snap.quat_w = *(const float*)((const char*)enemy + kEnemyQuatW);
    snap.knockback_position = enemy->positionKnockback;
    snap.knockback_rotation = enemy->rotationKnockback;
    snap.anim_id = enemy->animID;
    snap.anim_frame = enemy->animFrame;
    snap.anim_frame_max = enemy->animFrameMax;
    snap.dt = *(const uint8_t*)(enemy_base + kEnemyDt) != 0;
    snap.dt_ok = *(const uint8_t*)(enemy_base + kEnemyDtOk);
    snap.dt_timer_enabled = *(const uint8_t*)(enemy_base + kEnemyDtTimerEnable);
    snap.dt_sign = *(const uint8_t*)(enemy_base + kEnemyDtSign);
    snap.dt_timer = enemy->DTTimer;

    // The corrections moveEx re-applies to mPos every tick, at this enemy ID's real manager.
    if (const uCollisionMgr* coll = get_enemy_collision_manager(enemy)) {
        const char* bytes = (const char*)coll;
        snap.coll_valid = true;
        std::memcpy(&snap.coll_land, bytes + kCollLandOffset, sizeof snap.coll_land);
        std::memcpy(&snap.coll_flags, bytes + kCollFlagsOffset, sizeof snap.coll_flags);
        std::memcpy(snap.coll_correct, bytes + kCollCorrectOffset, sizeof snap.coll_correct);
    }

    switch (snap.id) {
    case BERIAL:
        snap.berial_fire = enemy->berialFire;
        snap.berial_fire_timer = enemy->berialFireTimer;
        break;
    case GLADIUS:
        snap.gladius_timer = enemy->gladiusTimer;
        snap.gladius_buried = enemy->gladiusBuried;
        break;
    case MEPHISTO:
    case FAUST:
        snap.faust_cloak = enemy->faustCloak;
        snap.faust_cloak_timer = enemy->faustCloakTimer;
        break;
    case BLITZ:
        snap.blitz_electric = enemy->blitzElectric;
        snap.blitz_electric_timer = enemy->blitzElectricTimer;
        snap.blitz_electric_suicide_timer = enemy->blitzElectricSuicideTimer;
        break;
    case ANGELO_BIANCO:
    case ANGELO_ALTO:
        snap.angelo_shield = enemy->angeloShield;
        break;
    case CREDO:
        snap.credo_shield = enemy->credoShield;
        snap.credo_shield_timer = enemy->credoShieldTimer;
        break;
    case SANCTUS_M11:
        snap.sanctus_shield_m11 = enemy->sanctusShieldM11;
        snap.sanctus_shield_timer_m11 = enemy->sanctusShieldTimerM11;
        break;
    case SANCTUS_M20:
        snap.sanctus_shield_m20 = enemy->sanctusShieldM20;
        snap.sanctus_shield_timer_m20 = enemy->sanctusShieldTimerM20;
        break;
    case FROST:
        snap.frost_heal_count = enemy->frostHealCount;
        break;
    default:
        break;
    }

    if (const uDamage_Old* damage = get_enemy_damage_block(enemy)) {
        snap.damage_valid = true;
        capture_damage_block(damage, snap);
    }
    if (!capture_model((uModel*)enemy, snap.model)) {
        return false;
    }
    capture_gate(enemy, snap.gate);
    snap.tick_timer = *(const float*)((const char*)enemy + kEnemyTickTimer);
    return true;
}

static void apply_enemy(uEnemy_Old* enemy, const EnemyState& snap)
{
    if (!enemy) {
        return;
    }
    enemy->team = snap.team;
    enemy->isActive = snap.is_active;
    enemy->inBattle = snap.in_battle;
    enemy->flags = (int)snap.unit_flags;
    enemy->delta = snap.unit_delta_time;
    enemy->moveID = snap.move_id;
    enemy->movePart = snap.move_part;
    enemy->position = snap.position;
    enemy->velocity = snap.velocity;
    char* enemy_base = (char*)enemy;
    *(MtVector3*)(enemy_base + kActorVelocity) = snap.actor_velocity;
    *(MtVector3*)(enemy_base + kActorAcceleration) = snap.actor_acceleration;
    *(MtVector3*)(enemy_base + kEnemyTargetPos) = snap.target_position;
    enemy->rotation = snap.rotation;
    enemy->scale = snap.scale;
    *(float*)((char*)enemy + kEnemyQuatW) = snap.quat_w;
    enemy->positionKnockback = snap.knockback_position;
    enemy->rotationKnockback = snap.knockback_rotation;
    enemy->animID = snap.anim_id;
    enemy->animFrame = snap.anim_frame;
    enemy->animFrameMax = snap.anim_frame_max;
    *(uint8_t*)(enemy_base + kEnemyDt) = snap.dt ? 1 : 0;
    *(uint8_t*)(enemy_base + kEnemyDtOk) = snap.dt_ok;
    *(uint8_t*)(enemy_base + kEnemyDtTimerEnable) = snap.dt_timer_enabled;
    *(uint8_t*)(enemy_base + kEnemyDtSign) = snap.dt_sign;
    enemy->DTTimer = snap.dt_timer;
    if (snap.coll_valid) {
        if (uCollisionMgr* coll = get_enemy_collision_manager(enemy)) {
            char* bytes = (char*)coll;
            std::memcpy(bytes + kCollLandOffset, &snap.coll_land, sizeof snap.coll_land);
            std::memcpy(bytes + kCollFlagsOffset, &snap.coll_flags, sizeof snap.coll_flags);
            std::memcpy(bytes + kCollCorrectOffset, snap.coll_correct, sizeof snap.coll_correct);
        }
    }

    switch (snap.id) {
    case BERIAL:
        enemy->berialFire = snap.berial_fire;
        enemy->berialFireTimer = snap.berial_fire_timer;
        break;
    case GLADIUS:
        enemy->gladiusTimer = snap.gladius_timer;
        enemy->gladiusBuried = snap.gladius_buried;
        break;
    case MEPHISTO:
    case FAUST:
        enemy->faustCloak = snap.faust_cloak;
        enemy->faustCloakTimer = snap.faust_cloak_timer;
        break;
    case BLITZ:
        enemy->blitzElectric = snap.blitz_electric;
        enemy->blitzElectricTimer = snap.blitz_electric_timer;
        enemy->blitzElectricSuicideTimer = snap.blitz_electric_suicide_timer;
        break;
    case ANGELO_BIANCO:
    case ANGELO_ALTO:
        enemy->angeloShield = snap.angelo_shield;
        break;
    case CREDO:
        enemy->credoShield = snap.credo_shield;
        enemy->credoShieldTimer = snap.credo_shield_timer;
        break;
    case SANCTUS_M11:
        enemy->sanctusShieldM11 = snap.sanctus_shield_m11;
        enemy->sanctusShieldTimerM11 = snap.sanctus_shield_timer_m11;
        break;
    case SANCTUS_M20:
        enemy->sanctusShieldM20 = snap.sanctus_shield_m20;
        enemy->sanctusShieldTimerM20 = snap.sanctus_shield_timer_m20;
        break;
    case FROST:
        enemy->frostHealCount = snap.frost_heal_count;
        break;
    default:
        break;
    }

    if (snap.damage_valid) {
        if (uDamage_Old* damage = get_enemy_damage_block(enemy)) {
            write_damage_block(damage, snap);
        }
    }
    apply_model((uModel*)enemy, snap.model);
    apply_gate(enemy, snap.gate);
    *(float*)((char*)enemy + kEnemyTickTimer) = snap.tick_timer;
}

static bool capture_player(uPlayer_old* player, PlayerState& snap)
{
    std::memset(&snap, 0, sizeof snap);
    if (!player) {
        return false;
    }

    snap.controller_id = player->controllerID;
    snap.pad_id = player->padID;
    snap.is_active = player->isActive;
    snap.unit_flags = player->flags.raw;
    snap.unit_delta_time = player->m_delta_time;
    snap.position = player->mPos;
    snap.velocity = player->m_d_velocity;
    snap.rotation2 = player->rotation2;
    snap.rotation3 = player->rotation3;
    snap.facing_direction = player->facingDirection;
    snap.ground_inertia_x = player->groundInertiaX;
    snap.inertia_y = player->inertiaY;
    snap.ground_inertia_z = player->groundInertiaZ;
    snap.ground_inertia = player->groundInertia;
    snap.inertia = player->inertia;
    snap.inertia_direction_real = player->inertiaDirectionReal;
    snap.vel_magnitude = player->m_d_vel_magnitude;
    snap.direction_of_magnitude = player->directionOfMagnitude;
    snap.anim_id = player->animID;
    snap.anim_frame = player->animFrame;
    snap.anim_frame_max = player->animFrameMax;
    snap.move_bank = player->moveBank;
    snap.move_part = player->movePart;
    snap.move_id2 = player->moveID2;
    snap.move_id3 = player->moveID3;
    snap.move_id_best = player->moveIDBest;
    snap.walk_type = player->walkType;
    snap.grounded = player->grounded;
    snap.grounded2_flag = player->grounded2;
    snap.air_attack_count = player->airAttackCount;
    snap.air_hike_count = player->airHikeCount;
    snap.weight = player->weight;
    snap.can_air_hike = player->canAirHike;
    snap.can_wall_hike = player->canWallHike;
    snap.jc_timer = player->jcTimer;
    snap.round_trip_charge = player->roundTripCharge;
    snap.character_speed = player->characterSpeed;
    snap.hitstop = player->hitstop;
    snap.hitstop_timer = player->hitstopTimer;

    snap.hp = player->damageStruct.HP;
    snap.hp_max = player->damageStruct.HPMax;
    snap.hp_taken = player->damageStruct.HPTaken;
    snap.prev_damage_resist = player->damageStruct.prevDamageResist;
    for (int i = 0; i < 5; ++i) {
        snap.stun[i] = player->damageStruct.stun[i];
        snap.displacement[i] = player->damageStruct.displacement[i];
    }
    snap.is_dead = player->damageStruct.isDead;
    snap.stun_reset_timer = player->damageStruct.stunResetTimer;

    snap.current_style = player->currentStyle;
    snap.dt = player->DT;
    snap.max_dt = player->maxDT;
    snap.dt_active = player->dtActive;
    snap.dt_outfit = player->dtOutfit;
    snap.exceed_level = player->exceedLevel;
    snap.exceed_timer = player->exceedTimer;
    snap.can_exceed = player->canExceed;
    snap.is_exceeding = player->isExceeding;
    snap.guard_timer = player->guardTimer;
    snap.revenge_gauge = player->revengeGauge;
    snap.disaster_gauge = player->disasterGauge;
    snap.dreadnaught = player->dreadnaught;
    snap.next_sword = player->nextSword;
    snap.next_gun = player->nextGun;
    snap.current_sword = player->currentSword;
    snap.current_gun = player->currentGun;
    snap.can_weapon_change = player->canWeaponChange;
    snap.is_lockon_animation = player->isLockonAnimation;
    snap.locked_on = player->lockedOn;

    for (int i = 0; i < 4; ++i) {
        snap.input_hold[i] = player->inputHold[i];
        snap.input_press[i] = player->inputPress[i];
        snap.input_release[i] = player->inputRelease[i];
        snap.cancels1[i] = player->cancels1[i];
        snap.melee_cancel_permissions1[i] = player->meleeCancelPermissions1[i];
        snap.jump_cancel_permissions1[i] = player->jumpCancelPermissions1[i];
    }
    snap.cancels2 = player->cancels2;
    snap.melee_cancel_permissions2 = player->meleeCancelPermissions2;
    snap.jump_cancel_permissions2 = player->jumpCancelPermissions2;
    snap.gun_cancel_permissions2 = player->gunCancelPermissions2;
    snap.buffer_permissions = player->bufferPermissions;
    snap.directional_melee_permissions = player->directionalMeleePermissions;
    snap.button_input_raw = player->buttonInputRaw;
    snap.stick_input_raw = player->stickInputRaw;
    snap.stick_direction = player->stickDirection;
    snap.stick_past_threshold = player->stickPastThreshold;
    snap.buffered_jump = player->bufferedJump;
    snap.tilt_forward = player->tiltForward;
    snap.tilt_back = player->tiltBack;
    snap.tilt_back_forward = player->tiltBackForward;
    snap.trigger_big_jump = player->triggerBigJump;
    snap.trigger_stinger_jump = player->triggerStingerJump;
    return capture_model((uModel*)player, snap.model);
}

static void apply_player(uPlayer_old* player, const PlayerState& snap)
{
    if (!player) {
        return;
    }
    player->padID = snap.pad_id;
    player->isActive = snap.is_active;
    player->flags.raw = snap.unit_flags;
    player->m_delta_time = snap.unit_delta_time;
    player->mPos = snap.position;
    player->m_d_velocity = snap.velocity;
    player->rotation2 = snap.rotation2;
    player->rotation3 = snap.rotation3;
    player->facingDirection = snap.facing_direction;
    player->groundInertiaX = snap.ground_inertia_x;
    player->inertiaY = snap.inertia_y;
    player->groundInertiaZ = snap.ground_inertia_z;
    player->groundInertia = snap.ground_inertia;
    player->inertia = snap.inertia;
    player->inertiaDirectionReal = snap.inertia_direction_real;
    player->m_d_vel_magnitude = snap.vel_magnitude;
    player->directionOfMagnitude = snap.direction_of_magnitude;
    player->animID = snap.anim_id;
    player->animFrame = snap.anim_frame;
    player->animFrameMax = snap.anim_frame_max;
    player->moveBank = snap.move_bank;
    player->movePart = snap.move_part;
    player->moveID2 = snap.move_id2;
    player->moveID3 = snap.move_id3;
    player->moveIDBest = snap.move_id_best;
    player->walkType = snap.walk_type;
    player->grounded = snap.grounded;
    player->grounded2 = snap.grounded2_flag;
    player->airAttackCount = snap.air_attack_count;
    player->airHikeCount = snap.air_hike_count;
    player->weight = snap.weight;
    player->canAirHike = snap.can_air_hike;
    player->canWallHike = snap.can_wall_hike;
    player->jcTimer = snap.jc_timer;
    player->roundTripCharge = snap.round_trip_charge;
    player->characterSpeed = snap.character_speed;
    player->hitstop = snap.hitstop;
    player->hitstopTimer = snap.hitstop_timer;

    player->damageStruct.HP = snap.hp;
    player->damageStruct.HPMax = snap.hp_max;
    player->damageStruct.HPTaken = snap.hp_taken;
    player->damageStruct.prevDamageResist = snap.prev_damage_resist;
    for (int i = 0; i < 5; ++i) {
        player->damageStruct.stun[i] = snap.stun[i];
        player->damageStruct.displacement[i] = snap.displacement[i];
    }
    player->damageStruct.isDead = snap.is_dead;
    player->damageStruct.stunResetTimer = snap.stun_reset_timer;

    player->currentStyle = snap.current_style;
    player->DT = snap.dt;
    player->maxDT = snap.max_dt;
    player->dtActive = snap.dt_active;
    player->dtOutfit = snap.dt_outfit;
    player->exceedLevel = snap.exceed_level;
    player->exceedTimer = snap.exceed_timer;
    player->canExceed = snap.can_exceed;
    player->isExceeding = snap.is_exceeding;
    player->guardTimer = snap.guard_timer;
    player->revengeGauge = snap.revenge_gauge;
    player->disasterGauge = snap.disaster_gauge;
    player->dreadnaught = snap.dreadnaught;
    player->nextSword = snap.next_sword;
    player->nextGun = snap.next_gun;
    player->currentSword = snap.current_sword;
    player->currentGun = snap.current_gun;
    player->canWeaponChange = snap.can_weapon_change;
    player->isLockonAnimation = snap.is_lockon_animation;
    player->lockedOn = snap.locked_on;

    for (int i = 0; i < 4; ++i) {
        player->inputHold[i] = snap.input_hold[i];
        player->inputPress[i] = snap.input_press[i];
        player->inputRelease[i] = snap.input_release[i];
        player->cancels1[i] = snap.cancels1[i];
        player->meleeCancelPermissions1[i] = snap.melee_cancel_permissions1[i];
        player->jumpCancelPermissions1[i] = snap.jump_cancel_permissions1[i];
    }
    player->cancels2 = snap.cancels2;
    player->meleeCancelPermissions2 = snap.melee_cancel_permissions2;
    player->jumpCancelPermissions2 = snap.jump_cancel_permissions2;
    player->gunCancelPermissions2 = snap.gun_cancel_permissions2;
    player->bufferPermissions = snap.buffer_permissions;
    player->directionalMeleePermissions = snap.directional_melee_permissions;
    player->buttonInputRaw = snap.button_input_raw;
    player->stickInputRaw = snap.stick_input_raw;
    player->stickDirection = snap.stick_direction;
    player->stickPastThreshold = snap.stick_past_threshold;
    player->bufferedJump = snap.buffered_jump;
    player->tiltForward = snap.tilt_forward;
    player->tiltBack = snap.tilt_back;
    player->tiltBackForward = snap.tilt_back_forward;
    player->triggerBigJump = snap.trigger_big_jump;
    player->triggerStingerJump = snap.trigger_stinger_jump;
    apply_model((uModel*)player, snap.model);
}

static void capture_timing(sDevil4Main* main, uint64_t sim_time_us, TimingState& out)
{
    out.sim_time_us = sim_time_us;
    out.main_timer = main->mTimer;         // actual pre-tick member; sim_time_us is already advanced
    out.frame_timer = main->mFrameTimer;
    out.fps = main->mFps;
    out.fps_lock = main->mFPSlock;
    out.fps_actual = main->mFPSActual;
    out.wanna_skip_frame = main->mWannaSkipFrame;
    out.skip = main->mSkip;
    out.pause_flags = (uint8_t)((main->mPause ? 1 : 0) | (main->mUnpause ? 2 : 0) |
        (main->mJobInit ? 4 : 0) | (main->mFinalize ? 8 : 0));
    out.delta_time_norm = main->mDeltaTimeNorm;
    out.delta_time_max = main->mDeltaTimeMax;
    out.delta_time_max2 = main->mDletaTimeMax;
    out.speed_global = main->mSpeedGlobal;
    out.delay_frame = main->mDelayFrame;
}

static void apply_timing(sDevil4Main* main, const TimingState& snap, uint64_t& sim_time_us)
{
    sim_time_us = snap.sim_time_us;
    main->mTimer = snap.main_timer;
    main->mFrameTimer = snap.frame_timer;
    main->mFps = snap.fps;
    main->mFPSlock = snap.fps_lock;
    main->mFPSActual = snap.fps_actual;
    main->mWannaSkipFrame = snap.wanna_skip_frame;
    main->mSkip = snap.skip;
    main->mPause = (snap.pause_flags & 1) != 0;
    main->mUnpause = (snap.pause_flags & 2) != 0;
    main->mJobInit = (snap.pause_flags & 4) != 0;
    main->mFinalize = (snap.pause_flags & 8) != 0;
    main->mDeltaTimeNorm = snap.delta_time_norm;
    main->mDeltaTimeMax = snap.delta_time_max;
    main->mDletaTimeMax = snap.delta_time_max2;
    main->mSpeedGlobal = snap.speed_global;
    main->mDelayFrame = snap.delay_frame;
}

static void capture_mediator(sMediator* mediator, MediatorState& out)
{
    out.mission_id = mediator->missionID;
    out.room_id = mediator->roomID;
    for (int i = 0; i < 3; ++i) {
        out.enemy_count[i] = mediator->enemyCount[i];
    }
    out.orb_count = mediator->orbCount;
    out.orb_mission_current = mediator->orbMissionCurrent;
    out.orb_mission_potential = mediator->orbMissionPotential;
    out.hell_gold_orb_count = mediator->hellGoldOrbCount;
    out.game_difficulty = (uint32_t)mediator->gameDifficulty;
    out.game_mode = (uint32_t)mediator->gameMode;
    out.enemy_difficulty = mediator->enemyDifficulty;
    out.style_points = mediator->stylePoints;
    out.unlock_flags = (uint8_t)((mediator->hasUnlockedDT ? 1 : 0) |
        (mediator->hasUnlockedAirHike ? 2 : 0) | (mediator->hasUnlockedSprint ? 4 : 0) |
        (mediator->hasUnlockedHold ? 8 : 0) | (mediator->hasUnlockedMaxAct ? 16 : 0));
    out.disaster_gauge_display = mediator->displayDisasterGauge ? 1 : 0;
    out.turbo_enabled = mediator->turboEnabled ? 1 : 0;

    if (sWorkRate* work = devil4_sdk::get_work_rate()) {
        out.game_speed = work->game_speed;
        out.work_unk2 = work->unk_float2;
        out.global_speed = work->global_speed;
        out.player_speed = work->player_speed;
        out.enemy_speed = work->enemy_speed;
        out.room_speed = work->room_speed;
        out.turbo_speed = work->turbo_speed;
        out.work_unk3 = work->unk_float3;
    }

    if (sStylishCount* stylish = devil4_sdk::get_stylish_count()) {
        out.stylish_tier = stylish->current_style_tier;
    }
}

static void apply_mediator(sMediator* mediator, const MediatorState& snap)
{
    mediator->missionID = snap.mission_id;
    mediator->roomID = snap.room_id;
    for (int i = 0; i < 3; ++i) {
        mediator->enemyCount[i] = snap.enemy_count[i];
    }
    mediator->orbCount = snap.orb_count;
    mediator->orbMissionCurrent = snap.orb_mission_current;
    mediator->orbMissionPotential = snap.orb_mission_potential;
    mediator->hellGoldOrbCount = snap.hell_gold_orb_count;
    mediator->gameDifficulty = (GameDifficulty)snap.game_difficulty;
    mediator->gameMode = (GameMode)snap.game_mode;
    mediator->enemyDifficulty = snap.enemy_difficulty;
    mediator->stylePoints = snap.style_points;
    mediator->hasUnlockedDT = (snap.unlock_flags & 1) != 0;
    mediator->hasUnlockedAirHike = (snap.unlock_flags & 2) != 0;
    mediator->hasUnlockedSprint = (snap.unlock_flags & 4) != 0;
    mediator->hasUnlockedHold = (snap.unlock_flags & 8) != 0;
    mediator->hasUnlockedMaxAct = (snap.unlock_flags & 16) != 0;
    mediator->displayDisasterGauge = snap.disaster_gauge_display != 0;
    mediator->turboEnabled = snap.turbo_enabled != 0;

    if (sWorkRate* work = devil4_sdk::get_work_rate()) {
        work->game_speed = snap.game_speed;
        work->unk_float2 = snap.work_unk2;
        work->global_speed = snap.global_speed;
        work->player_speed = snap.player_speed;
        work->enemy_speed = snap.enemy_speed;
        work->room_speed = snap.room_speed;
        work->turbo_speed = snap.turbo_speed;
        work->unk_float3 = snap.work_unk3;
    }

    if (sStylishCount* stylish = devil4_sdk::get_stylish_count()) {
        stylish->current_style_tier = snap.stylish_tier;
    }
}

static void capture_camera(uCameraCtrl* camera, CameraState& out)
{
    if (!camera) {
        return;
    }
    out.camera_valid = true;
    out.camera_position = camera->mCameraPos;
    out.camera_up = camera->mCameraUp;
    out.camera_lookat = camera->mTargetPos;
    out.camera_old_position = camera->mOldCameraPos;
    out.camera_old_lookat = camera->mOldTargetPos;
    out.near_plane = camera->mNearPlane;
    out.far_plane = camera->mFarPlane;
    out.aspect = camera->mAspect;
    out.fov = camera->mFov;
    out.old_fov = camera->mOldFov;
    out.roll = camera->mRoll;
    out.old_roll = camera->mOldRoll;
    out.transition_frame = camera->mTransitionFrame;
    out.transition_timer = camera->mTransitionTimer;

    if (cCameraPlayer* player_camera = camera->mpCamPlayer) {
        out.player_camera_valid = true;
        out.player_camera_position = player_camera->mCameraPos;
        out.player_camera_lookat = player_camera->mTargetPos;
        out.player_camera_up = player_camera->mCameraUp;
        out.player_camera_near_plane = player_camera->mNearPlane;
        out.player_camera_far_plane = player_camera->mFarPlane;
        out.player_camera_aspect = player_camera->mAspect;
        out.player_camera_fov = player_camera->mFov;
        out.player_camera_roll = player_camera->mRoll;
    }
}

static void apply_camera(uCameraCtrl* camera, const CameraState& snap)
{
    if (!camera || !snap.camera_valid) {
        return;
    }
    camera->mCameraPos = snap.camera_position;
    camera->mCameraUp = snap.camera_up;
    camera->mTargetPos = snap.camera_lookat;
    camera->mOldTargetPos = snap.camera_old_lookat;
    camera->mOldCameraPos = snap.camera_old_position;
    camera->mNearPlane = snap.near_plane;
    camera->mFarPlane = snap.far_plane;
    camera->mAspect = snap.aspect;
    camera->mFov = snap.fov;
    camera->mOldFov = snap.old_fov;
    camera->mRoll = snap.roll;
    camera->mOldRoll = snap.old_roll;
    camera->mTransitionFrame = snap.transition_frame;
    camera->mTransitionTimer = snap.transition_timer;

    if (!snap.player_camera_valid) {
        return;
    }
    if (cCameraPlayer* player_camera = camera->mpCamPlayer) {
        player_camera->mCameraPos = snap.player_camera_position;
        player_camera->mTargetPos = snap.player_camera_lookat;
        player_camera->mCameraUp = snap.player_camera_up;
        player_camera->mNearPlane = snap.player_camera_near_plane;
        player_camera->mFarPlane = snap.player_camera_far_plane;
        player_camera->mAspect = snap.player_camera_aspect;
        player_camera->mFov = snap.player_camera_fov;
        player_camera->mRoll = snap.player_camera_roll;
    }
}

static void capture_pad(PadState& out)
{
    if (sDevil4Pad* pad = *(sDevil4Pad**)kGamePadAddress) {
        const cPadInfo& info = pad->mPadInfo[0];
        out.buttons[0] = info.mBtn.on;
        out.buttons[1] = info.mBtn.trg;
        out.buttons[2] = info.mBtn.rel;
        out.buttons[3] = info.mBtn.rep;
        std::memcpy(out.analog, info.mAnlg, sizeof out.analog);
    }
}

static void apply_pad(const PadState& snap)
{
    if (sDevil4Pad* pad = *(sDevil4Pad**)kGamePadAddress) {
        cPadInfo& info = pad->mPadInfo[0];
        info.mBtn.on = snap.buttons[0];
        info.mBtn.trg = snap.buttons[1];
        info.mBtn.rel = snap.buttons[2];
        info.mBtn.rep = snap.buttons[3];
        std::memcpy(info.mAnlg, snap.analog, sizeof info.mAnlg);
    }
}

// ---------------------------------------------------------------- the whole battle

bool capture_battle(sDevil4Main* main, uint64_t sim_time_us, BattleSnapshot& out, const char*& reason)
{
    // No whole-record memset: a braced assignment materializes a whole BattleSnapshot
    // temporary on the stack, and the joint cursors pushed that past the 1 MiB game stack -
    // and zeroing the ~4 MiB record on every save costs more than the pack itself at nine
    // saves per presented frame. Everything below is fully rewritten per capture except the
    // camera and the pad, which are only captured when present; every present unit's record
    // clears itself (capture_player / capture_enemy -> capture_model), and unit records
    // beyond the counts are never packed or read back.
    out.magic = kWireMagic;
    out.version = kWireVersion;
    out.player_count = 0;
    out.enemy_count = 0;
    out.timing = {};
    out.mediator = {};
    out.rng = {};
    out.camera = {};
    out.pad = {};

    sUnit* unit_system = devil4_sdk::get_sUnit();
    sMediator* mediator = devil4_sdk::get_sMediator();
    if (!unit_system || !mediator) {
        reason = "sUnit or sMediator is missing";
        return false;
    }

    capture_timing(main, sim_time_us, out.timing);
    capture_mediator(mediator, out.mediator);
    // the generator the tick draws from: mt[624] then mti, the layout sdk/MtRandom.hpp documents
    std::memcpy(out.rng.mt, (const void*)kGameRngAddress, sizeof out.rng.mt);
    out.rng.mti = *(const int32_t*)(kGameRngAddress + sizeof out.rng.mt);

    if (uCameraCtrl* camera = devil4_sdk::get_local_camera()) {
        capture_camera(camera, out.camera);
    }
    capture_pad(out.pad);

    int walked = 0;
    for (cUnit* unit = (cUnit*)unit_system->mMoveLine[kLinePlayer].mTop;
        unit && walked < kMaxLineWalk; unit = unit->mp_next_unit, ++walked) {
        if (!is_player_unit(unit)) {
            continue;
        }
        if (out.player_count >= kMaxPlayers) {
            reason = "more players on the line than the snapshot holds";
            return false;
        }
        if (!capture_player((uPlayer_old*)unit, out.players[out.player_count])) {
            reason = "player capture failed";
            return false;
        }
        ++out.player_count;
    }

    walked = 0;
    for (cUnit* unit = (cUnit*)unit_system->mMoveLine[kLineEnemy].mTop;
        unit && walked < kMaxLineWalk; unit = unit->mp_next_unit, ++walked) {
        if (out.enemy_count >= kMaxEnemies) {
            reason = "more enemies on the line than the snapshot holds";
            return false;
        }
        if (!capture_enemy((uEnemy_Old*)unit, out.enemies[out.enemy_count])) {
            reason = "enemy capture failed";
            return false;
        }
        ++out.enemy_count;
    }

    reason = nullptr;
    return true;
}

// The live units are re-bound by move-line order; the ids and counts are checked first so a
// restore into a battle that no longer matches is refused instead of silently mis-slotted.
bool restore_battle(sDevil4Main* main, const BattleSnapshot& snap, uint64_t& sim_time_us,
    const char*& reason)
{
    sUnit* unit_system = devil4_sdk::get_sUnit();
    sMediator* mediator = devil4_sdk::get_sMediator();
    if (!unit_system || !mediator) {
        reason = "sUnit or sMediator is missing";
        return false;
    }
    if (snap.magic != kWireMagic || snap.version != kWireVersion) {
        reason = "state was not written by this snapshot format";
        return false;
    }
    if (mediator->missionID != snap.mediator.mission_id || mediator->roomID != snap.mediator.room_id) {
        reason = "mission / room changed since the state was saved";
        return false;
    }
    cUnit* player_line = (cUnit*)unit_system->mMoveLine[kLinePlayer].mTop;
    cUnit* enemy_line = (cUnit*)unit_system->mMoveLine[kLineEnemy].mTop;
    uCameraCtrl* camera = devil4_sdk::get_local_camera();
    if ((snap.camera.camera_valid && !camera) ||
        (snap.camera.player_camera_valid && (!camera || !camera->mpCamPlayer))) {
        reason = "a camera present in the state is missing";
        return false;
    }

    // Preflight every identity and binding before the first write. Mediator enemyCount is state,
    // not identity: it legitimately advances after an older save and apply_mediator rewinds it.
    uint32_t players_seen = 0, enemies_seen = 0;
    int walked = 0;
    for (cUnit* unit = player_line; unit && walked < kMaxLineWalk; unit = unit->mp_next_unit, ++walked) {
        if (is_player_unit(unit)) {
            ++players_seen;
        }
    }
    walked = 0;
    for (cUnit* unit = enemy_line; unit && walked < kMaxLineWalk; unit = unit->mp_next_unit, ++walked) {
        ++enemies_seen;
    }
    if (players_seen != snap.player_count || enemies_seen != snap.enemy_count) {
        reason = "unit counts changed since the state was saved";
        return false;
    }

    walked = 0;
    uint32_t index = 0;
    for (cUnit* unit = player_line; unit && walked < kMaxLineWalk && index < snap.player_count;
        unit = unit->mp_next_unit, ++walked) {
        if (!is_player_unit(unit)) {
            continue;
        }
        const uPlayer_old* player = (const uPlayer_old*)unit;
        if (snap.players[index].controller_id != player->controllerID ||
            snap.players[index].pad_id != player->padID) {
            reason = "player line order changed since the state was saved";
            return false;
        }
        if (!model_assets_available((const uModel*)player, snap.players[index].model)) {
            reason = "a player's saved motion asset is missing";
            return false;
        }
        ++index;
    }

    walked = 0;
    index = 0;
    for (cUnit* unit = enemy_line; unit && walked < kMaxLineWalk && index < snap.enemy_count;
        unit = unit->mp_next_unit, ++walked) {
        const EnemyState& enemy = snap.enemies[index];
        if (enemy.id != (int32_t)((uEnemy_Old*)unit)->ID) {
            reason = "enemy line order changed since the state was saved";
            return false;
        }
        if (enemy.coll_valid && !get_enemy_collision_manager((uEnemy_Old*)unit)) {
            reason = "enemy collision layout is unsupported";
            return false;
        }
        if (!model_assets_available((const uModel*)unit, enemy.model)) {
            reason = "an enemy's saved motion asset is missing";
            return false;
        }
        if (enemy.gate.target_kind == 1) {
            bool target_found = false;
            int player_walked = 0;
            for (cUnit* target = player_line; target && player_walked < kMaxLineWalk;
                target = target->mp_next_unit, ++player_walked) {
                if (is_player_unit(target) && ((uPlayer_old*)target)->padID == enemy.gate.target_player) {
                    target_found = true;
                    break;
                }
            }
            if (!target_found) {
                reason = "an enemy's target player is missing";
                return false;
            }
        }
        ++index;
    }

    walked = 0;
    index = 0;
    for (cUnit* unit = player_line; unit && walked < kMaxLineWalk && index < snap.player_count;
        unit = unit->mp_next_unit, ++walked) {
        if (is_player_unit(unit)) {
            apply_player((uPlayer_old*)unit, snap.players[index++]);
        }
    }
    walked = 0;
    index = 0;
    for (cUnit* unit = enemy_line; unit && walked < kMaxLineWalk && index < snap.enemy_count;
        unit = unit->mp_next_unit, ++walked) {
        apply_enemy((uEnemy_Old*)unit, snap.enemies[index++]);
    }

    apply_mediator(mediator, snap.mediator);
    std::memcpy((void*)kGameRngAddress, snap.rng.mt, sizeof snap.rng.mt);
    *(int32_t*)(kGameRngAddress + sizeof snap.rng.mt) = snap.rng.mti;

    if (camera) {
        apply_camera(camera, snap.camera);
    }
    apply_pad(snap.pad);
    apply_timing(main, snap.timing, sim_time_us);
    reason = nullptr;
    return true;
}

// ---------------------------------------------------------------- buffer <-> snapshot

bool pack_into(unsigned char* buffer, uint32_t capacity, uint32_t& written,
    BattleSnapshot& snap, const char*& reason)
{
    zpp::bits::out<std::span<unsigned char>> out{std::span<unsigned char>{buffer, capacity}};
    const auto result = wire(snap, out);
    if (zpp::bits::failure(result)) {
        written = (uint32_t)out.position();   // where the write stopped, for the diagnostic
        reason = "state does not fit the session buffer";
        return false;
    }
    written = (uint32_t)out.position();
    reason = nullptr;
    return true;
}

bool unpack_from(const unsigned char* buffer, uint32_t length, BattleSnapshot& snap,
    const char*& reason)
{
    std::memset(&snap, 0, sizeof snap);   // see capture_battle: no whole-state stack temporary
    zpp::bits::in<std::span<const unsigned char>> in{std::span<const unsigned char>{buffer, length}};
    const auto result = wire(snap, in);
    if (zpp::bits::failure(result)) {
        reason = "state is truncated or corrupt";
        return false;
    }
    if (snap.player_count > kMaxPlayers || snap.enemy_count > kMaxEnemies) {
        reason = "state holds more units than this build can restore";
        return false;
    }
    if (snap.magic != kWireMagic || snap.version != kWireVersion) {
        reason = "state was not written by this build's snapshot format";
        return false;
    }
    reason = nullptr;
    return true;
}

// GekkoNet hands the game back the checksum it wants for the saved frame; the game picks the
// function, so it only has to be stable and cover exactly the bytes that travel.
uint32_t checksum_of(const unsigned char* bytes, uint32_t length)
{
    // CRC-32/IEEE, reflected, polynomial 0xEDB88320; the table is built once on first use.
    static const std::array<uint32_t, 256> table = [] {
        std::array<uint32_t, 256> values{};
        for (uint32_t n = 0; n < 256u; ++n) {
            uint32_t c = n;
            for (int k = 0; k < 8; ++k) {
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            }
            values[n] = c;
        }
        return values;
    }();
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < length; ++i) {
        crc = table[(crc ^ bytes[i]) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}

} // namespace netplay_snapshot
