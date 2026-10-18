#include "WallRun.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/uActor.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <string>

using namespace wallrun;

bool  WallRun::mod_enabled  = false;
bool  WallRun::replace_wall_jump = true;
uintptr_t WallRun::jmp_ret1 = NULL;
float WallRun::run_speed    = 12.0f;
float WallRun::run_frames   = 0.0f;     // 0 = as long as jump is held
float WallRun::stick_speed  = 2.0f;
float WallRun::wall_gap     = 30.0f;
float WallRun::cast_length  = 120.0f;
float WallRun::cast_height  = 80.0f;
float WallRun::cast_angle   = 45.0f;
float WallRun::corner_dot   = 0.5f;    // normals more than 60 degrees apart = corner
float WallRun::climb_angle  = 35.0f;
float WallRun::climb_speed  = 10.0f;
float WallRun::away_deadzone = 0.3f;
int   WallRun::jumps_after_run = 1;
int   WallRun::motion_left  = 23;      // placeholder: the wall-jump side kick-offs (L 23 / R 22, bank 0)
int   WallRun::motion_right = 22;
int   WallRun::motion_up    = 20;      // placeholder: the front kick-off (F, facing the wall)
bool  WallRun::use_custom_lmt  = false;
bool  WallRun::load_coat_lmt   = false;
int   WallRun::lmt_bank        = 14;
int   WallRun::run_slot_left   = 1;      // the custom LMT: 0 right, 1 left, 2 vertical, all attribute 0 (looping)
int   WallRun::run_slot_right  = 0;
int   WallRun::run_slot_up     = 2;
char  WallRun::lmt_body[128]       = "motion\\common\\wallrun";
char  WallRun::lmt_coat_nero[128]  = "motion\\common\\wallrun_pl000_03";
char  WallRun::lmt_coat_dante[128] = "motion\\common\\wallrun_pl006_03";

// ---------------------------------------------------------------------------------------------------------------
// Game functions. All __usercall; each thunk saves the registers and the stack pointer around the call so the
// __userpurge ones (callee pops) are fine too. Addresses are DX9.
// ---------------------------------------------------------------------------------------------------------------
namespace {

static uintptr_t fn_findIntersection  = 0x0095B320; // sDevil4Collision::findIntersection(query@eax, inst, seg*, bothSides, hitOut*)
static uintptr_t fn_getNormalToTri    = 0x0048F1A0; // sMath::getNormalToTri(result@eax, out@edi) -> out
static uintptr_t fn_setAction         = 0x007AA030; // uPlayer::setAction(this@ecx, int act)          vtbl slot 101
static uintptr_t fn_initJumpBegin     = 0x007AC9E0; // uPlayer::initJumpBegin(this@ecx, int)          vtbl slot 109
static uintptr_t fn_setActionLand     = 0x007AA6A0; // uPlayer::setActionLand(this@ecx)
static uintptr_t fn_setJumpRate       = 0x007ACC70; // uPlayer::setJumpRate(this@eax)
static uintptr_t fn_adjustJumpSpeed   = 0x007ACCE0; // uPlayer::adjustJumpSpeed(this@esi)
static uintptr_t fn_calcSpeedXZ       = 0x007ACAB0; // uPlayer::calcSpeedXZ(this@esi, float yaw)      userpurge
static uintptr_t fn_setPadVibration   = 0x007AC130; // uPlayer::setPadVibration(this@edx, id@eax)
static uintptr_t fn_getEfctMtrlFlg    = 0x0045F780; // cUtil::getEfctMtrlFlg(result@eax)
static uintptr_t fn_setEffectStayDir  = 0x004806A0; // sDevil4Effect::setEffectStayDir(list@eax, idx@ecx, inst, dir*, pos*, group, material, line, owner) userpurge
static uintptr_t fn_setEffect         = 0x0081FD60; // uPlayer::setEffect(effect@eax, this@ecx, a, b, c, d)  userpurge
static uintptr_t fn_rotateDifference  = 0x004581A0; // cRotateCtrl::getDifference(a@xmm0, b@xmm1) -> xmm0
static uintptr_t fn_rotateRequest     = 0x00458210; // cRotateCtrl::request(this@eax, target@xmm3, float max) userpurge
static uintptr_t sDevil4Collision_ptr = 0x00E559D0; // sDevil4Collision::mpInstance
static uintptr_t sDevil4Effect_ptr    = 0x00E559C8; // sDevil4Effect::mpInstance

bool find_intersection(CollParam* param, MtLineSegment* segment, bool both_sides, LineResult* out) {
    bool hit = false;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov eax, param
        push out
        movzx ecx, byte ptr both_sides
        push ecx
        push segment
        mov edi, sDevil4Collision_ptr
        push dword ptr [edi]
        call fn_findIntersection
        mov byte ptr hit, al
        popad
        mov esp, ebx
        pop ebx
    }
    return hit;
}

MtVector3 tri_normal(const LineResult* poly) {
    MtVector3 out{};
    MtVector3* p = &out;
    _asm {
        pushad
        mov eax, poly
        mov edi, p
        call fn_getNormalToTri
        popad
    }
    return out;
}

