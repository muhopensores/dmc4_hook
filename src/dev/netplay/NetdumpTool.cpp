// netdump_tool - standalone decoder/differ for BattleSnapshot wire dumps (netdump\*.bin).
//
// The field tables below are transcribed verbatim from BattleSnapshot.cpp's wire() functions
// (same field order, same types), so a decoded offset names the exact field the checksum saw.
// The wire is packed (zpp::bits writes no padding), so the compile-time check compares each
// table against a #pragma pack(1) mirror of the matching BattleSnapshot.hpp struct: if the
// table's field order/count/types drift from the struct declaration, the static_asserts fire.
// The walk self-validates at run time too: it reads each unit's motion_count / joint_count
// from the bytes like unpack_from does and must land exactly on the file size.
//
// Build:
//   cl /nologo /std:c++17 /EHsc /O2 src\dev\netplay\NetdumpTool.cpp /Fe:netdump_tool.exe
//
// Usage:
//   netdump_tool map <file>               offset/field table + walk validation
//   netdump_tool diff <A.bin> <B.bin>     differing bytes mapped to fields, values decoded
//   netdump_tool summary <file>           key state values (counts, clock, rng, units)
//   netdump_tool scan [dir]           group dumps by frame and diff every same-frame pair

#include <cstdint>
#include <cstdio>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>

static constexpr size_t kRngWords = 624;
static constexpr size_t kCollCorrectWords = 23;
static constexpr size_t kMaxMotionSlots = 8;
static constexpr size_t kMaxMotionJoints = 192;
static constexpr size_t kMaxPlayers = 8;    // BattleSnapshot.hpp's
static constexpr size_t kMaxEnemies = 64;
static constexpr size_t kMaxParts = 32;
static constexpr uint8_t kPartActor = 2;    // PartState::kind: a model follows the record
static constexpr uint32_t kNoMotionTrack = 0xFFFFFFFFu;

enum Type { U8, U16, U32, I32, F32, U64, Bool };

static constexpr size_t type_size(Type t) {
    switch (t) {
    case U8: case Bool: return 1;
    case U16: return 2;
    case U32: case I32: case F32: return 4;
    case U64: return 8;
    }
    return 0;
}

static const char* type_name(Type t) {
    switch (t) {
    case U8: return "u8";
    case U16: return "u16";
    case U32: return "u32";
    case I32: return "i32";
    case F32: return "f32";
    case U64: return "u64";
    case Bool: return "bool";
    }
    return "?";
}

struct FieldDef { const char* name; Type type; int count; };

constexpr size_t table_size(const FieldDef* t) {
    size_t s = 0;
    for (int i = 0; t[i].name != nullptr; ++i) {
        s += type_size(t[i].type) * (size_t)t[i].count;
    }
    return s;
}

#define F(name, type) { name, type, 1 }
#define A(name, type, n) { name, type, n }
#define END { nullptr, U8, 0 }

static constexpr FieldDef kHeader[] = {
    F("magic", U32), F("version", U32), F("player_count", U32), F("enemy_count", U32),
    F("part_count", U32), F("part_overflow", U32), END };

// --- PartState (BattleSnapshot.cpp wire(PartState), struct hpp) --------------------------
static constexpr FieldDef kPart[] = {
    F("line", U8), F("kind", U8), F("nth", U16), F("class_id", U32), F("unit_flags", U16),
    F("delta", F32), F("state", U32), A("position", F32, 3), A("quat", F32, 4), A("scale", F32, 3),
    F("damage_valid", Bool), F("hp", F32), F("hp_max", F32), F("hp_taken", F32),
    A("stun", I32, 5), A("displacement", I32, 5), F("is_dead", Bool),
    F("coll_valid", Bool), F("coll_land", U32), F("coll_flags", U32),
    A("coll_correct", U32, (int)kCollCorrectWords), END };

// --- TimingState (BattleSnapshot.cpp:56, struct hpp:82) ----------------------------------
static constexpr FieldDef kTiming[] = {
    F("sim_time_us", U64), F("main_timer", U64), F("frame_timer", I32),
    F("fps", F32), F("fps_lock", F32), F("fps_actual", F32),
    F("wanna_skip_frame", U32), F("skip", U32), F("pause_flags", U8),
    F("delta_time_norm", F32), F("delta_time_max", F32), F("delta_time_max2", F32),
    F("speed_global", F32), F("delay_frame", F32), END };

// --- MediatorState (BattleSnapshot.cpp:66, struct hpp:99) --------------------------------
static constexpr FieldDef kMediator[] = {
    F("mission_id", U32), F("room_id", U32), A("enemy_count", U32, 3),
    F("orb_count", U32), F("orb_mission_current", U32), F("orb_mission_potential", U32),
    F("hell_gold_orb_count", U32), F("game_difficulty", U32), F("game_mode", U32),
    F("enemy_difficulty", F32), F("style_points", F32),
    F("unlock_flags", U8), F("disaster_gauge_display", U8), F("turbo_enabled", U8),
    F("game_speed", F32), F("work_unk2", F32), F("global_speed", F32), F("player_speed", F32),
    F("enemy_speed", F32), F("room_speed", F32), F("turbo_speed", F32), F("work_unk3", F32),
    F("stylish_tier", U32), END };

