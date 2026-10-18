#include "WallRun.hpp"
#include "../sdk/Devil4.hpp"
#include <cmath>
#include <cstring>

using namespace wallrun;

bool  WallRun::mod_enabled  = false;
float WallRun::run_speed    = 12.0f;
float WallRun::run_frames   = 45.0f;
float WallRun::stick_speed  = 2.0f;
float WallRun::cast_length  = 120.0f;
float WallRun::cast_height  = 80.0f;
float WallRun::cast_angle   = 45.0f;
float WallRun::corner_dot   = 0.5f;    // normals more than 60 degrees apart = corner
int   WallRun::motion_left  = 23;      // placeholder: the wall-jump side kick-offs (L 23 / R 22, pl000_01 slots)
int   WallRun::motion_right = 22;

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
bool WallRun::detect(uintptr_t player, Wall& out) {
    const glm::vec3 right = wmat_row(player, 0);
    const glm::vec3 up    = wmat_row(player, 1);
    const glm::vec3 fwd   = wmat_row(player, 2);
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

    glm::vec3 run_dir = glm::cross(normal, up);                 // parallel to the wall, level with the ground
    if (glm::length(run_dir) < 1e-4f) return false;             // floor or ceiling, not a wall
    run_dir = glm::normalize(run_dir);
    if (glm::dot(run_dir, fwd) < 0.0f) run_dir = -run_dir;      // run the way we were already going

    out.normal  = normal;
    out.run_dir = run_dir;
    out.hit_pos = hits[nearest].hitPos;
    out.yaw     = yaw_of(run_dir);
    out.on_left = glm::dot(normal, right) > 0.0f;               // normal points at us from the left
    out.poly    = hits[nearest];
    return true;
}

// ---------------------------------------------------------------------------------------------------------------
// The move. Phase is kept here, not in mBdyAction: the attack dispatch doesn't give us a phase word.
// ---------------------------------------------------------------------------------------------------------------
namespace {

struct RunState {
    uintptr_t player = 0;
    int phase = 0;          // 0 start, 1 running, 2 finished (waiting to be re-entered)
    float timer = 0.0f;
    Wall wall{};
};

RunState g_runs[2];         // two players (co-op)

RunState& state_for(uintptr_t player) {
    for (RunState& s : g_runs) if (s.player == player) return s;
    for (RunState& s : g_runs) if (s.player == 0) { s.player = player; return s; }
    g_runs[0] = RunState{}; g_runs[0].player = player; return g_runs[0];
}

void face(uintptr_t pl, float yaw) {                 // snap, the way the wall jump does
    at<float>(pl, pl::mRotateTarget) = yaw;
    at<float>(pl, pl::mRotateNow)    = yaw;
    at<bool>(pl, pl::mRotateIsMove)  = false;
}

void leave_to_air(RunState& s, uintptr_t pl) {
    request_motion(pl, MOT_FALL, ATTR_FALL, 10.0f, 7.0f, 1.0f);
    devil4_sdk::neutral_air_transition((uPlayer*)pl);
    s.phase = 2;
}

// Hand the wall to the game's own wall jump: exactly what checkWallJumpBegin does after its cast.
void leave_to_wall_jump(RunState& s, uintptr_t pl) {
    at<LineResult>(pl, pl::mGroundPoly) = s.wall.poly;
    init_jump_begin(pl, 3);
    at<int>(pl, pl::mJumpMode) = JUMP_MODE_NORM_WALL_LO;
    set_action(pl, ACT_WALL_JUMP);
    s.phase = 2;
}

} // namespace

void __stdcall WallRun::move(uintptr_t pl) {
    RunState& s = state_for(pl);
    MtVector3& vel = at<MtVector3>(pl, pl::mVel);
    MtVector3& acc = at<MtVector3>(pl, pl::mAcc);

    switch (s.phase) {
    case 2:                                     // a previous run ended; the dispatch calling again = a new one
        s.phase = 0;
        [[fallthrough]];
    case 0:
        if (!detect(pl, s.wall)) { leave_to_air(s, pl); return; }
        face(pl, s.wall.yaw);
        acc.x = acc.y = acc.z = 0.0f;
        vel.y = 0.0f;
        s.timer = run_frames;
        request_motion(pl, s.wall.on_left ? motion_left : motion_right, ATTR_KICK, 4.0f, 0.0f, 1.0f);
        s.phase = 1;
        [[fallthrough]];
    case 1: {
        if (!detect(pl, s.wall)) { leave_to_air(s, pl); return; }       // wall ended, or a corner
        if (at<uint32_t>(pl, pl::mBtnTrg) & 0x4) { leave_to_wall_jump(s, pl); return; }

        face(pl, s.wall.yaw);
        const glm::vec3 v = glm::vec3(s.wall.run_dir) * run_speed - glm::vec3(s.wall.normal) * stick_speed;
        vel.x = v.x; vel.z = v.z;
        vel.y = 0.0f;                           // level run; gravity is reapplied by the fall motion's state
        acc.x = acc.y = acc.z = 0.0f;

        s.timer -= at<float>(pl, pl::m_delta_time);
        if (s.timer <= 0.0f) { leave_to_air(s, pl); return; }

        uintptr_t coll = at<uintptr_t>(pl, pl::mCollMgr);
        if (coll && at<bool>(coll, 0x1C)) { set_action_land(pl); s.phase = 2; }   // mLand
        return;
    }
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
// Mod plumbing. No hooks: MoveTable's dispatch calls WallRun::move.
// ---------------------------------------------------------------------------------------------------------------
std::optional<std::string> WallRun::on_initialize() {
    return Mod::on_initialize();
}

void WallRun::on_gui_frame(int display) {
    if (display != DISPLAY_DANTE_A) return;        // the custom-move dispatch exists for Dante only today
    ImGui::Checkbox(_("Wall Run"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Run along a wall found by two 45-degree casts. Jump kicks off into the game's wall jump."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::SliderFloat(_("Run speed"), &run_speed, 0.0f, 40.0f);
    ImGui::SliderFloat(_("Run frames"), &run_frames, 1.0f, 180.0f);
    ImGui::SliderFloat(_("Wall stick"), &stick_speed, 0.0f, 10.0f);
    ImGui::SliderFloat(_("Cast length"), &cast_length, 10.0f, 400.0f);
    ImGui::SliderFloat(_("Cast height"), &cast_height, 0.0f, 200.0f);
    ImGui::SliderFloat(_("Cast angle"), &cast_angle, 5.0f, 85.0f);
    ImGui::SliderFloat(_("Corner dot"), &corner_dot, -1.0f, 1.0f);
    ImGui::InputInt(_("Motion left"), &motion_left);
    ImGui::InputInt(_("Motion right"), &motion_right);
    ImGui::Unindent(lineIndent);
}

void WallRun::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("wall_run").value_or(false);
    run_speed   = cfg.get<float>("wall_run_speed").value_or(run_speed);
    run_frames  = cfg.get<float>("wall_run_frames").value_or(run_frames);
}

void WallRun::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("wall_run", mod_enabled);
    cfg.set<float>("wall_run_speed", run_speed);
    cfg.set<float>("wall_run_frames", run_frames);
}