void set_action(uintptr_t pl, int act)        { ((void(__fastcall*)(uintptr_t, int, int))fn_setAction)(pl, 0, act); }
void init_jump_begin(uintptr_t pl, int arg)   { ((void(__fastcall*)(uintptr_t, int, int))fn_initJumpBegin)(pl, 0, arg); }
void set_action_land(uintptr_t pl)            { ((void(__fastcall*)(uintptr_t, int))fn_setActionLand)(pl, 0); }

void set_jump_rate(uintptr_t pl) {
    _asm {
        pushad
        mov eax, pl
        call fn_setJumpRate
        popad
    }
}

void adjust_jump_speed(uintptr_t pl) {
    _asm {
        pushad
        mov esi, pl
        call fn_adjustJumpSpeed
        popad
    }
}

void calc_speed_xz(uintptr_t pl, float yaw) {
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov esi, pl
        push yaw
        call fn_calcSpeedXZ
        popad
        mov esp, ebx
        pop ebx
    }
}

void set_pad_vibration(uintptr_t pl, int id) {
    _asm {
        pushad
        mov edx, pl
        mov eax, id
        call fn_setPadVibration
        popad
    }
}

int efct_mtrl_flag(const LineResult* poly) {
    int r;
    _asm {
        pushad
        mov eax, poly
        call fn_getEfctMtrlFlg
        mov r, eax
        popad
    }
    return r;
}

// The game's wall-kick dust: effect list 0 index 0x18 at `pos`, oriented along `dir`.
void wall_dust_effect(uintptr_t pl, const MtVector3& dir, const MtVector3& pos, int material) {
    const MtVector3* pdir = &dir;
    const MtVector3* ppos = &pos;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push pl                 // owner
        push 0x14               // unit line
        push material
        push 0xFFFFFFFF         // group mask
        push ppos
        push pdir
        mov edi, sDevil4Effect_ptr
        push dword ptr [edi]
        mov ecx, 0x18
        xor eax, eax
        call fn_setEffectStayDir
        mov ecx, pl             // uPlayer::setEffect(effect, this, 1, -1, 1, 1)
        push 1
        push 1
        push 0xFFFFFFFF
        push 1
        call fn_setEffect
        popad
        mov esp, ebx
        pop ebx
    }
}

float rotate_difference(float a, float b) {
    float r;
    _asm {
        movss xmm0, a
        movss xmm1, b
        call fn_rotateDifference
        movss r, xmm0
    }
    return r;
}

void rotate_request(uintptr_t pl, float target, float max_step) {
    uintptr_t ctrl = pl + 0x1704;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        mov eax, ctrl
        movss xmm3, target
        push max_step
        call fn_rotateRequest
        popad
        mov esp, ebx
        pop ebx
    }
}

// requestSetMotion_2(forcible = 1, this, motion, attr, interp, start, speed), whole-body form.
void request_motion(uintptr_t pl, int motion, uint32_t attr, float interp, float start, float speed) {
    devil4_sdk::indexed_anim_call(motion, (uPlayer*)pl, attr, speed, start, interp);
}

// ---- small helpers over the player ---------------------------------------------------------------------------

glm::vec3 wmat_row(uintptr_t pl, int row) {           // rows: 0 right, 1 up, 2 forward, 3 position
    const float* r = &at<float>(pl, pl::mWmat + 0x10 * row);
    return glm::vec3(r[0], r[1], r[2]);
}

CollParam wall_param() {
    CollParam p{};
    p.type             = 127;
    p.filter           = 0x20000;           // walls
    p.handle_exclude_0 = -1;
    p.handle_exclude_1 = -1;
    p.correctionNum    = 10;
    p.bAdjMvRelSet     = 1;
    p.bAdjAntiStop     = 1;
    p.capsuleFwdLimit  = 3.4028235e38f;
    p.capsuleBwdLimit  = 3.4028235e38f;
    return p;
}

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }   // the game's yaw: dir = (sin, 0, cos)

constexpr uint32_t ATTR_KICK = 5;      // what the wall jump uses for its kick-off motions
constexpr uint32_t ATTR_FALL = 1;
constexpr uint32_t ATTR_LOOP = 0x15;
constexpr int MOT_FALL       = 5;      // the wall jump's fall motion (4 for a vertical jump, 9 back-facing)
constexpr int ACT_WALL_JUMP  = 0x18;   // mBdyAction[1].no of moveActWallJump

} // namespace