// --- RngState (BattleSnapshot.cpp:229, struct hpp:385) -----------------------------------
static constexpr FieldDef kRng[] = { F("mti", I32), A("mt", U32, (int)kRngWords), END };

// --- CameraState (BattleSnapshot.cpp:77, struct hpp:127) ---------------------------------
static constexpr FieldDef kCamera[] = {
    F("camera_valid", Bool),
    A("camera_position", F32, 3), A("camera_up", F32, 3), A("camera_lookat", F32, 3),
    A("camera_old_position", F32, 3), A("camera_old_lookat", F32, 3),
    F("near_plane", F32), F("far_plane", F32), F("aspect", F32), F("fov", F32),
    F("old_fov", F32), F("roll", F32), F("old_roll", F32),
    F("transition_frame", I32), F("transition_timer", F32),
    F("player_camera_valid", Bool),
    A("player_camera_position", F32, 3), A("player_camera_lookat", F32, 3),
    A("player_camera_up", F32, 3),
    F("player_camera_near_plane", F32), F("player_camera_far_plane", F32),
    F("player_camera_aspect", F32), F("player_camera_fov", F32), F("player_camera_roll", F32),
    END };

// --- PadState (BattleSnapshot.hpp:390, wire cpp:289) -------------------------------------
static constexpr FieldDef kPad[] = { A("buttons", U32, 4), A("analog", F32, 4), END };

// --- PlayerState (BattleSnapshot.cpp:150 wire list, struct hpp:247) ----------------------
static constexpr FieldDef kPlayer[] = {
    F("controller_id", U32), F("pad_id", U8), F("is_active", Bool), F("unit_flags", U32),
    F("unit_delta_time", F32),
    A("position", F32, 3), A("velocity", F32, 3),
    F("rotation2", F32), F("rotation3", F32), F("facing_direction", F32),
    F("ground_inertia_x", F32), F("inertia_y", F32), F("ground_inertia_z", F32),
    F("ground_inertia", F32), F("inertia", F32), F("inertia_direction_real", F32),
    F("vel_magnitude", F32), F("direction_of_magnitude", F32),
    F("anim_id", U16), F("anim_frame", F32), F("anim_frame_max", F32),
    F("move_bank", U32), F("move_part", U32), F("move_id2", U32), F("move_id3", U32),
    F("move_id_best", U32), F("walk_type", U32),
    F("grounded", U8), F("grounded2_flag", U8), F("air_attack_count", U8),
    F("air_hike_count", U8), F("weight", U8),
    F("can_air_hike", Bool), F("can_wall_hike", Bool),
    F("jc_timer", F32), F("round_trip_charge", F32), F("character_speed", F32),
    F("hitstop", Bool), F("hitstop_timer", F32),
    F("hp", F32), F("hp_max", F32), F("hp_taken", F32), F("prev_damage_resist", F32),
    A("stun", I32, 5), A("displacement", I32, 5),
    F("is_dead", Bool), F("stun_reset_timer", F32),
    F("current_style", I32), F("dt", F32), F("max_dt", F32),
    F("dt_active", Bool), F("dt_outfit", Bool),
    F("exceed_level", U8), F("exceed_timer", F32),
    F("can_exceed", Bool), F("is_exceeding", Bool),
    F("guard_timer", F32), F("revenge_gauge", F32), F("disaster_gauge", F32),
    F("dreadnaught", I32), F("next_sword", I32),
    F("next_gun", U32), F("current_sword", U32), F("current_gun", U32),
    F("can_weapon_change", U32), F("is_lockon_animation", U32), F("locked_on", Bool),
    A("input_hold", U8, 4), A("input_press", U8, 4), A("input_release", U8, 4),
    F("button_input_raw", U16), F("stick_input_raw", U16), F("stick_direction", F32),
    F("stick_past_threshold", Bool), F("buffered_jump", U8),
    F("tilt_forward", Bool), F("tilt_back", Bool), F("tilt_back_forward", Bool),
    A("cancels1", U8, 4), F("cancels2", U32),
    A("melee_cancel_permissions1", U8, 4), A("jump_cancel_permissions1", U8, 4),
    F("melee_cancel_permissions2", U8), F("jump_cancel_permissions2", U8),
    F("gun_cancel_permissions2", U8), F("buffer_permissions", U8),
    F("directional_melee_permissions", U8),
    F("trigger_big_jump", Bool), F("trigger_stinger_jump", Bool), END };

