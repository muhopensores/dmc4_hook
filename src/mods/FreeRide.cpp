#include "FreeRide.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/Collision.hpp"
#include "../sdk/InputCommand.hpp"
#include "../sdk/MoveRegistry.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

using namespace wallrun;

namespace free_ride {

bool  mod_enabled   = false;
bool  need_forward  = true;
float mount_range   = 350.0f;
float mount_height  = 120.0f;
bool  allow_bosses  = false;
int   lmt_bank[2]   = { 12, 13 };
char  lmt_path[2][128] = { "motion\\pl000\\freeride", "motion\\pl006\\freeride" };
int   slot_mount    = 0;
int   slot_ride     = 1;
int   ctn_bank      = 0;
int   ctn_slot      = 5;             // the fall motion's script, as Gerb borrows: no windows of its own
int   vanilla_mount_motion = 0x11;
int   vanilla_ride_motion  = 5;
float vanilla_ride_frame   = 12.0f;
float mount_frames  = 14.0f;
float hop_height    = 70.0f;
float ride_height   = 45.0f;
float ride_speed    = 16.0f;
float turn_rate     = 3.0f;
float ride_frames   = 240.0f;
float step_down     = 120.0f;
float hold_frames   = 4.0f;
bool  align_enemy   = true;
float enemy_yaw_offset = 0.0f;
int   nero_finisher = 0x1F;          // uPlayerNero::moveAtckRedQueenHiTime (High Roller; the move picks its jump follow-up itself)
int   dante_finisher[3] = { 0x0B, -1, -1 };   // uPlayerDante::moveAtckRevellionHiTime; Gilgamesh / Lucifer: none chosen yet
float finish_back   = 90.0f;

// ---------------------------------------------------------------------------------------------------------------
// Game functions, offsets and helpers.
// ---------------------------------------------------------------------------------------------------------------
namespace {

// the enemy (uActor / uEnemy, DX9)
namespace em {
    constexpr uintptr_t mUnitState      = 0x0004;   // cUnit: & 7 = 1 / 2 while the unit lives (uActor::move's test on mpAttackSender)
    constexpr uintptr_t mPos            = 0x0030;
    constexpr uintptr_t mQuat           = 0x0040;   // MT quaternion x, y, z, w
    constexpr uintptr_t mActorStatus    = 0x0EA8;   // AS_DOWN 0x4, AS_DEAD 0x80
    constexpr uintptr_t mVel            = 0x0EC0;
    constexpr uintptr_t mEmHitStopTimer = 0x1434;   // uEnemy: frames; uEnemy::setHitStop (vtbl 52) writes it
    constexpr uintptr_t mEmHitStop      = 0x1438;   // uEnemy: set by uEnemy::move while the timer runs; every main skips its routine then
}
constexpr uint32_t AS_DOWN = 0x4, AS_DEAD = 0x80;

// cLockonTarget (sdk/uActor.hpp): +8 mpModel, +0x10 mPos, +0x1C mType (LOCKON_MODEL_TYPE: 0 enemy, 1 boss)
constexpr uintptr_t kTargetModel = 0x08, kTargetType = 0x1C;
constexpr uintptr_t kCurrentSword = 0x1DC0;      // uPlayer: Dante's held sword, 4 Rebellion 5 Gilgamesh 6 Lucifer (the rows' atckConditionWp)
constexpr uintptr_t kCmdFlagWords = 0x2184;      // mCmdFlag[3]

// uPlayerNero::moveActAtckL 0x7F9CD0 / uPlayerDante's attack dispatch 0x7CD190 (thiscall, no stack arguments): the
// state function of the current atckId, run the same tick as the hand-off as the vanilla chains do.
constexpr uintptr_t fn_dispatch[2] = { 0x007F9CD0, 0x007CD190 };
constexpr uintptr_t fn_setActionAtck = 0x007AAC80;   // uPlayer::setActionAtck (this@eax, atckId@ecx, body on the stack, callee pops)
constexpr uintptr_t fn_setAction = 0x007AA030;          // uPlayer::setAction (this@ecx, int act; vtbl 101): 25 = the enemy step, uPlayer::moveActEmJump 0x827890
constexpr uintptr_t kEmJumpTimer = 0x1FF4, kWJumpTimer = 0x1FF8;   // uPlayer: checkEmJumpBegin 0x804970 sets them from params 79 / 80 with action 25
// uCollisionMgr (pl::mCollMgr): sCollisionGame::checkPush 0x47BDA0 (and checkAttackObj / checkGrabObj) skips a pair of
// groups when either manager's mppIgnoreModel[16] holds the other's mpDstModel, which is the actor itself. uEm013 puts the
// player there while it carries him (preMain 0x5DE230 -> setIgnoreModel 0x50C1A0; clearIgnoreModel 0x50C1F0 otherwise).
constexpr uintptr_t kIgnoreModels = 0xE8, kIgnoreModelNum = 0x128;
constexpr int kIgnoreModelMax = 16;
constexpr uintptr_t fn_setActionFootwork = 0x007AA2C0;  // uPlayer::setActionFootwork (this@esi, float hokan, char isMotSet; callee pops)

// how a ground move ends (Stinger, Million Stab: setActionFootwork(10.0, 1))
void set_action_footwork(uintptr_t pl, float hokan, int is_mot_set) {
    uintptr_t fn = fn_setActionFootwork;
    __asm {
        pushad
        mov esi, pl
        push is_mot_set
        push hokan
        call fn
        popad
    }
}

void set_action_atck(uintptr_t pl, uint32_t id, int body) {
    uintptr_t fn = fn_setActionAtck;
    __asm {
        pushad
        mov eax, pl
        mov ecx, id
        push body
        call fn
        popad
    }
}

void set_action(uintptr_t pl, int act) { ((void(__fastcall*)(uintptr_t, int, int))fn_setAction)(pl, 0, act); }

void run_dispatch(uintptr_t pl, int character) {
    ((void(__fastcall*)(uintptr_t, int))fn_dispatch[character])(pl, 0);
}

// getLockOnTarget (vtbl 170, thiscall (this, a2, a3, exclude), retn 0Ch) as the game's lever check asks it: (0, 0, 0)
uintptr_t lock_target(uintptr_t pl) {
    const uintptr_t fn = (*(uintptr_t**)pl)[170];
    return ((uintptr_t(__fastcall*)(uintptr_t, int, int, int, uintptr_t))fn)(pl, 0, 0, 0, 0);
}

int character_of(uintptr_t pl) { return at<int>(pl, pl::mPlayerID) == 1 ? NERO : DANTE; }

void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

void face(uintptr_t pl, float yaw) {
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

inline glm::vec3 dir_of(float yaw) { return glm::vec3(std::sin(yaw), 0.0f, std::cos(yaw)); }
inline glm::vec3 vec(const MtVector3& v) { return glm::vec3(v.x, v.y, v.z); }

// Turns `from` toward `to` by at most `step` radians.
float turn_toward(float from, float to, float step) {
    float d = std::remainder(to - from, 6.2831853f);
    if (d > step) d = step; else if (d < -step) d = -step;
    return from + d;
}

// The stage floor under (x, z): a line from `from_y` down to `to_y` in the ground channel.
bool floor_at(float x, float z, float from_y, float to_y, float* out_y) {
    collision::CollParam param = collision::line_param(collision::COLL_GROUND);
    MtLineSegment seg{ { x, from_y, z }, { x, to_y, z } };
    collision::LineResult hit{};
    if (!collision::find_intersection(&param, &seg, false, &hit)) return false;
    *out_y = hit.hitPos.y;
    return true;
}

bool enemy_alive(uintptr_t e) {
    if (!e) return false;
    const uint32_t state = at<uint32_t>(e, em::mUnitState) & 7;
    return (state == 1 || state == 2) && !(at<uint32_t>(e, em::mActorStatus) & AS_DEAD);
}

// The enemy stands still in its own pose while its hit-stop timer runs (uEnemy::move -> mEmHitStop -> every main skips
// its routine); topping the timer up each tick is the whole hold. Stopping is the release.
void hold(uintptr_t e) {
    float& t = at<float>(e, em::mEmHitStopTimer);
    if (t < hold_frames) t = hold_frames;
    MtVector3& v = at<MtVector3>(e, em::mVel);
    v.x = v.y = v.z = 0.0f;
}
void release(uintptr_t e) {
    if (enemy_alive(e)) at<float>(e, em::mEmHitStopTimer) = 0.0f;
}

// The push capsules: the player's against the enemy's shoved the rider off and the two chased each other, so for the
// ride the enemy sits on the player's collision manager's ignore list (push, attack and grab checks all skip the pair;
// the finisher runs once the entry is gone). The game never clears the list by itself: every way out takes it out.
void ignore_enemy(uintptr_t pl, uintptr_t e, bool on) {
    const uintptr_t mgr = at<uintptr_t>(pl, pl::mCollMgr);
    if (!mgr || !e) return;
    int& num = at<int>(mgr, kIgnoreModelNum);
    if (num < 0 || num > kIgnoreModelMax) return;
    uintptr_t* list = &at<uintptr_t>(mgr, kIgnoreModels);
    int i = 0;
    while (i < num && list[i] != e) ++i;
    if (on) {
        if (i < num) return;
        if (num >= kIgnoreModelMax) { spdlog::warn("FreeRide: the player's ignore list is full; the enemy's push stays on"); return; }
        list[num++] = e;
    } else if (i < num) {
        list[i] = list[num - 1];
        list[--num] = 0;
    }
}

void place_enemy(uintptr_t e, const glm::vec3& p, float yaw) {
    MtVector3& pos = at<MtVector3>(e, em::mPos);
    pos.x = p.x; pos.y = p.y; pos.z = p.z;
    if (align_enemy) {
        const float y = yaw + glm::radians(enemy_yaw_offset);
        float* q = &at<float>(e, em::mQuat);
        q[0] = 0.0f; q[1] = std::sin(y * 0.5f); q[2] = 0.0f; q[3] = std::cos(y * 0.5f);
    }
}

// Velocity in both of the game's models: mVel, and the XZ speed along mJump.moveAng that calcSpeedXZ rebuilds it from in
// the air (Gerb's note).
void set_velocity(uintptr_t pl, const glm::vec3& v) {
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    vel.x = v.x; vel.y = v.y; vel.z = v.z;
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);
    acc.x = acc.y = acc.z = 0.0f;
    const glm::vec3 xz(v.x, 0.0f, v.z);
    at<float>(pl, pl::mMoveVel) = glm::length(xz);
    at<float>(pl, pl::mMoveAcc) = 0.0f;
    if (glm::length(xz) > 1e-4f) at<float>(pl, pl::mJumpMoveAng) = std::atan2(xz.x, xz.z);
    MtVector3& add = at<MtVector3>(pl, pl::mAddSpd);
    add.x = add.y = add.z = 0.0f;
    at<bool>(pl, pl::mIsAddSpeed) = false;
}

void set_airborne(uintptr_t pl, bool air) {
    uint32_t& st = at<uint32_t>(pl, pl::mActStat);
    st = air ? ((st & ~0x1u) | 0x2u) : ((st & ~0x2u) | 0x1u);
}

// ---- the LMT, per character: ours only when the bank holds the resource this mod loaded (another mod's file in the
// same bank would play its blocks under our slot numbers) ----
struct Bank { uintptr_t player = 0, rsrc = 0; bool failed = false, warned = false; };
Bank g_bank[2];

bool lmt_file_exists(const char* game_path) {
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

bool lmt_ready(uintptr_t pl, int c) {
    const Bank& b = g_bank[c];
    return b.player == pl && b.rsrc && devil4_sdk::get_motion_bank((void*)pl, lmt_bank[c]) == b.rsrc;
}

void ensure_bank(uintptr_t pl) {
    const int c = character_of(pl);
    Bank& b = g_bank[c];
    if (b.player != pl) b = Bank{ pl };
    if (b.failed || lmt_ready(pl, c) || lmt_bank[c] < 10 || lmt_bank[c] > 14) return;   // 0-9 setup banks, 15 Nero's Buster bank
    const uintptr_t existing = devil4_sdk::get_motion_bank((void*)pl, lmt_bank[c]);
    if (existing) {
        if (!b.warned) spdlog::warn("FreeRide: bank {} already holds another mod's LMT; the game's motions stand in", lmt_bank[c]);
        b.warned = true;
        return;
    }
    if (!lmt_file_exists(lmt_path[c])) { b.failed = true; return; }   // optional: the stand-ins play
    if (!devil4_sdk::load_motion_bank((void*)pl, lmt_path[c], lmt_bank[c])) { b.failed = true; return; }
    b.rsrc = devil4_sdk::get_motion_bank((void*)pl, lmt_bank[c]);
    spdlog::info("FreeRide: '{}' -> bank {}", lmt_path[c], lmt_bank[c]);
}

// ---- per player ----
constexpr int PHASE_ENTRY = 0, PHASE_MOUNT = 1, PHASE_RIDE = 2;
constexpr int kPlayerSlots = 4;

struct RideState {
    uintptr_t player = 0, enemy = 0;
    uint32_t  last_seen = 0;
    glm::vec3 start{0.0f};           // the player's position at the mount
    float     floor_y = 0.0f;        // the floor under the board, last found
    float     yaw = 0.0f;            // the board's direction
    float     t = 0.0f;              // frames into the mount / the ride
    bool      jump_held = false;     // the press that started the ride hasn't been let go yet (eaten until it is)
    bool      melee_held = false;
};
RideState g_rides[kPlayerSlots];

// One slot per uPlayer seen, the least recently seen evicted (Gerb's table).
RideState& state_for(uintptr_t player) {
    static uint32_t tick = 0;
    ++tick;
    for (RideState& s : g_rides) if (s.player == player) { s.last_seen = tick; return s; }
    RideState* victim = &g_rides[0];
    for (RideState& s : g_rides) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = RideState{};
    victim->player = player;
    victim->last_seen = tick;
    return *victim;
}

// The readout's look-up from the GUI thread: no eviction, no tick.
RideState* find_state(uintptr_t player) {
    for (RideState& s : g_rides) if (s.player == player) return &s;
    return nullptr;
}

bool riding(uintptr_t pl) {
    const uint32_t id = at<uint32_t>(pl, pl::mAtckId);
    return at<uint32_t>(pl, pl::mBdyActionLowerNo) == 12 && (id == nero_move_id || id == dante_move_id);
}

// The target the ride would take: the lock-on target's model when it is an enemy lying down, alive, within range.
uintptr_t ride_target(uintptr_t pl) {
    const uintptr_t t = lock_target(pl);
    if (!t) return 0;
    const uint32_t type = at<uint32_t>(t, kTargetType);
    if (type != 0 && !(allow_bosses && type == 1)) return 0;
    const uintptr_t e = at<uintptr_t>(t, kTargetModel);
    if (!enemy_alive(e) || !(at<uint32_t>(e, em::mActorStatus) & AS_DOWN)) return 0;
    const glm::vec3 d = vec(at<MtVector3>(e, em::mPos)) - vec(at<MtVector3>(pl, pl::mPos));
    if (std::fabs(d.y) > mount_height || d.x * d.x + d.z * d.z > mount_range * mount_range) return 0;
    return e;
}

static input_commands::Command g_cmd;
static int g_cmd_bit = -1;
static uint32_t g_fired = 0, g_entries = 0, g_finishes = 0, g_hops = 0;   // readout
static bool g_in_reach = false;   // readout: the local player's lock-on target could be ridden (set on the game thread)

// lock-on held, (the stick toward the target within 10 frames of) jump pressed. The game thread reads g_cmd's head and
// steps in place, so a rebuild (the GUI, a config load) writes the steps first and the count last, never a blank head.
void build_command() {
    using namespace input_commands;
    Command c;
    c.condition(CND_POSE);
    if (need_forward) c.lever(LEVER_FORWARD, LEVER_TARGET_45, 10);
    c.button(BTN_JUMP);
    for (int i = 0; i < Command::kMaxSteps; ++i) g_cmd.steps[i] = c.steps[i];
    g_cmd.head_.condition = c.head_.condition;
    g_cmd.num = c.num;
    g_cmd.head();
}

// The command fired for this player this tick: eat the jump press before checkJumpBegin (checkBeforeAction runs it
// ahead of checkAtck) takes it, so the row gets it. Riders: the press that started the ride is eaten until let go.
void pre_tick(uintptr_t pl) {
    if (!mod_enabled || !pl || g_cmd_bit < 0) return;
    if (pl == (uintptr_t)devil4_sdk::get_local_player()) g_in_reach = ride_target(pl) != 0;
    if ((at<uint32_t>(pl, kCmdFlagWords + 4 * (g_cmd_bit >> 5)) >> (g_cmd_bit & 31)) & 1) {
        at<uint32_t>(pl, pl::mBtnTrg) &= ~0x4u;
        at<bool>(pl, pl::mMotCnclJumpReserve) = false;
        ++g_fired;
        return;
    }
    if (!riding(pl)) {                                   // a ride cut short (hit out of it): the enemy goes free, the push comes back
        if (RideState* s = find_state(pl); s && s->enemy) { release(s->enemy); ignore_enemy(pl, s->enemy, false); s->enemy = 0; }
        return;
    }
    RideState& s = state_for(pl);
    const uint32_t on = at<uint32_t>(pl, pl::mBtnOn);
    if (s.jump_held) {
        if (!(on & 0x4)) s.jump_held = false;
        else { at<uint32_t>(pl, pl::mBtnTrg) &= ~0x4u; at<bool>(pl, pl::mMotCnclJumpReserve) = false; }
    }
    if (s.melee_held) {
        if (!(on & 0x1)) s.melee_held = false;
        else at<uint32_t>(pl, pl::mBtnTrg) &= ~0x1u;
    }
}

// ---- the exits ----
// Off the board with the game's own enemy step: action 25, what checkEmJumpBegin 0x804970 starts (the 0x11 hop toward the
// stick, or straight up, then the fall), with the timers it sets; the air rules take it from there.
void hop_off(uintptr_t pl, RideState& s) {
    release(s.enemy);
    ignore_enemy(pl, s.enemy, false);
    s.enemy = 0;
    ++g_hops;
    set_velocity(pl, glm::vec3(0.0f));
    set_airborne(pl, true);
    const float* prm = at<float*>(pl, pl::mParamTbl0Data);
    at<float>(pl, kEmJumpTimer) = prm[79];
    at<float>(pl, kWJumpTimer)  = prm[80];
    set_action(pl, 25);
}

// The launcher: the player back on the floor behind the enemy, then the Stinger -> Million Stab hand-off (setActionAtck
// with both halves, the new move's state function this same tick).
bool finish(uintptr_t pl, RideState& s, int c) {
    int id = c == NERO ? nero_finisher : -1;
    if (c == DANTE) {
        const uint32_t sword = at<uint32_t>(pl, kCurrentSword);
        if (sword >= 4 && sword <= 6) id = dante_finisher[sword - 4];
    }
    if (id <= 0) return false;
    release(s.enemy);
    ignore_enemy(pl, s.enemy, false);
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    const glm::vec3 p = vec(at<MtVector3>(s.enemy, em::mPos)) - dir_of(s.yaw) * finish_back;
    pos.x = p.x; pos.y = s.floor_y; pos.z = p.z;
    set_velocity(pl, glm::vec3(0.0f));
    set_airborne(pl, false);
    face(pl, s.yaw);
    s.enemy = 0;
    ++g_finishes;
    set_action_atck(pl, (uint32_t)id, -1);
    run_dispatch(pl, c);
    return true;
}

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// The predicate and the move.
// ---------------------------------------------------------------------------------------------------------------
bool can_mount(uintptr_t pl) {
    if (!pl || (at<uint32_t>(pl, pl::mActStat) & 0x2)) return false;    // the rows are ground only
    return ride_target(pl) != 0;
}

void __stdcall move(uintptr_t pl, uint32_t) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    RideState& s = state_for(pl);
    const int c = character_of(pl);
    const float dt = std::max(at<float>(pl, pl::m_delta_time), 0.01f);
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);