// ---------------------------------------------------------------------------------------------------------------
// Wall detection: two casts, +-cast_angle about the up axis, from cast_height above the feet.
// ---------------------------------------------------------------------------------------------------------------
glm::vec3 WallRun::heading(uintptr_t player, const Wall* running) {
    if (at<int>(player, pl::mAnlgPow) != 0) {                       // stick deflected: its world yaw
        const float yaw = at<float>(player, pl::mAnlgRadian);
        const glm::vec3 stick(std::sin(yaw), 0.0f, std::cos(yaw));
        if (!running) return stick;
        const glm::vec3 normal = running->normal;
        if (glm::dot(stick, normal) <= 0.0f) return stick;          // into the wall or along it: as is
        // Leaning away from the wall has no effect by itself: only its part along the wall steers, and when
        // that part is small (mostly straight away) the run just keeps going.
        const glm::vec3 along = stick - normal * glm::dot(stick, normal);
        if (glm::length(along) >= away_deadzone) return glm::normalize(along);
    }
    if (running)                                                    // centred (or pushed away): keep going as we were
        return running->vertical ? -glm::vec3(running->normal) : glm::vec3(running->run_dir);
    return wmat_row(player, 2);                                     // no run yet: the facing, once
}

bool WallRun::detect(uintptr_t player, Wall& out, const glm::vec3& heading) {
    const glm::vec3 up    = wmat_row(player, 1);
    glm::vec3 fwd = heading - up * glm::dot(heading, up);            // the casts fan out around the heading, level
    if (glm::length(fwd) < 1e-4f) fwd = wmat_row(player, 2);
    fwd = glm::normalize(fwd);
    glm::vec3 right = glm::cross(up, fwd);
    if (glm::dot(right, wmat_row(player, 0)) < 0.0f) right = -right; // same handedness as the model's right axis
    const glm::vec3 feet  = at<MtVector3>(player, pl::mPos);
    const glm::vec3 origin = feet + up * cast_height;

    const float a = glm::radians(cast_angle);
    const glm::vec3 dirs[2] = {
        glm::normalize(fwd * std::cos(a) - right * std::sin(a)),   // left cast
        glm::normalize(fwd * std::cos(a) + right * std::sin(a)),   // right cast
    };

    CollParam param = wall_param();
    LineResult hits[2]{};
    glm::vec3 normals[2]{};
    int n = 0, nearest = -1;
    for (int i = 0; i < 2; ++i) {
        MtLineSegment seg;
        seg.p0 = origin;
        seg.p1 = origin + dirs[i] * cast_length;
        if (!find_intersection(&param, &seg, false, &hits[i])) continue;
        if (*(uint32_t*)(hits[i].pTriangle + 0x14) & 1) continue;   // surface_attr bit 0: the wall jump refuses it too
        normals[n] = tri_normal(&hits[i]);
        if (nearest < 0 || hits[i].hitDistance < hits[nearest].hitDistance) nearest = i;
        ++n;
    }
    if (n == 0) return false;
    if (n == 2 && glm::dot(normals[0], normals[1]) < corner_dot) return false;   // inside corner: two walls

    glm::vec3 normal = normals[0];
    if (n == 2) normal += normals[1];
    normal = glm::normalize(normal);

    if (glm::length(glm::cross(normal, up)) < 1e-4f) return false;   // floor or ceiling, not a wall

    out.normal  = normal;
    out.hit_pos = hits[nearest].hitPos;
    out.poly    = hits[nearest];
    return true;
}

void WallRun::steer(uintptr_t player, Wall& wall, const glm::vec3& heading) {
    const glm::vec3 right  = wmat_row(player, 0);
    const glm::vec3 up     = wmat_row(player, 1);
    const glm::vec3 normal = wall.normal;
    const glm::vec3 want   = heading;        // the stick, or the run's own direction while it's centred

    // Into the wall within climb_angle: climb. Otherwise sideways, the way `want` leans along the wall.
    const bool vertical = glm::dot(want, -normal) > std::cos(glm::radians(climb_angle));
    glm::vec3 along = glm::normalize(glm::cross(normal, up));   // parallel to the wall, level with the ground
    if (glm::dot(along, want) < 0.0f) along = -along;
    if (vertical) {
        wall.run_dir = glm::normalize(up - normal * glm::dot(up, normal));   // up, projected onto the wall plane
        wall.yaw     = yaw_of(-normal);                                       // face into the wall
    } else {
        wall.run_dir = along;
        wall.yaw     = yaw_of(along);
    }
    wall.vertical = vertical;
    wall.on_left  = glm::dot(normal, right) > 0.0f;             // normal points at us from the left
}