// --- EnemyState (BattleSnapshot.cpp:186 wire list, struct hpp:323) -----------------------
static constexpr FieldDef kEnemy[] = {
    F("id", I32), F("team", I32), F("is_active", Bool), F("in_battle", Bool),
    F("unit_flags", U32), F("unit_delta_time", F32), F("move_id", U8), F("move_part", U8),
    A("position", F32, 3), A("velocity", F32, 3),
    A("actor_velocity", F32, 3), A("actor_acceleration", F32, 3),
    A("target_position", F32, 3), A("rotation", F32, 3), A("scale", F32, 3),
    F("quat_w", F32),
    A("knockback_position", F32, 3), A("knockback_rotation", F32, 3),
    F("anim_id", U16), F("anim_frame", F32), F("anim_frame_max", F32),
    F("dt", Bool), F("dt_ok", U8), F("dt_timer_enabled", U8), F("dt_sign", U8),
    F("dt_timer", F32),
    F("berial_fire", F32), F("berial_fire_timer", F32),
    F("gladius_timer", F32), F("gladius_buried", Bool),
    F("faust_cloak", F32), F("faust_cloak_timer", F32),
    F("blitz_electric", F32), F("blitz_electric_timer", F32),
    F("blitz_electric_suicide_timer", F32),
    F("angelo_shield", F32), F("credo_shield", F32), F("credo_shield_timer", F32),
    F("sanctus_shield_m11", F32), F("sanctus_shield_timer_m11", F32),
    F("sanctus_shield_m20", F32), F("sanctus_shield_timer_m20", F32),
    F("frost_heal_count", I32), F("damage_valid", Bool),
    F("hp", F32), F("hp_max", F32), F("hp_taken", F32), F("prev_damage_resist", F32),
    A("stun", I32, 5), A("displacement", I32, 5),
    F("is_dead", Bool), F("stun_reset_timer", F32),
    F("coll_valid", Bool), F("coll_land", U32), F("coll_flags", U32),
    A("coll_correct", U32, (int)kCollCorrectWords), END };

// --- BattleGateState (hpp:231, wire BattleSnapshot.cpp:233) ------------------------------
static constexpr FieldDef kGate[] = {
    F("gate.actor_status", U32), F("gate.mode", U32), F("gate.battle_flag", U8),
    F("gate.target_kind", U8), F("gate.target_player", U8),
    A("gate.lockon_pos", F32, 3),
    F("gate.lockon_range", F32), F("gate.lockon_max_hp", F32), F("gate.lockon_hp", F32),
    F("gate.lockon_pi_length", F32),
    F("gate.lockon_is_in", U8), F("gate.lockon_is_target", U8), F("gate.lockon_is_catch", U8),
    F("gate.lockon_camera_in", U8), F("gate.lockon_no_target", U8),
    F("gate.lockon_inter_target", U8), F("gate.lockon_no_inter", U8), END };

// --- MotionSlotState (BattleSnapshot.cpp:101, wire version 16) ---------------------------
// pad38[0..1] and prev_trans[0] are live per-process pointers and left the wire in v16.
static constexpr FieldDef kMotion[] = {
    F("motion_no", U16), F("reserved", U16), F("state", U16), F("attr", U16),
    F("inter_frame", F32), F("inter_count", F32), F("blend", F32),
    F("frame", F32), F("frame_max", F32), F("loop_frame", F32), F("speed", F32),
    F("inter_rate", F32), F("next_frame", F32),
    A("inter_curve", F32, 2),
    A("prev_quat", F32, 4), A("base_quat", F32, 4), A("null_quat", F32, 4),
    F("prev_trans.y", F32), F("prev_trans.z", F32),
    A("base_trans", F32, 3), A("null_trans", F32, 3),
    F("trans_frame", F32), F("trans_weight", F32), F("quat_frame", F32), F("quat_weight", F32),
    F("trans_cursor_offset", U32), F("quat_cursor_offset", U32),
    A("sequence", I32, 2), A("prev_sequence", I32, 2), END };

// --- JointCursorState (BattleSnapshot.cpp:93, struct hpp:151) ----------------------------
static constexpr FieldDef kJoint[] = {
    F("quat_offset", U32), F("trans_offset", U32), F("scale_offset", U32),
    F("quat_frame", F32), F("trans_frame", F32), F("scale_frame", F32),
    F("quat_weight", F32), F("trans_weight", F32), F("scale_weight", F32), END };

// ModelState fixed head (hpp:184): motion_count u8, joint_count u16, 2 vectors, 2 quats,
// continue_words[4] u32 - the mContinueCount block plus the 15 bytes the SDK calls padding0.
static constexpr FieldDef kModelHead[] = {
    F("model.motion_count", U8), F("model.joint_count", U16),
    A("model.now_frame_pos", F32, 3), A("model.old_frame_pos", F32, 3),
    A("model.now_frame_quat", F32, 4), A("model.old_frame_quat", F32, 4),
    A("model.continue_words", U32, 4), END };

static constexpr FieldDef kTickTimer[] = { F("tick_timer", F32), END };

