#include "SpineAim.hpp"
#include "WallRun.hpp"          // wallrun::at / wallrun::pl offsets; shares no state with WallRun
#include "../sdk/Devil4.hpp"
#include "../sdk/Math.hpp"      // glm quaternions: this tick's chain orientations by forward kinematics
#include <cmath>
#include <cstdio>
#include <cstring>

#undef naked                     // mod.hpp's `naked` is `static __declspec(naked)`; detour() is declared in the header
#define naked __declspec(naked)

using namespace wallrun;

namespace spine_aim {

bool  mod_enabled     = true;
bool  aim_always      = false;
float max_deg         = 70.0f;
float slew_deg        = 6.0f;
float share[3]        = { 0.30f, 0.45f, 0.25f };
float joint_max[3]    = { 20.0f, 25.0f, 40.0f };
float aim_gain        = 1.0f;
float aim_offset_deg  = 0.0f;
float unwind_deg      = 120.0f;
int   axis_mode       = 1;
int   ref_mode        = 1;
float return_slew_deg = 24.0f;
int   order           = 0;
bool  flip            = false;
bool  hips_flip       = false;
float test_deg        = 0.0f;
float test_hips_deg   = 0.0f;
uintptr_t jmp_ret     = 0;
uintptr_t jmp_ret_unpoke = 0;

namespace {

// uModel (the player is one): the joint array and the joint-number -> index table
constexpr uintptr_t kJointNum   = 0x02E0;
constexpr uintptr_t kpJoint     = 0x02E4;
constexpr uintptr_t kJointTable = 0x02E8;   // uchar[]: joint number -> index, 255 = the model lacks it
// uModel::Joint (0x90)
constexpr uintptr_t kJointSize  = 0x90;
constexpr uintptr_t kJSymmetry  = 0x08;     // byte: the mirror joint's INDEX (255 = none)
constexpr uintptr_t kJOffset    = 0x10;     // MtFloat3: the joint's position in its parent's frame
constexpr uintptr_t kJQuat      = 0x20;     // MtVector4 x y z w: the local rotation updateLocalMatrix wrote
constexpr uintptr_t kJTrans     = 0x40;     // MtVector3: the decoded local translation (the offset when the track is absent)
constexpr uintptr_t kJWmat      = 0x50;     // MtMatrix: last tick's world matrix until updateWorldMatrix runs
// uCoord / uPlayer
constexpr uintptr_t kModelQuat   = 0x40;    // uCoord::mQuat: the actor's orientation, built from the facing in uPlayer::main before this pass
constexpr uintptr_t kBdyUpperNo  = 0x14F8;  // mBdyAction[0].no: 12 attack (the shot), 13 / 16 the gun pose
constexpr uintptr_t kpLockTarget = 0x2000;  // cLockOnTarget*: +4 mpNext, +8 mpModel (the uActor*), +0x10 mPos = the lock-on point, +0x40 mNearPos
constexpr uintptr_t kLockPos     = 0x10;
constexpr int kSpine[3] = { 1, 2, 3 };       // spine_lower, spine_upper, neck; each one's child is the next number (4 = head)
constexpr int kClavicle = 5;                // right clavicle, under joint 2; its mirror is the left one

inline float yaw_of(const glm::vec3& d) { return std::atan2(d.x, d.z); }   // the game's yaw: dir = (sin, 0, cos)
inline float wrap_pi(float a) { while (a > 3.14159265f) a -= 6.2831853f; while (a < -3.14159265f) a += 6.2831853f; return a; }
inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline void slew(float& now, float want, float step) { now += clampf(want - now, -step, step); }

uintptr_t joint(uintptr_t model, int no) {
    const uint8_t* table = at<const uint8_t*>(model, kJointTable);
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    if (!table || !joints || no < 0 || no > 254) return 0;
    const uint32_t idx = table[no];
    if (idx == 255 || idx >= at<uint32_t>(model, kJointNum)) return 0;
    return joints + idx * kJointSize;
}

uintptr_t mirror_of(uintptr_t model, uintptr_t jp) {
    if (!jp) return 0;
    const uint32_t idx = at<uint8_t>(jp, kJSymmetry);
    if (idx == 255 || idx >= at<uint32_t>(model, kJointNum)) return 0;
    return at<uintptr_t>(model, kpJoint) + idx * kJointSize;
}

// Hamilton product o = a * b, MT layout x y z w
inline void qmul(const float a[4], const float b[4], float o[4]) {
    o[0] = a[3] * b[0] + a[0] * b[3] + a[1] * b[2] - a[2] * b[1];
    o[1] = a[3] * b[1] - a[0] * b[2] + a[1] * b[3] + a[2] * b[0];
    o[2] = a[3] * b[2] + a[0] * b[1] - a[1] * b[0] + a[2] * b[3];
    o[3] = a[3] * b[3] - a[0] * b[0] - a[1] * b[1] - a[2] * b[2];
}

// Rotate the joint's local quaternion by `deg` about the unit `axis` given in the joint's own frame (post-multiply
// under order 0, the variant settled in-game; `order` 1 swaps).
void rotate_local(uintptr_t jp, const glm::vec3& axis, float deg) {
    if (std::fabs(deg) < 1e-3f) return;
    const float h = deg * 3.14159265f / 360.0f, s = std::sin(h);
    const float q[4] = { axis.x * s, axis.y * s, axis.z * s, std::cos(h) };
    float* L = &at<float>(jp, kJQuat);
    float o[4];
    if (order == 0) qmul(L, q, o); else qmul(q, L, o);
    const float n = std::sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2] + o[3] * o[3]);
    if (n < 1e-6f) return;
    for (int i = 0; i < 4; ++i) L[i] = o[i] / n;
}