// ---------------------------------------------------------------------------------------------------------------
// The move. It is the body of lower-body action 0x18 (the hook), so the phase is the game's mBdyAction[1].phase:
// 0 = just entered, PHASE_RUN = running, 1-4 = handed over to wall_jump_port (the game's own wall jump).
// ---------------------------------------------------------------------------------------------------------------
namespace {

constexpr int PHASE_RUN = 100;      // out of the port's 0-4

struct RunState {                   // what the game's phase word can't hold
    uintptr_t player = 0;
    float timer = 0.0f;             // frames left when run_frames caps the run
    int motion = -1;                // the motion playing, to re-request when the direction mode changes
    Wall wall{};
};

RunState g_runs[2];                 // two players (co-op)

RunState& state_for(uintptr_t player) {
    for (RunState& s : g_runs) if (s.player == player) return s;
    for (RunState& s : g_runs) if (s.player == 0) { s.player = player; return s; }
    g_runs[0] = RunState{}; g_runs[0].player = player; return g_runs[0];
}

// Backstop against clipping through the wall (root motion, push collision off for a frame, ...): keep the
// position at least wall_gap from the wall plane, on the side the normal points to. Runs before moveMotion
// integrates the velocity, so it corrects last tick's result.
void keep_off_wall(uintptr_t pl, const Wall& wall) {
    if (WallRun::wall_gap <= 0.0f) return;
    MtVector3& pos = at<MtVector3>(pl, pl::mPos);
    const glm::vec3 n = wall.normal;
    const float d = glm::dot(glm::vec3(pos) - glm::vec3(wall.hit_pos), n);   // signed distance to the plane
    if (d < WallRun::wall_gap) {
        const glm::vec3 fixed = glm::vec3(pos) + n * (WallRun::wall_gap - d);
        pos = fixed;
    }
}

void face(uintptr_t pl, float yaw) {                 // snap, the way the wall jump does
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

// .ctn borrow (D15): make the next motion request read the cancel script of `ctn_bank:ctn_slot` for both body
// halves, as uPlayerNero::setShortGrabMotionSet does for the Buster motions. updateMotion consumes and clears it.
void borrow_ctn(uintptr_t pl, uint32_t ctn_bank, uint32_t ctn_slot) {
    for (int half = 0; half < 2; ++half) {
        uintptr_t p = pl + pl::mMotionSetCharTblParam + 12 * half;
        at<bool>(p, 0)     = true;
        at<uint32_t>(p, 4) = ctn_bank;
        at<uint32_t>(p, 8) = ctn_slot;
    }
}

void play_run_motion(RunState& s, uintptr_t pl) {
    const int motion = WallRun::motion_for(pl, s.wall);
    if (motion == s.motion) return;
    const bool custom = WallRun::custom_ready(pl);
    if (custom)                                  // a runtime bank has no .ctn scripts: borrow the placeholder's
        borrow_ctn(pl, 0, WallRun::placeholder_motion(s.wall));
    // The custom cycles loop, with their root translation ignored (A_NULL_TRANS_OFF: the run's velocity moves the
    // player, the LMT's root track would walk the capsule into the wall); the placeholder kick-offs use the wall
    // jump's 5 (A_NULL_TRANS_OFF | A_LOOP_OFF).
    request_motion(pl, motion, custom ? (uint32_t)A_NULL_TRANS_OFF : ATTR_KICK, 4.0f, 0.0f, 1.0f);
    s.motion = motion;
}

// Entering action 0x18 cost a wall jump and counted an air jump (checkWallJumpBegin -> initJumpBegin(3):
// --mWallJumpCnt, ++mWJumpCnt). Give them back when the run ends so the player can keep jumping.
void give_jumps_back(uintptr_t pl) {
    char& wall_left = at<char>(pl, pl::mWallJumpCnt);
    char& air_used  = at<char>(pl, pl::mWJumpCnt);
    switch (WallRun::jumps_after_run) {
    case 1:                                                 // undo the entry's cost
        ++wall_left;
        if (air_used > 0) --air_used;
        break;
    case 2: {                                               // as if freshly airborne
        const uint8_t* tbl = at<uint8_t*>(pl, pl::mpActDataTbl);
        if (tbl) wall_left = (char)tbl[1];
        air_used = 0;
        break;
    }
    default: break;                                         // 0: the game's own accounting
    }
}

// Leave into the air: the fall motion and the game's neutral air state (ends action 0x18).
void leave_to_air(uintptr_t pl) {
    give_jumps_back(pl);
    request_motion(pl, MOT_FALL, ATTR_FALL, 10.0f, 7.0f, 1.0f);
    devil4_sdk::neutral_air_transition((uPlayer*)pl);
}

// Hand the action over to the game's wall jump: it reads the wall from mGroundPoly and runs its phases 0-4 in
// the same phase word from here on (checkWallJumpBegin already set mJump.mode and the jump counters on entry).
void hand_to_wall_jump(RunState& s, uintptr_t pl, bool from_run) {
    if (from_run) {
        at<LineResult>(pl, pl::mGroundPoly) = s.wall.poly;   // else keep checkWallJumpBegin's wall
        give_jumps_back(pl);                                 // the kick-off itself doesn't charge again
    }
    at<int>(pl, pl::mBdyActionLowerPhase) = 0;
    s.motion = -1;
    WallRun::wall_jump_port(pl);
}

} // namespace

void __stdcall WallRun::move(uintptr_t pl) {
    int& phase = at<int>(pl, pl::mBdyActionLowerPhase);
    if (phase != 0 && phase != PHASE_RUN) { wall_jump_port(pl); return; }   // handed over earlier

    RunState& s = state_for(pl);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);

    if (phase == 0) {                                   // entry: this tick's jump press near a wall
        s.motion = -1;
        s.timer  = run_frames;
        const glm::vec3 h = heading(pl, nullptr);
        if (!detect(pl, s.wall, h)) { hand_to_wall_jump(s, pl, false); return; }   // nothing runnable that way: plain wall jump
        steer(pl, s.wall, h);
        face(pl, s.wall.yaw);
        acc.x = acc.y = acc.z = 0.0f;
        vel.x = vel.y = vel.z = 0.0f;
        play_run_motion(s, pl);
        phase = PHASE_RUN;
        return;                                         // the jump button is down on this tick by definition
    }