// ---------------------------------------------------------------- compile-time validation
//
// Packed mirrors of the BattleSnapshot.hpp structs, transcribed in declaration order. The
// static_asserts tie the wire tables to the struct declarations: a reordered/retyped field
// in either changes a sizeof and breaks the build.

#pragma pack(push, 1)

struct TimingX {
    uint64_t sim_time_us;
    uint64_t main_timer;
    int32_t frame_timer;
    float fps, fps_lock, fps_actual;
    uint32_t wanna_skip_frame, skip;
    uint8_t pause_flags;
    float delta_time_norm, delta_time_max, delta_time_max2, speed_global, delay_frame;
};

struct MediatorX {
    uint32_t mission_id, room_id;
    uint32_t enemy_count[3];
    uint32_t orb_count, orb_mission_current, orb_mission_potential, hell_gold_orb_count;
    uint32_t game_difficulty, game_mode;
    float enemy_difficulty, style_points;
    uint8_t unlock_flags, disaster_gauge_display, turbo_enabled;
    float game_speed, work_unk2, global_speed, player_speed, enemy_speed, room_speed,
        turbo_speed, work_unk3;
    uint32_t stylish_tier;
};

struct RngX { int32_t mti; uint32_t mt[kRngWords]; };

struct Vec3X { float x, y, z; };
struct Vec4X { float x, y, z, w; };

struct CameraX {
    bool camera_valid;
    Vec3X camera_position, camera_up, camera_lookat;
    Vec3X camera_old_position, camera_old_lookat;
    float near_plane, far_plane, aspect, fov, old_fov, roll, old_roll;
    int32_t transition_frame;
    float transition_timer;
    bool player_camera_valid;
    Vec3X player_camera_position, player_camera_lookat, player_camera_up;
    float player_camera_near_plane, player_camera_far_plane, player_camera_aspect,
        player_camera_fov, player_camera_roll;
};

struct PadX { uint32_t buttons[4]; float analog[4]; };

struct JointX {
    uint32_t quat_offset, trans_offset, scale_offset;
    float quat_frame, trans_frame, scale_frame;
    float quat_weight, trans_weight, scale_weight;
};

// Wire layout (v16): the SDK's pad38 pair and prev_trans.x are not serialized.
struct MotionX {
    uint16_t motion_no, reserved, state, attr;
    float inter_frame, inter_count, blend;
    float frame, frame_max, loop_frame, speed;
    float inter_rate, next_frame;
    float inter_curve[2];
    float prev_quat[4], base_quat[4], null_quat[4];
    float prev_trans[2];
    float base_trans[3], null_trans[3];
    float trans_frame, trans_weight, quat_frame, quat_weight;
    uint32_t trans_cursor_offset, quat_cursor_offset;
    int32_t sequence[2], prev_sequence[2];
    JointX joints[kMaxMotionJoints];
};

struct ModelX {
    uint8_t motion_count;
    uint16_t joint_count;
    Vec3X now_frame_pos, old_frame_pos;
    Vec4X now_frame_quat, old_frame_quat;
    uint32_t continue_words[4];
    MotionX motion[kMaxMotionSlots];
};

struct PlayerX {
    uint32_t controller_id;
    uint8_t pad_id;
    bool is_active;
    uint32_t unit_flags;
    float unit_delta_time;
    Vec3X position, velocity;
    float rotation2, rotation3, facing_direction;
    float ground_inertia_x, inertia_y, ground_inertia_z;
    float ground_inertia, inertia, inertia_direction_real;
    float vel_magnitude, direction_of_magnitude;
    uint16_t anim_id;
    float anim_frame, anim_frame_max;
    uint32_t move_bank, move_part, move_id2, move_id3, move_id_best, walk_type;
    uint8_t grounded, grounded2_flag, air_attack_count, air_hike_count, weight;
    bool can_air_hike, can_wall_hike;
    float jc_timer, round_trip_charge, character_speed;
    bool hitstop;
    float hitstop_timer;
    float hp, hp_max, hp_taken, prev_damage_resist;
    int32_t stun[5], displacement[5];
    bool is_dead;
    float stun_reset_timer;
    int32_t current_style;
    float dt, max_dt;
    bool dt_active, dt_outfit;
    uint8_t exceed_level;
    float exceed_timer;
    bool can_exceed, is_exceeding;
    float guard_timer, revenge_gauge, disaster_gauge;
    int32_t dreadnaught, next_sword;
    uint32_t next_gun, current_sword, current_gun;
    uint32_t can_weapon_change, is_lockon_animation;
    bool locked_on;
    uint8_t input_hold[4], input_press[4], input_release[4];
    uint16_t button_input_raw, stick_input_raw;
    float stick_direction;
    bool stick_past_threshold;
    uint8_t buffered_jump;
    bool tilt_forward, tilt_back, tilt_back_forward;
    uint8_t cancels1[4];
    uint32_t cancels2;
    uint8_t melee_cancel_permissions1[4], jump_cancel_permissions1[4];
    uint8_t melee_cancel_permissions2, jump_cancel_permissions2, gun_cancel_permissions2,
        buffer_permissions, directional_melee_permissions;
    bool trigger_big_jump, trigger_stinger_jump;
    ModelX model;
};