// The bone axis of joint `no`: its child's offset (expressed in this joint's frame), normalised.
bool bone_axis(uintptr_t model, int no, glm::vec3& out) {
    const uintptr_t child = joint(model, no + 1);
    if (!child) return false;
    const float* o = &at<float>(child, kJOffset);
    const glm::vec3 v(o[0], o[1], o[2]);
    const float len = glm::length(v);
    if (len < 1e-3f) return false;
    out = v / len;
    return true;
}

// An MT quaternion (x y z w) as glm's (w x y z): the same rotation, column form; rows of an MtMatrix = its columns
// (checked against uModel::updateWorldMatrix: standard terms, child = parent * local, position = parent + R * trans).
inline glm::quat mt_quat(const float* q) { return glm::quat(q[3], q[0], q[1], q[2]); }
inline glm::vec3 mt_vec(const float* v) { return glm::vec3(v[0], v[1], v[2]); }

// Per-player slewed state. Only players that twist get a slot (updateMatrix also runs for every shell the player
// fires, so a slot per visitor would thrash), 16 of them, least recently seen evicted.
struct State {
    uintptr_t player = 0;
    uint32_t  last_seen = 0;
    float aim = 0.0f;              // the current aim twist, degrees
    float lean = 0.0f;
    float hips = 0.0f;             // joint 0's counter-yaw (exact while provided, 0 otherwise)
    float ret = 0.0f;              // the spine's return twist (slewed)
    // The pokes are deltas on what the motion blend wrote this tick. A joint no playing motion has a track for is NOT
    // rewritten by updateLocalMatrix, so last tick's poke would still be in it and the deltas would pile up (Dante's
    // pelvis spun about the run axis under the lean). So the value last written is remembered: a joint still holding
    // it gets its pre-poke value back first. unpoke does the same right after the world pass, so the next tick's
    // motion snapshot (moveMotion) and interpolation (updateLocalMatrix) never start from a poked value.
    float base[4][4]  = {};        // joints 0..3: the local quaternion before this tick's pokes
    float poked[4][4] = {};        // what the pokes left in it
    bool  has_poked[4] = {};
};
constexpr int kPoked[4] = { 0, 1, 2, 3 };
State g_states[16];
uint32_t g_tick = 0;

State* find_state(uintptr_t player) {
    for (State& s : g_states) if (s.player == player) return &s;
    return nullptr;
}
State& state_for(uintptr_t player) {
    State* lru = &g_states[0];
    for (State& s : g_states) {
        if (s.player == player) { s.last_seen = g_tick; return s; }
        if (s.last_seen < lru->last_seen) lru = &s;
    }
    *lru = State{};
    lru->player = player; lru->last_seen = g_tick;
    return *lru;
}

Provider g_providers[8];
int g_provider_count = 0;