    // running
    if (!(at<uint32_t>(pl, pl::mBtnOn) & 0x4)) { hand_to_wall_jump(s, pl, true); return; }   // jump released: kick off
    const glm::vec3 h = heading(pl, &s.wall);
    if (!detect(pl, s.wall, h)) { leave_to_air(pl); return; }                        // wall ended, or a corner
    steer(pl, s.wall, h);
    play_run_motion(s, pl);                             // only when left / right / up changed
    face(pl, s.wall.yaw);

    const float speed = s.wall.vertical ? climb_speed : run_speed;
    const glm::vec3 v = glm::vec3(s.wall.run_dir) * speed - glm::vec3(s.wall.normal) * stick_speed;
    vel.x = v.x; vel.z = v.z;
    vel.y = s.wall.vertical ? v.y : 0.0f;               // climb: up the wall; sideways: level
    acc.x = acc.y = acc.z = 0.0f;
    keep_off_wall(pl, s.wall);

    if (run_frames > 0.0f) {
        s.timer -= at<float>(pl, pl::m_delta_time);
        if (s.timer <= 0.0f) { hand_to_wall_jump(s, pl, true); return; }
    }

    uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
    if (coll && at<bool>(coll, 0x1C)) set_action_land(pl);   // mLand: back on the ground
}

// ---------------------------------------------------------------------------------------------------------------
// Custom LMT: a free motion bank filled at runtime (MoveDef D14), body and coat. Without the file the run keeps
// the placeholder kick-offs, so the mod works for people who don't have the animation.
// ---------------------------------------------------------------------------------------------------------------
static bool lmt_file_exists(const char* game_path) {        // the loose-file root, as Payline checks its LMT
    if (!game_path || !*game_path) return false;
    return std::filesystem::exists(std::string(".\\nativePC\\") + game_path + ".lmt");
}

int WallRun::placeholder_motion(const Wall& wall) {
    if (wall.vertical) return motion_up;
    return wall.on_left ? motion_left : motion_right;
}

bool WallRun::custom_ready(uintptr_t player) {
    return use_custom_lmt && player && devil4_sdk::get_motion_bank((void*)player, lmt_bank) != 0;
}

int WallRun::motion_for(uintptr_t player, const Wall& wall) {
    if (!custom_ready(player)) return placeholder_motion(wall);   // no file for this player: the stand-ins
    const int slot = wall.vertical ? run_slot_up : wall.on_left ? run_slot_left : run_slot_right;
    return (lmt_bank << 8) | (slot & 0xFF);
}