struct EnemyX {
    int32_t id, team;
    bool is_active, in_battle;
    uint32_t unit_flags;
    float unit_delta_time;
    uint8_t move_id, move_part;
    Vec3X position, velocity, actor_velocity, actor_acceleration, target_position, rotation,
        scale;
    float quat_w;
    Vec3X knockback_position, knockback_rotation;
    uint16_t anim_id;
    float anim_frame, anim_frame_max;
    bool dt;
    uint8_t dt_ok, dt_timer_enabled, dt_sign;
    float dt_timer;
    float berial_fire, berial_fire_timer;
    float gladius_timer;
    bool gladius_buried;
    float faust_cloak, faust_cloak_timer;
    float blitz_electric, blitz_electric_timer, blitz_electric_suicide_timer;
    float angelo_shield;
    float credo_shield, credo_shield_timer;
    float sanctus_shield_m11, sanctus_shield_timer_m11;
    float sanctus_shield_m20, sanctus_shield_timer_m20;
    int32_t frost_heal_count;
    bool damage_valid;
    float hp, hp_max, hp_taken, prev_damage_resist;
    int32_t stun[5], displacement[5];
    bool is_dead;
    float stun_reset_timer;
    bool coll_valid;
    uint32_t coll_land, coll_flags;
    uint32_t coll_correct[kCollCorrectWords];
    ModelX model;
    struct GateX {
        uint32_t actor_status, mode;
        uint8_t battle_flag, target_kind, target_player;
        float lockon_pos[3];
        float lockon_range, lockon_max_hp, lockon_hp, lockon_pi_length;
        uint8_t lockon_is_in, lockon_is_target, lockon_is_catch, lockon_camera_in;
        uint8_t lockon_no_target, lockon_inter_target, lockon_no_inter;
    } gate;
    float tick_timer;
};

#pragma pack(pop)

static_assert(table_size(kHeader) == 24, "header drift");
static_assert(table_size(kTiming) == sizeof(TimingX), "kTiming drifted from TimingState");
static_assert(table_size(kMediator) == sizeof(MediatorX), "kMediator drifted from MediatorState");
static_assert(table_size(kRng) == sizeof(RngX), "kRng drifted from RngState");
static_assert(table_size(kCamera) == sizeof(CameraX), "kCamera drifted from CameraState");
static_assert(table_size(kPad) == sizeof(PadX), "kPad drifted from PadState");
static_assert(table_size(kJoint) == sizeof(JointX), "kJoint drifted from JointCursorState");
static_assert(table_size(kMotion) + kMaxMotionJoints * sizeof(JointX) == sizeof(MotionX),
    "kMotion drifted from MotionSlotState");
static_assert(table_size(kModelHead) + kMaxMotionSlots * sizeof(MotionX) == sizeof(ModelX),
    "kModelHead drifted from ModelState");
static_assert(table_size(kPlayer) + sizeof(ModelX) == sizeof(PlayerX),
    "kPlayer drifted from PlayerState");
static_assert(table_size(kEnemy) + sizeof(ModelX) + sizeof(EnemyX::GateX) + 4 == sizeof(EnemyX),
    "kEnemy drifted from EnemyState");
static_assert(table_size(kGate) == sizeof(EnemyX::GateX), "kGate drifted from BattleGateState");

// spot checks: struct offsets against the packed layout the walk relies on
static_assert(offsetof(TimingX, fps) == 20, "timing layout");
static_assert(offsetof(MotionX, prev_quat) == 52, "motion layout (v16)");
static_assert(offsetof(MotionX, prev_trans) == 100, "motion layout (v16)");
static_assert(offsetof(ModelX, continue_words) == 59, "model head layout");
static_assert(offsetof(PlayerX, model) == table_size(kPlayer), "player model offset");
static_assert(offsetof(EnemyX, model) == table_size(kEnemy), "enemy model offset");

// ---------------------------------------------------------------- field map

struct FieldRef { size_t offset; std::string name; Type type; size_t size; };

static void add_table(std::vector<FieldRef>& map, size_t offset, const char* prefix,
    const FieldDef* table) {
    for (int i = 0; table[i].name; ++i) {
        const FieldDef& f = table[i];
        const size_t esize = type_size(f.type);
        for (int e = 0; e < f.count; ++e) {
            char name[256];
            if (f.count > 1) {
                sprintf_s(name, "%s%s[%d]", prefix, f.name, e);
            } else {
                sprintf_s(name, "%s%s", prefix, f.name);
            }
            map.push_back({ offset, name, f.type, esize });
            offset += esize;
        }
    }
}

struct WalkResult {
    size_t end = 0;
    bool ok = false;
    std::vector<FieldRef> map;
    std::string error;
};