// readout
struct Debug { float err = 0, want = 0, now = 0, lean = 0, fk_check = 0, torso_deg = 0, hips = 0, ret = 0; int upper = -1; bool target = false, asked = false, from_provider = false; int joints = 0; } g_dbg;

} // namespace

bool add_provider(Provider fn) {
    if (!fn || g_provider_count >= 8) return false;
    g_providers[g_provider_count++] = fn;
    return true;
}

void __stdcall tick(uintptr_t pl) {
    if (!mod_enabled || !pl) return;
    ++g_tick;

    // who wants what
    Request req;
    for (int i = 0; i < g_provider_count; ++i) {
        Request r;
        if (!g_providers[i](pl, r)) continue;
        if (r.aim) req.aim = true;
        if (r.wanted_set) { req.wanted_set = true; req.wanted_yaw = r.wanted_yaw; }
        if (r.hips_set) { req.hips_set = true; req.hips_deg = r.hips_deg; req.return_deg = r.return_deg; }
        if (r.lean_deg != 0.0f) { req.lean_deg = r.lean_deg; req.lean_axis = r.lean_axis; }
    }
    const int upper = at<int>(pl, kBdyUpperNo);
    if (aim_always && (upper == 12 || upper == 13 || upper == 16)) req.aim = true;

    State* sp = find_state(pl);
    const bool wants = req.aim || req.wanted_set || req.hips_set || req.lean_deg != 0.0f || test_deg != 0.0f || test_hips_deg != 0.0f;
    if (!sp && !wants) return;                      // a visitor that twists nothing (a shell, a bystander): no slot
    State& s = sp ? *sp : state_for(pl);
    s.last_seen = g_tick;
    for (int k = 0; k < 4; ++k) {                   // undo a poke the engine didn't overwrite, remember the fresh value
        const uintptr_t jp = joint(pl, kPoked[k]);
        if (!jp) continue;
        float* q = &at<float>(jp, kJQuat);
        if (s.has_poked[k] && std::memcmp(q, s.poked[k], sizeof s.poked[k]) == 0) std::memcpy(q, s.base[k], sizeof s.base[k]);
        std::memcpy(s.base[k], q, sizeof s.base[k]);
    }

    const float dt = at<float>(pl, pl::m_delta_time);
    const float step = slew_deg * (dt > 0.0f ? dt : 1.0f);

    // Where the chest points THIS tick before any poke: forward kinematics over the local quaternions / translations
    // updateLocalMatrix just wrote (root = the actor's mQuat / mPos), down to the two clavicles; their line is the
    // chest's right axis (signed by the actor's own right, so the torso may turn up to 90 deg in the motion), and
    // right x up its forward.
    const glm::quat Rm = mt_quat(&at<float>(pl, kModelQuat));
    const glm::vec3 pm = mt_vec(&at<float>(pl, pl::mPos));
    glm::quat Rc = Rm; glm::vec3 pc = pm;                        // the chain's orientation / position, walked down
    const int chain[3] = { 0, 1, 2 };
    bool chest_ok = true;
    for (int no : chain) {
        const uintptr_t jp = joint(pl, no);
        if (!jp) { chest_ok = false; break; }
        pc = pc + Rc * mt_vec(&at<float>(jp, kJTrans));
        Rc = Rc * mt_quat(&at<float>(jp, kJQuat));
    }
    float chest_yaw = at<float>(pl, pl::mRotateNow);
    if (chest_ok) {
        const uintptr_t cr = joint(pl, kClavicle), cl = mirror_of(pl, cr);
        if (cr && cl) {
            const glm::vec3 pr = pc + Rc * mt_vec(&at<float>(cr, kJTrans)), plf = pc + Rc * mt_vec(&at<float>(cl, kJTrans));
            glm::vec3 right = pr - plf;
            if (glm::dot(right, Rm * glm::vec3(1.0f, 0.0f, 0.0f)) < 0.0f) right = -right;
            right.y = 0.0f;
            const glm::vec3 fwd = glm::cross(right, glm::vec3(0.0f, 1.0f, 0.0f));
            if (glm::length(fwd) > 1e-3f) chest_yaw = yaw_of(fwd); else chest_ok = false;
        } else chest_ok = false;
    }
    g_dbg.torso_deg = wrap_pi(chest_yaw - at<float>(pl, pl::mRotateNow)) * 180.0f / 3.14159265f;

    // the aim: reference yaw - wanted yaw, clamped (the sense settled in-game with order 0)
    float err_deg = 0.0f, want = 0.0f, wanted = 0.0f;
    bool have_wanted = false;
    const uintptr_t target = at<uintptr_t>(pl, kpLockTarget);
    if (req.wanted_set) { wanted = req.wanted_yaw; have_wanted = true; }
    else if (req.aim && target) {                                               // the fallback: the lock-on point, as setLockOnTargetAngle reads it
        const MtVector3& tp = at<MtVector3>(target, kLockPos);
        const glm::vec3 d(tp.x - pm.x, 0.0f, tp.z - pm.z);
        if (glm::length(d) > 1.0f) { wanted = yaw_of(d); have_wanted = true; }
    }
    if (test_deg != 0.0f) want = test_deg;
    else if (have_wanted) {
        const float ref = (ref_mode == 1 && chest_ok) ? chest_yaw : at<float>(pl, pl::mRotateNow);
        err_deg = wrap_pi(ref - wanted) * 180.0f / 3.14159265f;
        if (std::fabs(err_deg) > 150.0f && s.aim * err_deg < 0.0f) err_deg = -err_deg;   // target behind: keep the side we are on
        if (std::fabs(err_deg) > unwind_deg) want = 0.0f;                                // too far round to aim at: back to neutral
        else want = clampf(err_deg * aim_gain + (err_deg < 0.0f ? -aim_offset_deg : aim_offset_deg), -max_deg, max_deg);
    }
    slew(s.aim, want, step);
    slew(s.lean, req.lean_deg, step);
    s.hips = test_hips_deg != 0.0f ? test_hips_deg : (req.hips_set ? req.hips_deg : 0.0f);   // exact: the legs must face their way every frame
    slew(s.ret, req.return_deg, return_slew_deg * (dt > 0.0f ? dt : 1.0f));
    g_dbg.err = err_deg; g_dbg.want = want; g_dbg.now = s.aim; g_dbg.lean = s.lean; g_dbg.upper = upper;
    g_dbg.target = target != 0; g_dbg.asked = req.aim || req.wanted_set; g_dbg.from_provider = req.wanted_set; g_dbg.joints = 0;
    g_dbg.hips = s.hips; g_dbg.ret = s.ret;

    // The chain's orientations THIS tick, by forward kinematics over the local quaternions, recomputed after each poke
    // so a child's axis sees its parent's new turn. (Last tick's mWmat carried last tick's twist and lean into each
    // other's axis: a one-tick feedback between two rotations about different axes, which oscillated.)
    const float sign = flip ? -1.0f : 1.0f;
    const float hsign = hips_flip ? -1.0f : 1.0f;
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::quat R = Rm;
    if (const uintptr_t j0 = joint(pl, 0)) {
        if (std::fabs(s.hips) >= 0.01f) {                                 // the hips (and so the legs) turned about world up
            const glm::quat R0 = R * mt_quat(&at<float>(j0, kJQuat));
            rotate_local(j0, glm::normalize(glm::inverse(R0) * up), hsign * s.hips);
        }
        R = R * mt_quat(&at<float>(j0, kJQuat));
    }
    // the spine's return twist, split by the shares within the joint limits; what a joint can't take goes to the others
    float ret_k[3] = { 0, 0, 0 };
    if (std::fabs(s.ret) >= 0.01f) {
        float left = s.ret;
        for (int k = 0; k < 3; ++k) { ret_k[k] = clampf(s.ret * share[k], -joint_max[k], joint_max[k]); left -= ret_k[k]; }
        for (int pass = 0; pass < 2 && std::fabs(left) > 0.01f; ++pass)
            for (int k = 0; k < 3; ++k) {
                const float room = joint_max[k] - std::fabs(ret_k[k]);
                if (room <= 0.0f) continue;
                const float take = clampf(left, -room, room);
                ret_k[k] += take; left -= take;
            }
    }
    bool checked = false;
    for (int k = 0; k < 3; ++k) {
        const uintptr_t jp = joint(pl, kSpine[k]);
        if (!jp) break;                                                   // the chain is 0 -> 1 -> 2 -> 3 on both players
        glm::quat Rj = R * mt_quat(&at<float>(jp, kJQuat));               // this joint before the pokes
        if (!checked) {                                                   // convention check: FK vs last tick's matrix (a still pose: ~0)
            const float* m = &at<float>(jp, kJWmat);
            const glm::vec3 z_fk = Rj * glm::vec3(0.0f, 0.0f, 1.0f), z_mat = glm::normalize(glm::vec3(m[8], m[9], m[10]));
            g_dbg.fk_check = glm::degrees(std::acos(clampf(glm::dot(z_fk, z_mat), -1.0f, 1.0f)));
            checked = true;
        }
        if (k == 0 && std::fabs(s.lean) >= 0.01f) {                       // the lean: joint 1 about the world axis the provider gave
            rotate_local(jp, glm::normalize(glm::inverse(Rj) * req.lean_axis), s.lean);
            Rj = R * mt_quat(&at<float>(jp, kJQuat));
        }
        const float aim_k = std::fabs(s.aim) >= 0.01f ? clampf(s.aim * share[k], -joint_max[k], joint_max[k]) : 0.0f;
        const float total = sign * aim_k + hsign * ret_k[k];
        if (std::fabs(total) >= 0.01f) {                                  // this joint's turn: aim share + return share
            glm::vec3 axis;
            bool ok = true;
            if (axis_mode == 1) axis = glm::normalize(glm::inverse(Rj) * up);   // world up in this joint's frame, as it is now
            else ok = bone_axis(pl, kSpine[k], axis);
            if (ok) {
                ++g_dbg.joints;
                rotate_local(jp, axis, total);
                Rj = R * mt_quat(&at<float>(jp, kJQuat));
            }
        }
        R = Rj;
    }

    for (int k = 0; k < 4; ++k) {                   // what the pokes left behind, for next tick's undo
        const uintptr_t jp = joint(pl, kPoked[k]);
        if (!jp) { s.has_poked[k] = false; continue; }
        std::memcpy(s.poked[k], &at<float>(jp, kJQuat), sizeof s.poked[k]);
        s.has_poked[k] = true;
    }
    if (!wants && std::fabs(s.aim) < 0.01f && std::fabs(s.lean) < 0.01f && std::fabs(s.ret) < 0.01f) {
        // settled back to nothing: put the joints back once more (a poke nobody overwrites would stay) and free the slot
        for (int k = 0; k < 4; ++k) if (const uintptr_t jp = joint(pl, kPoked[k])) std::memcpy(&at<float>(jp, kJQuat), s.base[k], sizeof s.base[k]);
        if (State* own = find_state(pl)) *own = State{};
    }
}