    switch (phase) {
    case PHASE_ENTRY: {
        ++g_entries;
        const uintptr_t e = ride_target(pl);
        if (!e) { set_action_footwork(pl, 10.0f, 1); return; }    // the target got up or went away since the press
        s.enemy = e;
        s.start = vec(pos);
        s.t = 0.0f;
        s.jump_held = (at<uint32_t>(pl, pl::mBtnOn) & 0x4) != 0;
        s.melee_held = (at<uint32_t>(pl, pl::mBtnOn) & 0x1) != 0;
        const glm::vec3 ep = vec(at<MtVector3>(e, em::mPos));
        const glm::vec3 to = ep - s.start;
        s.yaw = (to.x * to.x + to.z * to.z > 1.0f) ? std::atan2(to.x, to.z) : at<float>(pl, pl::mRotateNow);
        if (!floor_at(ep.x, ep.z, ep.y + 100.0f, ep.y - 300.0f, &s.floor_y)) s.floor_y = ep.y;
        face(pl, s.yaw);
        hold(e);
        ignore_enemy(pl, e, true);
        ensure_bank(pl);
        if (lmt_ready(pl, c)) {
            devil4_sdk::borrow_ctn((void*)pl, (uint32_t)ctn_bank, (uint32_t)ctn_slot);
            request_motion(pl, (lmt_bank[c] << 8) | (slot_mount & 0xFF), (uint32_t)A_NULL_TRANS_OFF, 3.0f, 0.0f, 1.0f);
        } else {
            request_motion(pl, vanilla_mount_motion, (uint32_t)A_NULL_TRANS_OFF, 3.0f, 0.0f, 1.0f);
        }
        set_airborne(pl, true);
        phase = PHASE_MOUNT;
        return;
    }
    case PHASE_MOUNT: {
        if (!enemy_alive(s.enemy)) { hop_off(pl, s); return; }
        hold(s.enemy);
        // a straight line to the board with a parabola on top, sampled where the player should be after this tick
        const glm::vec3 ep = vec(at<MtVector3>(s.enemy, em::mPos));
        const glm::vec3 end(ep.x, s.floor_y + ride_height, ep.z);
        const float n = std::max(mount_frames, 1.0f);
        s.t += dt;
        const float k = std::min(s.t / n, 1.0f);
        const glm::vec3 want = s.start + (end - s.start) * k + glm::vec3(0.0f, hop_height * 4.0f * k * (1.0f - k), 0.0f);
        set_velocity(pl, (want - vec(pos)) / dt);
        if (k < 1.0f) return;
        s.t = 0.0f;
        if (lmt_ready(pl, c))
            request_motion(pl, (lmt_bank[c] << 8) | (slot_ride & 0xFF), (uint32_t)A_NULL_TRANS_OFF, 4.0f, 0.0f, 1.0f);
        else
            request_motion(pl, vanilla_ride_motion, (uint32_t)A_NULL_TRANS_OFF, 4.0f, vanilla_ride_frame, 0.0f);
        phase = PHASE_RIDE;
        return;
    }
    case PHASE_RIDE: {
        if (!enemy_alive(s.enemy)) { hop_off(pl, s); return; }
        s.t += dt;
        const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
        if (!s.jump_held && (trg & 0x4)) { hop_off(pl, s); return; }   // jump: the enemy step off the board
        if (!s.melee_held && (trg & 0x1)) {                    // melee: the launcher, else a hop
            if (finish(pl, s, c)) return;
            hop_off(pl, s);
            return;
        }
        if (ride_frames > 0.0f && s.t >= ride_frames) { hop_off(pl, s); return; }

        if (at<int>(pl, pl::mAnlgPow) != 0)                     // steer: the board turns toward the stick
            s.yaw = turn_toward(s.yaw, at<float>(pl, pl::mAnlgRadian), glm::radians(turn_rate) * dt);
        face(pl, s.yaw);

        const glm::vec3 next = vec(pos) + dir_of(s.yaw) * ride_speed * dt;
        float fy;
        if (!floor_at(next.x, next.z, next.y + 60.0f, next.y - ride_height - step_down, &fy)) { hop_off(pl, s); return; }   // a ledge
        s.floor_y = fy;
        const glm::vec3 v(dir_of(s.yaw).x * ride_speed, (fy + ride_height - pos.y) / dt, dir_of(s.yaw).z * ride_speed);
        set_velocity(pl, v);
        hold(s.enemy);
        place_enemy(s.enemy, glm::vec3(next.x, fy, next.z), s.yaw);
        return;
    }
    default:
        return;
    }
}

// The rows MoveFreeRide added (one per character): byte 0 of the command word = the command bit, byte 1 = 1 makes the
// row unmatchable (the table's convention for a disabled custom move).
static int g_row_marker[2] = { 0, 0 };

void apply_rows() {
    for (int c = 0; c < 2; ++c) {
        kAtckDefTbl* row = MoveTable::row(c == NERO ? 0 : 1, g_row_marker[c]);
        if (!row) continue;
        row->command.buffer = (uint32_t)(g_cmd_bit & 0xFF) | ((mod_enabled && g_cmd_bit >= 0) ? 0u : 0x100u);
    }
}

} // namespace free_ride

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only.
// ---------------------------------------------------------------------------------------------------------------
using namespace free_ride;