static uint32_t le32(const unsigned char* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Reads motion_count / joint_count from the bytes exactly like unpack_from / wire(ModelState).
static WalkResult walk_file(const std::vector<unsigned char>& data, bool build_map) {
    WalkResult w;
    size_t off = 0;

    if (data.size() < 24) {
        w.error = "file too small for the 24-byte header";
        return w;
    }
    const uint32_t players = le32(&data[8]);
    const uint32_t enemies = le32(&data[12]);
    const uint32_t parts = le32(&data[16]);
    if (players > kMaxPlayers || enemies > kMaxEnemies || parts > kMaxParts) {
        w.error = "implausible unit counts in the header";
        return w;
    }

    if (build_map) {
        add_table(w.map, 0, "", kHeader);
    }
    off = 24;

    // advance and optionally record a field table
    auto add = [&](const FieldDef* table, const char* prefix) {
        if (build_map) {
            add_table(w.map, off, prefix, table);
        }
        for (int i = 0; table[i].name; ++i) {
            off += type_size(table[i].type) * (size_t)table[i].count;
        }
    };

    auto add_model = [&](const char* prefix) -> bool {
        if (off + 3 > data.size()) {
            w.error = "file ends inside a unit record";
            return false;
        }
        const uint32_t mc = data[off];
        const uint32_t jc = (uint32_t)data[off + 1] | ((uint32_t)data[off + 2] << 8);
        if (mc > kMaxMotionSlots || jc > kMaxMotionJoints) {
            char msg[160];
            sprintf_s(msg, "implausible motion_count %u / joint_count %u at 0x%zX (%s)",
                mc, jc, off, prefix);
            w.error = msg;
            return false;
        }
        add(kModelHead, prefix);
        for (uint32_t s = 0; s < mc; ++s) {
            char sprefix[128];
            sprintf_s(sprefix, "%smodel.motion[%u].", prefix, s);
            add(kMotion, sprefix);
            for (uint32_t j = 0; j < jc; ++j) {
                char jprefix[160];
                sprintf_s(jprefix, "%smodel.motion[%u].joints[%u].", prefix, s, j);
                add(kJoint, jprefix);
            }
        }
        return true;
    };

    add(kTiming, "timing.");
    add(kMediator, "mediator.");
    add(kRng, "rng.");
    add(kCamera, "camera.");
    add(kPad, "pad.");

    for (uint32_t p = 0; p < players; ++p) {
        char prefix[32];
        sprintf_s(prefix, "players[%u].", p);
        add(kPlayer, prefix);
        if (!add_model(prefix)) {
            return w;
        }
    }

    for (uint32_t e = 0; e < enemies; ++e) {
        char prefix[32];
        sprintf_s(prefix, "enemies[%u].", e);
        add(kEnemy, prefix);
        if (!add_model(prefix)) {
            return w;
        }
        add(kGate, prefix);
        add(kTickTimer, prefix);
    }

    for (uint32_t p = 0; p < parts; ++p) {
        char prefix[32];
        sprintf_s(prefix, "parts[%u].", p);
        if (off + 2 > data.size()) {
            w.error = "file ends inside a part record";
            return w;
        }
        const uint8_t kind = data[off + 1];
        add(kPart, prefix);
        if ((kind & kPartActor) && !add_model(prefix)) {
            return w;
        }
    }

    w.end = off;
    w.ok = off == data.size();
    if (!w.ok) {
        char msg[160];
        sprintf_s(msg, "walk ended at 0x%zX but the file is 0x%zX bytes (delta %zd)",
            off, data.size(), (ptrdiff_t)off - (ptrdiff_t)data.size());
        w.error = msg;
    }
    return w;
}

static bool pointer_shaped(uint32_t v) {
    // game statics 0x00E0xxxx..0x00FAxxxx, heap 0x01000000..0x7FFF0000, 4-aligned
    return v >= 0x00400000u && v <= 0x7FFF0000u && (v & 3u) == 0;
}

static const FieldRef* find_field(const std::vector<FieldRef>& map, size_t offset) {
    // ordered map: binary search for the last entry with offset <= target
    size_t lo = 0, hi = map.size();
    while (lo < hi) {
        const size_t mid = (lo + hi) / 2;
        if (map[mid].offset <= offset) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo == 0) {
        return nullptr;
    }
    const FieldRef& f = map[lo - 1];
    if (offset < f.offset + f.size) {
        return &f;
    }
    return nullptr;
}

static std::string decode(const std::vector<unsigned char>& data, size_t offset, Type t) {
    char buf[128];
    const unsigned char* p = data.data() + offset;
    switch (t) {
    case U8:
        sprintf_s(buf, "%u (0x%02X)", p[0], p[0]);
        break;
    case U16: {
        const uint32_t v = (uint32_t)p[0] | ((uint32_t)p[1] << 8);
        sprintf_s(buf, "%u (0x%04X)", v, v);
        break;
    }
    case U32: {
        const uint32_t v = le32(p);
        sprintf_s(buf, "%u (0x%08X)%s", v, v, pointer_shaped(v) ? " [pointer-shaped]" : "");
        break;
    }
    case I32: {
        const uint32_t u = le32(p);
        int32_t v;
        memcpy(&v, &u, 4);
        sprintf_s(buf, "%d (0x%08X)", v, u);
        break;
    }
    case F32: {
        float v;
        memcpy(&v, p, 4);
        sprintf_s(buf, "%.6g", (double)v);
        break;
    }
    case U64: {
        uint64_t v;
        memcpy(&v, p, 8);
        sprintf_s(buf, "%llu (0x%016llX)", (unsigned long long)v, (unsigned long long)v);
        break;
    }
    case Bool:
        sprintf_s(buf, "%s", p[0] ? "true" : "false");
        break;
    default:
        buf[0] = 0;
        break;
    }
    return buf;
}

static std::vector<unsigned char> read_file(const char* path) {
    FILE* f = nullptr;
    fopen_s(&f, path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open %s\n", path);
        exit(1);
    }
    std::vector<unsigned char> data;
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    data.resize((size_t)size);
    if (size > 0 && fread(data.data(), 1, data.size(), f) != (size_t)size) {
        fprintf(stderr, "short read on %s\n", path);
        exit(1);
    }
    fclose(f);
    return data;
}

static int cmd_map(const char* path) {
    const std::vector<unsigned char> data = read_file(path);
    const WalkResult w = walk_file(data, true);
    printf("file %s: 0x%zX bytes\n", path, data.size());
    for (const FieldRef& f : w.map) {
        printf("0x%04zX %-70s %-4s = %s\n", f.offset, f.name.c_str(), type_name(f.type),
            decode(data, f.offset, f.type).c_str());
    }
    if (w.ok) {
        printf("walk OK: ends exactly at the file size (0x%zX)\n", w.end);
        return 0;
    }
    printf("walk MISMATCH: %s\n", w.error.c_str());
    return 2;
}

static int cmd_diff(const char* pa, const char* pb) {
    const std::vector<unsigned char> a = read_file(pa);
    const std::vector<unsigned char> b = read_file(pb);
    if (a.size() != b.size()) {
        printf("sizes differ: %zu vs %zu - layout changed, diff is unreliable\n", a.size(), b.size());
    }
    const WalkResult wa = walk_file(a, true);
    const WalkResult wb = walk_file(b, true);
    printf("A %s: 0x%zX bytes (%s)\n", pa, a.size(), wa.ok ? "walk OK" : wa.error.c_str());
    printf("B %s: 0x%zX bytes (%s)\n", pb, b.size(), wb.ok ? "walk OK" : wb.error.c_str());
    if (!wa.ok || !wb.ok) {
        printf("NOTE(): a walk mismatch means this tool's tables drifted from BattleSnapshot\n");
    }

    std::vector<size_t> diffs;
    const size_t n = a.size() < b.size() ? a.size() : b.size();
    for (size_t i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            diffs.push_back(i);
        }
    }
    printf("%zu differing bytes\n", diffs.size());
    if (diffs.empty()) {
        return 0;
    }

    // group consecutive diffs that share one field element
    for (size_t i = 0; i < diffs.size();) {
        const FieldRef* f = find_field(wa.map, diffs[i]);
        size_t j = i + 1;
        while (j < diffs.size() && find_field(wa.map, diffs[j]) == f) {
            ++j;
        }
        if (!f) {
            printf("0x%04zX: <outside the walk - transcription drift>\n", diffs[i]);
        } else {
            printf("0x%04zX %-70s :", f->offset, f->name.c_str());
            for (size_t k = i; k < j; ++k) {
                printf(" %s vs %s", decode(a, diffs[k], f->type).c_str(),
                    decode(b, diffs[k], f->type).c_str());
            }
            printf("\n");
        }
        i = j;
    }
    return 0;
}