void WallRun::ensure_bank(uintptr_t player) {
    if (!use_custom_lmt || !player || lmt_bank < 10 || lmt_bank > 14) return;   // 0-9 are setup banks, 15 Buster
    static uintptr_t s_failed_for = 0;       // don't retry (and spam the log) every frame on a missing file
    if (s_failed_for == player) return;

    void* body = (void*)player;
    if (!devil4_sdk::get_motion_bank(body, lmt_bank)) {
        if (!lmt_file_exists(lmt_body)) {                    // no file: the placeholders stay in use, say so once
            spdlog::warn("WallRun: nativePC\\{}.lmt not found, using the placeholder motions", lmt_body);
            s_failed_for = player;
            return;
        }
        if (!devil4_sdk::load_motion_bank(body, lmt_body, lmt_bank)) { s_failed_for = player; return; }
        spdlog::info("WallRun: '{}' -> bank {} of the body", lmt_body, lmt_bank);
    }
    if (!load_coat_lmt) return;                              // coat (and face) motions stay off for now
    void* coat = (void*)at<uintptr_t>(player, pl::mpCoatMdl);   // requestSetMotion mirrors motion numbers to it
    if (coat && !devil4_sdk::get_motion_bank(coat, lmt_bank)) {
        const char* path = at<uint32_t>(player, pl::charType) == 1 ? lmt_coat_nero : lmt_coat_dante;
        if (!lmt_file_exists(path)) {                        // body runs the custom cycles, the coat holds its pose
            spdlog::warn("WallRun: nativePC\\{}.lmt not found, the coat won't animate on the run", path);
            s_failed_for = player;
            return;
        }
        if (!devil4_sdk::load_motion_bank(coat, path, lmt_bank)) { s_failed_for = player; return; }
        spdlog::info("WallRun: '{}' -> bank {} of the coat", path, lmt_bank);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Reference: uPlayer::moveActWallJump 0x827200 (SE 0xA2BCD0), readable. Phases in mBdyAction[1].phase.
// ---------------------------------------------------------------------------------------------------------------
void WallRun::wall_jump_port(uintptr_t pl) {
    int&   phase     = at<int>(pl, pl::mBdyActionLowerPhase);
    int&   jumpType  = at<int>(pl, pl::mJumpType);
    bool&  wallFront = at<bool>(pl, pl::isWallFront);
    MtVector3& vel   = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc   = at<MtVector3>(pl, pl::mAcc);
    const LineResult& poly = at<LineResult>(pl, pl::mGroundPoly);

    switch (phase) {
    case 0: {
        at<float>(pl, pl::mDownMaxSpd) = at<float*>(pl, pl::mParamTbl0Data)[103];
        at<float>(pl, pl::mActTimer0)  = 10.0f;

        const glm::vec3 n = tri_normal(&poly);              // wall normal, points at the player
        const float wallYaw = yaw_of(n);

        // Stick direction against the wall normal picks the jump type.
        if (at<int>(pl, pl::mAnlgPow) != 0) {
            const float stick = at<float>(pl, pl::mAnlgRadian);
            const glm::vec3 s(std::sin(stick), 0.0f, std::cos(stick));
            const float dot = glm::dot(s, n);
            if (dot < -0.75f)      jumpType = JUMP_TYPE_F;              // pushing into the wall
            else if (dot > 0.75f)  jumpType = JUMP_TYPE_B;              // pulling away from it
            else                   jumpType = (s.x * n.z - s.z * n.x) <= 0.0f ? JUMP_TYPE_L : JUMP_TYPE_R;
            at<float>(pl, pl::mJumpMoveAng) = at<float>(pl, pl::mMoveAngle);
        } else {
            jumpType = JUMP_TYPE_V;                                     // no stick: straight up, facing-based
            at<float>(pl, pl::mJumpMoveAng) = at<float>(pl, pl::mRotateNow);
        }

        const float facing = at<float>(pl, pl::mRotateNow);
        const float facingDot = std::sin(facing) * n.x + std::cos(facing) * n.z;

        acc.x = acc.y = acc.z = 0.0f;
        vel.x = vel.y = vel.z = 0.0f;
        at<int8_t>(pl, pl::mJumpAtckCnt) = 0;
        if ((at<uint32_t>(pl, pl::mActStat) & 2) && at<uint8_t>(pl, pl::mAtckLongCnt) > 2)
            at<uint8_t>(pl, pl::mAtckLongCnt) = 2;
        phase = 1;

        if (facingDot >= 0.0f) {                 // facing away from the wall
            wallFront = false;
            face(pl, wallYaw);
        } else {                                 // facing the wall
            wallFront = true;
            face(pl, wallYaw + 3.1415927f);
            if (jumpType == JUMP_TYPE_B) {       // back jump while facing it: turn first (motion 21), wait
                request_motion(pl, 21, ATTR_KICK, 0.0f, 0.0f, 1.0f);
                return;
            }
        }
        [[fallthrough]];
    }
    case 1: {
        // Kick-off motion by type and side. Front-facing / back-facing pairs: F 20 / 25, L 23 / 27, R 22 / 26.
        switch (jumpType) {
        case JUMP_TYPE_F: request_motion(pl, wallFront ? 20 : 25, ATTR_KICK, 4.0f, 0.0f, 1.0f); break;
        case JUMP_TYPE_L: request_motion(pl, wallFront ? 23 : 27, ATTR_KICK, 4.0f, 0.0f, 1.0f); break;
        case JUMP_TYPE_R: request_motion(pl, wallFront ? 22 : 26, ATTR_KICK, 4.0f, 0.0f, 1.0f); break;
        case JUMP_TYPE_B:
            if (!wallFront) { request_motion(pl, 25, ATTR_KICK, 4.0f, 0.0f, 1.0f); break; }
            if (!at<bool>(pl, pl::mMotionFrameEnd0)) return;           // still turning
            request_motion(pl, 28, ATTR_KICK, 0.0f, 0.0f, 1.0f);
            face(pl, yaw_of(tri_normal(&poly)));
            break;
        default:          request_motion(pl, wallFront ? 20 : 25, ATTR_KICK, 4.0f, 0.0f, 1.0f); break;   // V
        }
        // Turn toward the move direction, 20% of the remaining difference per request.
        if (jumpType != JUMP_TYPE_F || wallFront) {
            const float moveAng = at<float>(pl, pl::mJumpMoveAng);
            const float diff = rotate_difference(moveAng, at<float>(pl, pl::mRotateNow));
            rotate_request(pl, moveAng, std::fabs(diff * 0.2f));
        }
        ++phase;
        break;
    }
    case 2:
        if (at<bool>(pl, pl::mMotionFrameEnd0)) {
            const int fall = jumpType == JUMP_TYPE_V ? 4 : (jumpType != JUMP_TYPE_F || wallFront) ? 5 : 9;
            request_motion(pl, fall, ATTR_FALL, 10.0f, 7.0f, 1.0f);
            if (at<uint32_t>(pl, pl::mBtnOn) & 0x4)                         // jump still held: high variant
                at<int>(pl, pl::mJumpMode) = JUMP_MODE_NORM_WALL_HI;
            set_jump_rate(pl);
            wall_dust_effect(pl, tri_normal(&poly), poly.hitPos, efct_mtrl_flag(&poly));
            ++phase;
        }
        break;
    case 3:
        if (at<uint8_t>(pl, pl::mMotContFree0) >= 2) {                      // .ctn trigger: loop the fall
            request_motion(pl, at<int>(pl, pl::mMotionCtrl0No), ATTR_LOOP, 0.0f, 51.0f, 1.0f);
            ++phase;
        }
        [[fallthrough]];
    case 4: {
        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (vel.y <= 0.0f && at<bool>(coll, 0x1C)) { set_action_land(pl); return; }   // mLand
        adjust_jump_speed(pl);
        calc_speed_xz(pl, at<float>(pl, pl::mJumpMoveAng));
        break;
    }
    default:
        break;
    }

    // Every phase but a landing: the 10-frame timer from phase 0 ends in a pad vibration.
    float& t = at<float>(pl, pl::mActTimer0);
    if (t > 0.0f) {
        t -= at<float>(pl, pl::m_delta_time);
        if (t <= 0.0f) set_pad_vibration(pl, 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Hook: the entry of uPlayer::moveActWallJump 0x827200 (`this` in eax, no stack arguments, returns nothing). The
// first 6 bytes are `push ebp; mov ebp, esp; and esp, -16`; the detour runs our port and returns straight to the
// caller (moveActionLower), or replays those 6 bytes and continues the original at 0x827206.
// WallRun::move itself needs no hook: MoveTable's dispatch calls it.
// ---------------------------------------------------------------------------------------------------------------
static void __stdcall wall_jump_from_hook(uintptr_t player) { WallRun::move(player); }

naked void detour_wall_jump() {
    _asm {
        cmp byte ptr [WallRun::mod_enabled], 1
        jne original
        cmp byte ptr [WallRun::replace_wall_jump], 1
        jne original
        pushad
        push eax                        // this
        call wall_jump_from_hook        // __stdcall: pops its argument
        popad
        ret                             // back to moveActionLower, as the game's function would
    original:
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF0
        jmp dword ptr [WallRun::jmp_ret1]
    }
}

std::optional<std::string> WallRun::on_initialize() {
    if (!install_hook_absolute(0x827200, hook1, &detour_wall_jump, &jmp_ret1, 6)) {
        spdlog::error("Failed to init WallRun hook (moveActWallJump entry)\n");
        return "Failed to init WallRun hook (moveActWallJump entry)";
    }
    return Mod::on_initialize();
}

void WallRun::on_frame(fmilliseconds& dt) {
    if (!mod_enabled || !use_custom_lmt) return;
    if (uPlayer* pl = devil4_sdk::get_local_player()) ensure_bank((uintptr_t)pl);
}

void WallRun::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;       // debug page: it hooks the wall jump of both characters
    ImGui::Checkbox(_("Wall Run"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Run along a wall found by two 45-degree casts. Jump kicks off into the game's wall jump."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::Checkbox(_("Replace the game's wall jump"), &replace_wall_jump);
    ImGui::SameLine();
    help_marker(_("Runs the C++ port of uPlayer::moveActWallJump instead of the game's own (same behaviour; the port is "
                  "the base the wall run is built on, so this checks it)."));
    ImGui::SliderFloat(_("Run speed"), &run_speed, 0.0f, 40.0f);
    ImGui::SliderFloat(_("Max run frames"), &run_frames, 0.0f, 300.0f);
    ImGui::SameLine();
    help_marker(_("The run lasts while jump is held; this caps it (0 = no cap). Releasing jump kicks off with the game's wall jump."));
    ImGui::SliderFloat(_("Wall stick"), &stick_speed, 0.0f, 10.0f);
    ImGui::SliderFloat(_("Wall gap"), &wall_gap, 0.0f, 100.0f);
    ImGui::SameLine();
    help_marker(_("The position is pushed back out to this distance from the wall plane every tick of the run (0 = off). "
                  "Stops the animation's root motion or a missed push from clipping through."));
    ImGui::SliderFloat(_("Cast length"), &cast_length, 10.0f, 400.0f);
    ImGui::SliderFloat(_("Cast height"), &cast_height, 0.0f, 200.0f);
    ImGui::SliderFloat(_("Cast angle"), &cast_angle, 5.0f, 85.0f);
    ImGui::SliderFloat(_("Corner dot"), &corner_dot, -1.0f, 1.0f);
    ImGui::SliderFloat(_("Climb angle"), &climb_angle, 0.0f, 90.0f);
    ImGui::SameLine();
    help_marker(_("Facing the wall within this many degrees of its normal runs up it; otherwise the run goes sideways."));
    ImGui::SliderFloat(_("Climb speed"), &climb_speed, 0.0f, 40.0f);
    ImGui::SliderFloat(_("Away deadzone"), &away_deadzone, 0.0f, 1.0f);
    ImGui::SameLine();
    help_marker(_("A stick leaning away from the wall only steers by its part along the wall; below this much it does nothing."));
    ImGui::Combo(_("Jumps after a run"), &jumps_after_run, "Costs one wall jump\0Costs nothing\0Reset to the maximum\0");
    ImGui::SameLine();
    help_marker(_("Entering the run spends a wall jump and counts an air jump, like the game's wall jump. "
                  "Costs nothing gives both back when the run ends; Reset fills them to the character's maxima."));
    ImGui::Checkbox(_("Custom wall run LMT"), &use_custom_lmt);
    ImGui::SameLine();
    help_marker(_("Off: the wall-jump kick-off motions stand in for the run cycles.\n"
                  "On: the LMT below is loaded into a free motion bank of the body and coat at runtime, and the run "
                  "plays bank:slot from it. The .ctn cancels of the placeholder motions are borrowed either way."));
    if (!use_custom_lmt) {
        ImGui::InputInt(_("Motion left"), &motion_left);
        ImGui::InputInt(_("Motion right"), &motion_right);
        ImGui::InputInt(_("Motion up"), &motion_up);
    } else {
        ImGui::SliderInt(_("Bank"), &lmt_bank, 10, 14);
        ImGui::InputInt(_("Slot left"), &run_slot_left);
        ImGui::InputInt(_("Slot right"), &run_slot_right);
        ImGui::InputInt(_("Slot up"), &run_slot_up);
        ImGui::InputText(_("Body LMT"), lmt_body, sizeof(lmt_body));
        ImGui::Checkbox(_("Load coat LMT too"), &load_coat_lmt);
        ImGui::SameLine();
        help_marker(_("Off: only the body plays the custom cycles and the coat holds its pose during the run."));
        if (load_coat_lmt) {
            ImGui::InputText(_("Nero coat LMT"), lmt_coat_nero, sizeof(lmt_coat_nero));
            ImGui::InputText(_("Dante coat LMT"), lmt_coat_dante, sizeof(lmt_coat_dante));
        }
        if (uPlayer* pl = devil4_sdk::get_local_player()) {
            ImGui::Text(_("File: %s | in use: %s"), lmt_file_exists(lmt_body) ? "found" : "missing",
                        custom_ready((uintptr_t)pl) ? "custom cycles" : "placeholders");
            ImGui::Text(_("Loaded: body %s, coat %s"),
                        devil4_sdk::get_motion_bank(pl, lmt_bank) ? "yes" : "no",
                        devil4_sdk::get_motion_bank((void*)at<uintptr_t>((uintptr_t)pl, pl::mpCoatMdl), lmt_bank) ? "yes" : "no");
            if (ImGui::Button(_("Reload LMT"))) {       // swap the slots out and let ensure_bank refill them
                devil4_sdk::unload_motion_bank(pl, lmt_bank);
                if (load_coat_lmt)
                    devil4_sdk::unload_motion_bank((void*)at<uintptr_t>((uintptr_t)pl, pl::mpCoatMdl), lmt_bank);
            }
        }
    }
    ImGui::Unindent(lineIndent);
}

void WallRun::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("wall_run").value_or(false);
    replace_wall_jump = cfg.get<bool>("wall_run_replace_wall_jump").value_or(true);
    run_speed   = cfg.get<float>("wall_run_speed").value_or(run_speed);
    run_frames  = cfg.get<float>("wall_run_frames").value_or(run_frames);
    wall_gap    = cfg.get<float>("wall_run_wall_gap").value_or(wall_gap);
    use_custom_lmt = cfg.get<bool>("wall_run_custom_lmt").value_or(false);
    lmt_bank       = cfg.get<int>("wall_run_bank").value_or(lmt_bank);
    run_slot_left  = cfg.get<int>("wall_run_slot_left").value_or(run_slot_left);
    run_slot_right = cfg.get<int>("wall_run_slot_right").value_or(run_slot_right);
    run_slot_up    = cfg.get<int>("wall_run_slot_up").value_or(run_slot_up);
    climb_angle    = cfg.get<float>("wall_run_climb_angle").value_or(climb_angle);
    climb_speed    = cfg.get<float>("wall_run_climb_speed").value_or(climb_speed);
    jumps_after_run = cfg.get<int>("wall_run_jumps_after").value_or(jumps_after_run);
    load_coat_lmt   = cfg.get<bool>("wall_run_coat_lmt").value_or(false);
}

void WallRun::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("wall_run", mod_enabled);
    cfg.set<bool>("wall_run_replace_wall_jump", replace_wall_jump);
    cfg.set<float>("wall_run_speed", run_speed);
    cfg.set<float>("wall_run_frames", run_frames);
    cfg.set<float>("wall_run_wall_gap", wall_gap);
    cfg.set<bool>("wall_run_custom_lmt", use_custom_lmt);
    cfg.set<int>("wall_run_bank", lmt_bank);
    cfg.set<int>("wall_run_slot_left", run_slot_left);
    cfg.set<int>("wall_run_slot_right", run_slot_right);
    cfg.set<int>("wall_run_slot_up", run_slot_up);
    cfg.set<float>("wall_run_climb_angle", climb_angle);
    cfg.set<float>("wall_run_climb_speed", climb_speed);
    cfg.set<int>("wall_run_jumps_after", jumps_after_run);
    cfg.set<bool>("wall_run_coat_lmt", load_coat_lmt);
}