std::optional<std::string> FreeRide::on_initialize() {
    using namespace input_commands;
    // COMMAND attr, start | cancel | repeat, any weapon / style, ground only (atckAs 1), cancel words as Gerb's rows
    // (C_COMMAND open or buffered, or C_ALL). The command byte is written by apply_rows (0x100 = disabled until then).
    const uint32_t nero_row[14]  = { 2, nero_move_id,  7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    const uint32_t dante_row[14] = { 2, dante_move_id, 7, 1, 0x100, 0xFFFFFFFF, 0xFFFFFFFF, 0, 1, 0x03000001, 0, 0, 0, 0x07000005 };
    g_row_marker[NERO]  = MoveTable::add_row(0, nero_row);
    g_row_marker[DANTE] = MoveTable::add_row(1, dante_row);
    move_registry::register_move(move_registry::NERO, nero_move_id, &mod_enabled, &free_ride::move);     // the ID gate + dispatch
    move_registry::register_move(move_registry::DANTE, dante_move_id, &mod_enabled, &free_ride::move);

    build_command();                           // + can_mount: a downed enemy in range
    g_cmd_bit = next_free_bit();
    if (g_cmd_bit < 0 || !add(g_cmd.head(), g_cmd_bit, &mod_enabled, &can_mount)) {
        spdlog::error("FreeRide: no free command bit (85-95 are all taken)");
        g_cmd_bit = -1;
    }
    add_tick(&pre_tick);
    if (!input_commands::hook_installed) {     // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("FreeRide: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init FreeRide (command hook)";
        }
        input_commands::hook_installed = true;
    }
    apply_rows();
    return Mod::on_initialize();
}