static const FieldRef* find_exact(const std::vector<FieldRef>& map, const char* name) {
    for (const FieldRef& f : map) {
        if (f.name == name) {
            return &f;
        }
    }
    return nullptr;
}

static int cmd_summary(const char* path) {
    const std::vector<unsigned char> data = read_file(path);
    const WalkResult w = walk_file(data, true);
    printf("file %s: 0x%zX bytes - walk %s%s\n", path, data.size(), w.ok ? "OK" : "MISMATCH",
        w.error.c_str());
    if (!w.ok) {
        return 2;
    }
    for (const char* name : { "magic", "version", "player_count", "enemy_count",
        "timing.sim_time_us", "timing.main_timer", "timing.frame_timer", "timing.fps",
        "timing.fps_lock", "timing.fps_actual", "mediator.mission_id", "mediator.room_id",
        "rng.mti", "camera.camera_valid", "pad.buttons[0]", "pad.buttons[1]" }) {
        const FieldRef* f = find_exact(w.map, name);
        if (f) {
            printf("  %-24s = %s\n", name, decode(data, f->offset, f->type).c_str());
        }
    }
    for (const FieldRef& f : w.map) {
        const bool is_unit_pos = f.name.find(".position[") != std::string::npos &&
            (f.name.find("players[") == 0 || f.name.find("enemies[") == 0);
        const bool is_id = f.name.find(".pad_id") != std::string::npos ||
            f.name.find(".controller_id") != std::string::npos ||
            f.name.find(".id[") != std::string::npos ||
            f.name == "enemies[0].id" || f.name == "enemies[1].id" || f.name == "enemies[2].id";
        const bool is_model_counts = f.name.find("model.motion_count") != std::string::npos ||
            f.name.find("model.joint_count") != std::string::npos;
        const bool is_motion_frame = f.name.find("model.motion[0].frame ") == 0 ||
            (f.name.find("model.motion[0].frame") != std::string::npos &&
             f.name.find("max") == std::string::npos &&
             f.name.find("inter") == std::string::npos);
        if (is_unit_pos || is_id || is_model_counts || is_motion_frame) {
            printf("  %-70s = %s\n", f.name.c_str(), decode(data, f.offset, f.type).c_str());
        }
    }
    return 0;
}