// 0x7A8D4A in uPlayer::updateMatrix: the 7 bytes replaced are `mov eax,[esi]; mov edx,[eax+38h]; mov ecx,esi`
naked void detour() {
    _asm {
        pushad
        push esi
        call tick               // __stdcall: pops its argument
        popad
        mov eax, [esi]
        mov edx, [eax + 0x38]
        mov ecx, esi
        jmp dword ptr [jmp_ret]
    }
}

void __stdcall unpoke(uintptr_t pl) {
    if (!pl) return;
    State* s = find_state(pl);
    if (!s) return;
    for (int k = 0; k < 4; ++k) {
        if (!s->has_poked[k]) continue;
        s->has_poked[k] = false;
        const uintptr_t jp = joint(pl, kPoked[k]);
        if (!jp) continue;
        float* q = &at<float>(jp, kJQuat);
        if (std::memcmp(q, s->poked[k], sizeof s->poked[k]) == 0) std::memcpy(q, s->base[k], sizeof s->base[k]);   // ours, untouched since
    }
}

// 0x7A8D53 in uPlayer::updateMatrix, after the updateWorldMatrix call (vtbl +0x38): the 6 bytes replaced are
// `lea eax,[esi+1D88h]` (then `pop esi; jmp cWeaponCtrl::updateMatrix`, which reads the joints' world matrices).
naked void detour_unpoke() {
    _asm {
        pushad
        push esi
        call unpoke             // __stdcall: pops its argument
        popad
        lea eax, [esi + 0x1D88]
        jmp dword ptr [jmp_ret_unpoke]
    }
}

} // namespace spine_aim