void FreeRide::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) return;
    if (uPlayer* pl = devil4_sdk::get_local_player()) ensure_bank((uintptr_t)pl);
}

void FreeRide::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;        // debug page, like Gerb and the dodge
    if (ImGui::Checkbox(_("Free Ride"), &mod_enabled)) apply_rows();
    ImGui::SameLine();
    help_marker(_("DMC3's Free Ride for Nero and Dante: lock-on + forward + jump next to a knocked-down enemy hops onto it and surfs "
                  "it along the ground. The stick steers; jump steps off (the enemy step), melee launches it (Nero's High Roller, "
                  "Dante's High Time with Rebellion). The enemy is held with the game's own hit stop, so it lies still under the "
                  "board and carries on when the ride ends. Optional motions: motion\\pl000\\freeride.lmt (Nero) / "
                  "motion\\pl006\\freeride.lmt (Dante), slot 0 mount, 1 ride."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("FreeRide");

    ImGui::TextUnformatted(_("Input"));
    if (ImGui::Checkbox(_("Stick toward the target"), &need_forward)) build_command();
    ImGui::SameLine();
    help_marker(_("On: lock-on + forward (toward the target, within 45 degrees) + jump. Off: lock-on + jump alone."));
    ImGui::SliderFloat(_("Mount range"), &mount_range, 50.0f, 1000.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("How far (level distance) the knocked-down lock-on target may be for the input to take it."));
    ImGui::SliderFloat(_("Mount height"), &mount_height, 0.0f, 400.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("How far above or below the player the target may lie."));
    ImGui::Checkbox(_("Bosses too"), &allow_bosses);
    ImGui::SameLine();
    help_marker(_("Also ride lock-on targets of the boss type. Off by default: bosses are large and some don't lie down the way normal enemies do."));

    ImGui::TextUnformatted(_("Ride"));
    ImGui::SliderFloat(_("Hop frames"), &mount_frames, 1.0f, 40.0f);
    ImGui::SameLine();
    help_marker(_("Frames the hop onto the enemy takes."));
    ImGui::SliderFloat(_("Hop height"), &hop_height, 0.0f, 200.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("How high the hop arcs above the straight line to the enemy."));
    ImGui::SliderFloat(_("Ride height"), &ride_height, 0.0f, 150.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("The player's feet above the floor under the enemy while riding. Raise it if the player sinks into the enemy, lower it if they float."));
    ImGui::SliderFloat(_("Speed"), &ride_speed, 0.0f, 50.0f, "%.1f cm/frame");
    ImGui::SameLine();
    help_marker(_("How fast the board travels."));
    ImGui::SliderFloat(_("Turn rate"), &turn_rate, 0.0f, 15.0f, "%.1f deg/frame");
    ImGui::SameLine();
    help_marker(_("How fast the board turns toward where the stick points. 0 = straight ahead only."));
    ImGui::SliderFloat(_("Ride length"), &ride_frames, 0.0f, 900.0f, "%.0f frames");
    ImGui::SameLine();
    help_marker(_("Frames before the ride ends by itself with a hop off. 0 = only an input, a ledge or the enemy dying ends it."));
    ImGui::SliderFloat(_("Step down"), &step_down, 10.0f, 400.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("How far the floor may drop under the board (stairs, slopes) before the ride counts it as a ledge and hops off."));
    ImGui::SliderFloat(_("Hold frames"), &hold_frames, 1.0f, 20.0f);
    ImGui::SameLine();
    help_marker(_("The enemy's hit-stop timer is kept at least this high while ridden. It is also how long the enemy stays frozen after the "
                  "ride ends, if the ride is cut short by something other than an exit."));
    ImGui::Checkbox(_("Turn the enemy with the board"), &align_enemy);
    ImGui::SameLine();
    help_marker(_("Sets the enemy's yaw to the board's direction plus the offset below every tick."));
    if (align_enemy) ImGui::SliderFloat(_("Enemy yaw offset"), &enemy_yaw_offset, -180.0f, 180.0f, "%.0f deg");

    ImGui::TextUnformatted(_("Exits"));
    ImGui::InputInt(_("Nero finisher atckId"), &nero_finisher, 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("The attack melee hands off to (the Stinger -> Million Stab way). 0x1F = High Roller (it follows the enemy up when melee is "
                  "held, as usual). -1 = melee hops off instead."));
    ImGui::InputInt(_("Dante finisher, Rebellion"), &dante_finisher[0], 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputInt(_("Dante finisher, Gilgamesh"), &dante_finisher[1], 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputInt(_("Dante finisher, Lucifer"), &dante_finisher[2], 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SameLine();
    help_marker(_("Dante's attack per held sword. 0xB = Rebellion's High Time. -1 = melee hops off with that sword."));
    ImGui::SliderFloat(_("Finisher step back"), &finish_back, 0.0f, 250.0f, "%.0f cm");
    ImGui::SameLine();
    help_marker(_("Before the launcher the player is put on the floor this far behind the enemy, so the swing meets it."));
    ImGui::SameLine();
    help_marker(_("The plain dismount (ride over, a ledge, the enemy gone, or an exit with nothing set): backward and up, then the game's fall."));

    ImGui::TextUnformatted(_("Motions"));
    for (int c = 0; c < 2; ++c) {
        ImGui::PushID(c);
        ImGui::SliderInt(c == NERO ? _("Nero bank") : _("Dante bank"), &lmt_bank[c], 10, 14);
        ImGui::InputText(c == NERO ? _("Nero LMT") : _("Dante LMT"), lmt_path[c], sizeof(lmt_path[c]));
        ImGui::PopID();
    }
    ImGui::SameLine();
    help_marker(_("Optional. A free motion bank per character (Nero: 10 / 11 MoveDef, 13 Gerb, 14 the wall run; Dante: 12 the air dodge, "
                  "14 the wall run) and the LMT loaded into it. Without the file the game's own motions stand in."));
    ImGui::InputInt(_("Slot mount"), &slot_mount);
    ImGui::InputInt(_("Slot ride"), &slot_ride);
    ImGui::InputInt(_("Stand-in mount motion"), &vanilla_mount_motion, 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::InputInt(_("Stand-in ride motion"), &vanilla_ride_motion, 1, 1, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::SliderFloat(_("Stand-in ride frame"), &vanilla_ride_frame, 0.0f, 40.0f);
    ImGui::SameLine();
    help_marker(_("Without the LMT: the mount plays motion 0x11 (the enemy step) and the ride holds motion 5 (the jump) still at this frame."));

    if (uPlayer* pl = devil4_sdk::get_local_player()) {      // live readout
        const uintptr_t P = (uintptr_t)pl;
        const int c = character_of(P);
        const RideState* s = find_state(P);
        const uintptr_t ridden = s ? s->enemy : 0;
        ImGui::Text("command bit %d | fired %u | entries %u | finishers %u | hops %u", g_cmd_bit, g_fired, g_entries, g_finishes, g_hops);
        ImGui::Text("riding %d phase %d t %.1f | target in reach %s | ridden %p hitstop %.1f | floor %.1f | LMT %s",
                    (int)riding(P), at<int>(P, pl::mBdyActionLowerPhase), s ? s->t : 0.0f, g_in_reach ? "yes" : "no", (void*)ridden,
                    enemy_alive(ridden) ? at<float>(ridden, em::mEmHitStopTimer) : 0.0f, s ? s->floor_y : 0.0f,
                    lmt_ready(P, c) ? "loaded" : (lmt_file_exists(lmt_path[c]) ? "not loaded" : "none (stand-ins)"));
        if (ImGui::Button(_("Reload LMT"))) { devil4_sdk::unload_motion_bank(pl, lmt_bank[c]); g_bank[c] = Bank{}; }
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void FreeRide::on_config_load(const utility::Config& cfg) {
    mod_enabled   = cfg.get<bool>("free_ride").value_or(false);
    need_forward  = cfg.get<bool>("free_ride_need_forward").value_or(need_forward);
    mount_range   = cfg.get<float>("free_ride_mount_range").value_or(mount_range);
    mount_height  = cfg.get<float>("free_ride_mount_height").value_or(mount_height);
    allow_bosses  = cfg.get<bool>("free_ride_bosses").value_or(allow_bosses);
    for (int c = 0; c < 2; ++c) {
        lmt_bank[c] = cfg.get<int>("free_ride_bank_" + std::to_string(c)).value_or(lmt_bank[c]);
        if (auto p = cfg.get("free_ride_lmt_" + std::to_string(c))) strcpy_s(lmt_path[c], p->c_str());
    }
    slot_mount    = cfg.get<int>("free_ride_slot_mount").value_or(slot_mount);
    slot_ride     = cfg.get<int>("free_ride_slot_ride").value_or(slot_ride);
    vanilla_mount_motion = cfg.get<int>("free_ride_vanilla_mount").value_or(vanilla_mount_motion);
    vanilla_ride_motion  = cfg.get<int>("free_ride_vanilla_ride").value_or(vanilla_ride_motion);
    vanilla_ride_frame   = cfg.get<float>("free_ride_vanilla_ride_frame").value_or(vanilla_ride_frame);
    mount_frames  = cfg.get<float>("free_ride_mount_frames").value_or(mount_frames);
    hop_height    = cfg.get<float>("free_ride_hop_height").value_or(hop_height);
    ride_height   = cfg.get<float>("free_ride_height").value_or(ride_height);
    ride_speed    = cfg.get<float>("free_ride_speed").value_or(ride_speed);
    turn_rate     = cfg.get<float>("free_ride_turn_rate").value_or(turn_rate);
    ride_frames   = cfg.get<float>("free_ride_frames").value_or(ride_frames);
    step_down     = cfg.get<float>("free_ride_step_down").value_or(step_down);
    hold_frames   = cfg.get<float>("free_ride_hold_frames").value_or(hold_frames);
    align_enemy   = cfg.get<bool>("free_ride_align_enemy").value_or(align_enemy);
    enemy_yaw_offset = cfg.get<float>("free_ride_enemy_yaw").value_or(enemy_yaw_offset);
    nero_finisher = cfg.get<int>("free_ride_nero_finisher").value_or(nero_finisher);
    for (int i = 0; i < 3; ++i) dante_finisher[i] = cfg.get<int>("free_ride_dante_finisher_" + std::to_string(i)).value_or(dante_finisher[i]);
    finish_back   = cfg.get<float>("free_ride_finish_back").value_or(finish_back);
    build_command();                           // the steps follow need_forward
    apply_rows();
}

void FreeRide::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("free_ride", mod_enabled);
    cfg.set<bool>("free_ride_need_forward", need_forward);
    cfg.set<float>("free_ride_mount_range", mount_range);
    cfg.set<float>("free_ride_mount_height", mount_height);
    cfg.set<bool>("free_ride_bosses", allow_bosses);
    for (int c = 0; c < 2; ++c) {
        cfg.set<int>("free_ride_bank_" + std::to_string(c), lmt_bank[c]);
        cfg.set("free_ride_lmt_" + std::to_string(c), std::string(lmt_path[c]));
    }
    cfg.set<int>("free_ride_slot_mount", slot_mount);
    cfg.set<int>("free_ride_slot_ride", slot_ride);
    cfg.set<int>("free_ride_vanilla_mount", vanilla_mount_motion);
    cfg.set<int>("free_ride_vanilla_ride", vanilla_ride_motion);
    cfg.set<float>("free_ride_vanilla_ride_frame", vanilla_ride_frame);
    cfg.set<float>("free_ride_mount_frames", mount_frames);
    cfg.set<float>("free_ride_hop_height", hop_height);
    cfg.set<float>("free_ride_height", ride_height);
    cfg.set<float>("free_ride_speed", ride_speed);
    cfg.set<float>("free_ride_turn_rate", turn_rate);
    cfg.set<float>("free_ride_frames", ride_frames);
    cfg.set<float>("free_ride_step_down", step_down);
    cfg.set<float>("free_ride_hold_frames", hold_frames);
    cfg.set<bool>("free_ride_align_enemy", align_enemy);
    cfg.set<float>("free_ride_enemy_yaw", enemy_yaw_offset);
    cfg.set<int>("free_ride_nero_finisher", nero_finisher);
    for (int i = 0; i < 3; ++i) cfg.set<int>("free_ride_dante_finisher_" + std::to_string(i), dante_finisher[i]);
    cfg.set<float>("free_ride_finish_back", finish_back);
}