// scan: pair the dumps without anyone picking filenames by hand. Both CRCs a GekkoDesyncDetected
// event carries come from THIS process - the ring entry's current checksum and the one GekkoNet
// recorded in _checksum_history - so the files that share a frame and differ in checksum are exactly
// the two sides of that comparison. dump_sample names them f<frame>_c<checksum>.bin, which is
// deterministic per pack, so a re-save of the same pack just rewrites the same file. The name does
// not say which side is the re-run and which the saved state (CoopOnline's DESYNC line prints the
// local/remote order for that), so pairs print in checksum order and cmd_diff labels them A / B.
static int cmd_scan(const char* dir) {
    struct Entry { int frame; uint32_t checksum; std::string path; };
    std::vector<Entry> entries;
    try {
        for (const auto& item : std::filesystem::directory_iterator(dir)) {
            const std::string full = item.path().string();
            const size_t slash = full.find_last_of("\\/");
            const std::string name = slash == std::string::npos ? full : full.substr(slash + 1);
            if (name.size() < 5 || name.compare(name.size() - 4, 4, ".bin") != 0) {
                continue;
            }
            int frame = -1;
            uint32_t checksum = 0;
            if (std::sscanf(name.c_str(), "f%d_c%08X", &frame, &checksum) != 2 || frame < 0) {
                continue;
            }
            entries.push_back(Entry{ frame, checksum, item.path().string() });
        }
    } catch (...) {
        printf("cannot scan %s: no such folder, or nothing readable in it\n", dir);
        return 1;
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        return a.frame != b.frame ? a.frame < b.frame : a.checksum < b.checksum;
    });
    printf("%zu dump files under %s\n", entries.size(), dir);
    int pairs = 0, frames_with_pairs = 0;
    for (size_t i = 0; i < entries.size();) {
        size_t last = i;
        while (last < entries.size() && entries[last].frame == entries[i].frame) {
            ++last;
        }
        // one file per distinct checksum: the same pack re-saved rewrites the same name
        std::vector<const Entry*> sides;
        for (size_t k = i; k < last; ++k) {
            if (sides.empty() || sides.back()->checksum != entries[k].checksum) {
                sides.push_back(&entries[k]);
            }
        }
        printf("f%d: %zu files, %zu distinct checksums\n", entries[i].frame, last - i, sides.size());
        // adjacent pairs only: with N peers dumping one frame the pairwise blowup is noise, and
        // the pair the desync event named is always among these
        for (size_t a = 0; a + 1 < sides.size(); ++a) {
            ++pairs;
            printf("== f%d pair %zu: A %s vs B %s\n", entries[i].frame, a + 1,
                sides[a]->path.c_str(), sides[a + 1]->path.c_str());
            cmd_diff(sides[a]->path.c_str(), sides[a + 1]->path.c_str());
        }
        if (sides.size() > 1) {
            ++frames_with_pairs;
        }
        i = last;
    }
    printf("%d frames with a desync pair, %d pairs diffed\n", frames_with_pairs, pairs);
    return pairs > 0 ? 2 : 0;
}

int main(int argc, char** argv) {
    if (argc >= 3 && strcmp(argv[1], "map") == 0) {
        return cmd_map(argv[2]);
    }
    if (argc >= 4 && strcmp(argv[1], "diff") == 0) {
        return cmd_diff(argv[2], argv[3]);
    }
    if (argc >= 3 && strcmp(argv[1], "summary") == 0) {
        return cmd_summary(argv[2]);
    }
    if (argc >= 2 && strcmp(argv[1], "scan") == 0) {
        return cmd_scan(argc >= 3 ? argv[2] : "netdump");
    }
    fprintf(stderr, "usage: netdump_tool map <f> | diff <A> <B> | summary <f> | scan [dir]\n");
    return 1;
}