// ---------------------------------------------------------------------------------------------------------------
using namespace spine_aim;

std::optional<std::string> SpineAim::on_initialize() {
    if (!install_hook_absolute(0x7A8D4A, hook, &detour, &jmp_ret, 7)) {
        spdlog::error("SpineAim: failed to hook uPlayer::updateMatrix (0x7A8D4A)");
        return "Failed to init SpineAim";
    }
    if (!install_hook_absolute(0x7A8D53, hook_unpoke, &detour_unpoke, &jmp_ret_unpoke, 6)) {
        spdlog::error("SpineAim: failed to hook uPlayer::updateMatrix after updateWorldMatrix (0x7A8D53)");
        return "Failed to init SpineAim (unpoke hook)";
    }
    return Mod::on_initialize();
}

void SpineAim::on_config_load(const utility::Config& cfg) {
    mod_enabled     = cfg.get<bool>("spine_aim_enabled").value_or(mod_enabled);
    aim_always      = cfg.get<bool>("spine_aim_always").value_or(aim_always);
    max_deg         = cfg.get<float>("spine_aim_max").value_or(max_deg);
    slew_deg        = cfg.get<float>("spine_aim_slew").value_or(slew_deg);
    share[0]        = cfg.get<float>("spine_aim_share1").value_or(share[0]);
    share[1]        = cfg.get<float>("spine_aim_share2").value_or(share[1]);
    share[2]        = cfg.get<float>("spine_aim_share3").value_or(share[2]);
    joint_max[0]    = cfg.get<float>("spine_aim_limit1").value_or(joint_max[0]);
    joint_max[1]    = cfg.get<float>("spine_aim_limit2").value_or(joint_max[1]);
    joint_max[2]    = cfg.get<float>("spine_aim_limit3").value_or(joint_max[2]);
    aim_gain        = cfg.get<float>("spine_aim_gain").value_or(aim_gain);
    aim_offset_deg  = cfg.get<float>("spine_aim_offset").value_or(aim_offset_deg);
    unwind_deg      = cfg.get<float>("spine_aim_unwind").value_or(unwind_deg);
    axis_mode       = cfg.get<int>("spine_aim_axis").value_or(axis_mode);
    ref_mode        = cfg.get<int>("spine_aim_ref").value_or(ref_mode);
    return_slew_deg = cfg.get<float>("spine_aim_return_slew").value_or(return_slew_deg);
    order           = cfg.get<int>("spine_aim_order").value_or(order);
    flip            = cfg.get<bool>("spine_aim_flip").value_or(flip);
    hips_flip       = cfg.get<bool>("spine_aim_hips_flip").value_or(hips_flip);
}

void SpineAim::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("spine_aim_enabled", mod_enabled);
    cfg.set<bool>("spine_aim_always", aim_always);
    cfg.set<float>("spine_aim_max", max_deg);
    cfg.set<float>("spine_aim_slew", slew_deg);
    cfg.set<float>("spine_aim_share1", share[0]);
    cfg.set<float>("spine_aim_share2", share[1]);
    cfg.set<float>("spine_aim_share3", share[2]);
    cfg.set<float>("spine_aim_limit1", joint_max[0]);
    cfg.set<float>("spine_aim_limit2", joint_max[1]);
    cfg.set<float>("spine_aim_limit3", joint_max[2]);
    cfg.set<float>("spine_aim_gain", aim_gain);
    cfg.set<float>("spine_aim_offset", aim_offset_deg);
    cfg.set<float>("spine_aim_unwind", unwind_deg);
    cfg.set<int>("spine_aim_axis", axis_mode);
    cfg.set<int>("spine_aim_ref", ref_mode);
    cfg.set<float>("spine_aim_return_slew", return_slew_deg);
    cfg.set<int>("spine_aim_order", order);
    cfg.set<bool>("spine_aim_flip", flip);
    cfg.set<bool>("spine_aim_hips_flip", hips_flip);
}

void SpineAim::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Spine aim"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Procedural spine and hips rotations for a move that asks for them (none does today: kept for a third-person / "
                  "aiming mode): a twist of the spine toward a target, a hips counter-yaw with the spine giving it back, a lean. "
                  "Applied after the motion blend and before the skeleton is built (uPlayer::updateMatrix), so the arms, coat and "
                  "hitboxes follow."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("SpineAim");
    ImGui::Checkbox(_("Aim on every gun pose (debug)"), &aim_always);
    ImGui::SameLine();
    help_marker(_("Twist toward the lock-on point on every gun pose even when no move asks. On the ground the game turns the whole "
                  "body itself, so the twist shows during that turn."));
    ImGui::SliderFloat(_("Max angle"), &max_deg, 0.0f, 120.0f);
    ImGui::SliderFloat(_("Speed (deg per frame)"), &slew_deg, 0.5f, 30.0f);
    ImGui::SliderFloat(_("Gain"), &aim_gain, 0.5f, 2.0f);
    ImGui::SliderFloat(_("Offset (deg)"), &aim_offset_deg, -45.0f, 45.0f);
    ImGui::SliderFloat(_("Share spine lower"), &share[0], 0.0f, 1.0f);
    ImGui::SliderFloat(_("Share spine upper"), &share[1], 0.0f, 1.0f);
    ImGui::SliderFloat(_("Share neck"), &share[2], 0.0f, 1.0f);
    ImGui::SliderFloat(_("Limit spine lower"), &joint_max[0], 0.0f, 60.0f);
    ImGui::SliderFloat(_("Limit spine upper"), &joint_max[1], 0.0f, 60.0f);
    ImGui::SliderFloat(_("Limit neck"), &joint_max[2], 0.0f, 90.0f);
    ImGui::SameLine();
    help_marker(_("The spine constraint: no joint turns past its own limit whatever the shares and the error ask, so the spine "
                  "bends toward the target instead of coiling."));
    ImGui::SliderFloat(_("Unwind beyond (deg)"), &unwind_deg, 60.0f, 180.0f);
    ImGui::SliderFloat(_("Return speed (deg per frame)"), &return_slew_deg, 1.0f, 60.0f);
    ImGui::SameLine();
    help_marker(_("How fast the spine gives a hips counter-yaw back (a provider's return_deg); the game turns an actor at 24."));
    ImGui::SliderInt(_("Twist axis"), &axis_mode, 0, 1);
    ImGui::SameLine();
    help_marker(_("1 = world up in each joint's frame: a true yaw. 0 = each joint's own bone axis: anatomical, but on a leaning "
                  "spine part of it becomes pitch."));
    ImGui::SliderInt(_("Reference"), &ref_mode, 0, 1);
    ImGui::SameLine();
    help_marker(_("What the wanted yaw is measured against. 1 = the chest's own yaw this tick (the clavicle line, before any "
                  "twist), which absorbs whatever torso angle the playing motion holds. 0 = the actor's facing."));
    ImGui::SliderInt(_("Quaternion order"), &order, 0, 1);
    ImGui::SameLine();
    help_marker(_("0 is the one that turns the torso about its own axis (settled in-game); 1 bends it sideways instead."));
    ImGui::Checkbox(_("Flip"), &flip);
    ImGui::SameLine();
    ImGui::Checkbox(_("Hips flip"), &hips_flip);
    ImGui::SameLine();
    help_marker(_("Flip negates the aim twist; Hips flip negates the hips counter-yaw and the spine's return together."));
    ImGui::SliderFloat(_("Test angle"), &test_deg, -90.0f, 90.0f);
    ImGui::SliderFloat(_("Test hips"), &test_hips_deg, -90.0f, 90.0f);
    ImGui::SameLine();
    help_marker(_("Constants for calibration: Test angle twists the spine, Test hips rotates joint 0 (the root: legs and spine hang "
                  "off it) about world up, so the whole character must yaw by it standing still."));
    ImGui::Text("hips %.1f | return %.1f | aim err %.1f (%s) want %.1f now %.1f | lean %.1f | torso vs facing %.1f | upper %d | target %s | joints %d | fk check %.1f",
                g_dbg.hips, g_dbg.ret, g_dbg.err, g_dbg.from_provider ? "provider" : "own", g_dbg.want, g_dbg.now, g_dbg.lean, g_dbg.torso_deg,
                g_dbg.upper, g_dbg.target ? "yes" : "no", g_dbg.joints, g_dbg.fk_check);
    ImGui::SameLine();
    help_marker(_("fk check: angle between the spine's orientation computed from this tick's local rotations and last tick's world "
                  "matrix. Near 0 while standing still means the quaternion convention is right."));
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}
