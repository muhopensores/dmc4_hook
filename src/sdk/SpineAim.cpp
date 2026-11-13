#include "SpineAim.hpp"
#include "../mods/WallRun.hpp"  // wallrun::at / wallrun::pl offsets; shares no state with WallRun
#include "../utility/FunctionHook.hpp"
#include "../utility/Patch.hpp"
#include "Devil4.hpp"
#include "Math.hpp"             // glm quaternions: this tick's chain orientations by forward kinematics
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>
#include <vector>

#undef naked                     // a precompiled mod.hpp's `naked` is `static __declspec(naked)`; detour_unpoke() is declared in the header
#define naked __declspec(naked)

using namespace wallrun;

namespace spine_aim {

uintptr_t jmp_ret_unpoke = 0;
constexpr uintptr_t kUpdateLocalMatrix = 0x9E87E0;   // uModel::updateLocalMatrix, what the players' vtable slot 13 held
constexpr uintptr_t kLocalSlotNero  = 0xBE4FD4;      // uPlayerNero::vftable slot 13 (+0x34)
constexpr uintptr_t kLocalSlotDante = 0xBE3C84;      // uPlayerDante::vftable slot 13 (the boss's too: it derives)
constexpr uintptr_t kUnpokeSite     = 0x9F89D0;      // uModel::moveMotion entry

namespace {

// the settled values (tuned in-game on the wall run and ThirdPersonAim, 2026-10-25)
constexpr float kMaxDeg        = 70.0f;                        // the aim twist is clamped to +-this
constexpr float kSlewDeg       = 6.0f;                         // degrees per frame the aim twist and the lean may change
constexpr float kShare[3]      = { 0.30f, 0.45f, 0.25f };      // how a twist splits over joints 1, 2, 3
constexpr float kJointMax[3]   = { 20.0f, 25.0f, 40.0f };      // the spine constraint: no joint turns past its own limit
constexpr float kUnwindDeg     = 120.0f;                       // further round than this the aim goes back to neutral
constexpr float kReturnSlewDeg = 24.0f;                        // the spine's return twist (the game turns an actor at 24)

// uModel (the player is one): the joint array and the joint-number -> index table
constexpr uintptr_t kJointNum   = 0x02E0;
constexpr uintptr_t kpJoint     = 0x02E4;
constexpr uintptr_t kJointTable = 0x02E8;   // uchar[]: joint number -> index, 255 = the model lacks it
// uModel::Joint (0x90)
constexpr uintptr_t kJointSize  = 0x90;
constexpr uintptr_t kJSymmetry  = 0x08;     // byte: the mirror joint's INDEX (255 = none)
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

// Rotate the joint's local quaternion by `deg` about the unit `axis` given in the joint's own frame (post-multiplied:
// the variant settled in-game).
void rotate_local(uintptr_t jp, const glm::vec3& axis, float deg) {
    if (std::fabs(deg) < 1e-3f) return;
    const float h = deg * 3.14159265f / 360.0f, s = std::sin(h);
    const float q[4] = { axis.x * s, axis.y * s, axis.z * s, std::cos(h) };
    float* L = &at<float>(jp, kJQuat);
    float o[4];
    qmul(L, q, o);
    const float n = std::sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2] + o[3] * o[3]);
    if (n < 1e-6f) return;
    for (int i = 0; i < 4; ++i) L[i] = o[i] / n;
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
    // it gets its pre-poke value back first. unpoke does the same at the next tick's moveMotion entry, so the motion
    // snapshot and the interpolation (updateLocalMatrix) never start from a poked value.
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

// --- inertialization (see the header) ---
// One offset per joint and channel: x(t) = A t^5 + B t^4 + C t^3 + a0 / 2 t^2 + v0 t + x0, with x(tf) = x'(tf) = x''(tf)
// = 0, in frames. x0 >= 0 along `axis` (rotation: radians about it, translation: cm along it); v0 is clamped to <= 0
// (no overshoot) and tf shortened to -5 x0 / v0 when that is sooner, as in the talk.
constexpr int kInertJoints = 256;
struct Fade {
    float x0 = 0, v0 = 0, a0 = 0, A = 0, B = 0, C = 0, tf = 0;
    float axis[3] = {};
};
void make_fade(Fade& f, float x0, float v0, float tf, const float axis[3]) {
    f.tf = 0.0f;
    if (x0 < 1e-5f) return;
    if (v0 > 0.0f) v0 = 0.0f;
    if (v0 < 0.0f) tf = std::min(tf, -5.0f * x0 / v0);
    if (tf < 1e-2f) return;
    float a0 = (-8.0f * v0 * tf - 20.0f * x0) / (tf * tf);
    if (a0 < 0.0f) a0 = 0.0f;
    const float tf2 = tf * tf, tf3 = tf2 * tf, tf4 = tf3 * tf, tf5 = tf4 * tf;
    f.x0 = x0; f.v0 = v0; f.a0 = a0; f.tf = tf;
    f.A = -(a0 * tf2 + 6.0f * v0 * tf + 12.0f * x0) / (2.0f * tf5);
    f.B =  (3.0f * a0 * tf2 + 16.0f * v0 * tf + 30.0f * x0) / (2.0f * tf4);
    f.C = -(3.0f * a0 * tf2 + 12.0f * v0 * tf + 20.0f * x0) / (2.0f * tf3);
    for (int i = 0; i < 3; ++i) f.axis[i] = axis[i];
}
inline float fade_at(const Fade& f, float t) {
    if (t >= f.tf) return 0.0f;
    return ((((f.A * t + f.B) * t + f.C) * t + 0.5f * f.a0) * t + f.v0) * t + f.x0;
}

// Per player: the pose shown the last two motion steps (before the pokes), the fades, and what they wrote (so the next
// pass gets the engine's values back: an interpolating slot lerps from the joints, as with the pokes). Big (~50 KB), so
// slots are cleared field by field, never copied.
struct Inert {
    uintptr_t player;
    uint32_t  step, hist_step, last_seen;
    int   n;                                       // joints recorded (the model's count, at most kInertJoints)
    bool  have_last, have_last2;
    float last_dt;                                 // frames between the two recorded steps
    bool  pending;  float pending_frames;
    bool  active;   float t, t_end;
    int   fading;                                  // readout: joints with a fade
    float last_q[kInertJoints][4],  last_t[kInertJoints][3];
    float last2_q[kInertJoints][4], last2_t[kInertJoints][3];
    Fade  rot[kInertJoints], pos[kInertJoints];
    bool  written[kInertJoints];
    float base_q[kInertJoints][4], base_t[kInertJoints][3], out_q[kInertJoints][4], out_t[kInertJoints][3];
};
Inert g_inert[4];
uint32_t g_inert_clock = 0;

Inert* find_inert(uintptr_t model) {
    for (Inert& in : g_inert) if (in.player == model) return &in;
    return nullptr;
}
Inert& inert_for(uintptr_t model) {
    Inert* lru = &g_inert[0];
    for (Inert& in : g_inert) {
        if (in.player == model) { in.last_seen = g_inert_clock; return in; }
        if (in.last_seen < lru->last_seen) lru = &in;
    }
    Inert& in = *lru;
    in.player = model; in.step = in.hist_step = 0; in.last_seen = g_inert_clock; in.n = 0;
    in.have_last = in.have_last2 = false; in.last_dt = 1.0f;
    in.pending = in.active = false; in.pending_frames = in.t = in.t_end = 0.0f; in.fading = 0;
    std::memset(in.written, 0, sizeof in.written);
    return in;
}

// the joints back to the engine's values where a fade's value is still in them
void inert_undo(Inert& in, uintptr_t model) {
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    for (int j = 0; j < in.n; ++j) {
        if (!in.written[j]) continue;
        in.written[j] = false;
        if (!joints) continue;
        const uintptr_t jp = joints + j * kJointSize;
        float* q = &at<float>(jp, kJQuat);
        float* p = &at<float>(jp, kJTrans);
        if (std::memcmp(q, in.out_q[j], sizeof in.out_q[j]) == 0) std::memcpy(q, in.base_q[j], sizeof in.base_q[j]);
        if (std::memcmp(p, in.out_t[j], sizeof in.out_t[j]) == 0) std::memcpy(p, in.base_t[j], sizeof in.base_t[j]);
    }
}

// The fades from the pose shown last step to this pass's (the new motion's), with the speed of the last two steps.
void inert_start(Inert& in, uintptr_t joints, float frames) {
    in.active = false; in.t_end = 0.0f; in.fading = 0;
    for (int j = 0; j < in.n; ++j) {
        const uintptr_t jp = joints + j * kJointSize;
        const float* tq = &at<float>(jp, kJQuat);
        const float* tt = &at<float>(jp, kJTrans);
        const float conj[4] = { -tq[0], -tq[1], -tq[2], tq[3] };
        // rotation: q0 = last * conj(target), so last = q0 * target
        float q0[4];
        qmul(in.last_q[j], conj, q0);
        if (q0[3] < 0.0f) for (float& c : q0) c = -c;
        const float s = std::sqrt(q0[0] * q0[0] + q0[1] * q0[1] + q0[2] * q0[2]);
        in.rot[j].tf = 0.0f;
        if (s > 1e-6f) {
            const float axis[3] = { q0[0] / s, q0[1] / s, q0[2] / s };
            const float x0 = 2.0f * std::atan2(s, q0[3]);
            float v0 = 0.0f;
            if (in.have_last2) {   // the step before, measured about the same axis against the same target
                float qm[4];
                qmul(in.last2_q[j], conj, qm);
                const float xm = wrap_pi(2.0f * std::atan2(qm[0] * axis[0] + qm[1] * axis[1] + qm[2] * axis[2], qm[3]));
                v0 = (x0 - xm) / in.last_dt;
            }
            make_fade(in.rot[j], x0, v0, frames, axis);
        }
        // translation: last = target + x0 * dir
        const float d[3] = { in.last_t[j][0] - tt[0], in.last_t[j][1] - tt[1], in.last_t[j][2] - tt[2] };
        const float len = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
        in.pos[j].tf = 0.0f;
        if (len > 1e-3f) {
            const float dir[3] = { d[0] / len, d[1] / len, d[2] / len };
            float v0 = 0.0f;
            if (in.have_last2) {
                const float xm = (in.last2_t[j][0] - tt[0]) * dir[0] + (in.last2_t[j][1] - tt[1]) * dir[1] + (in.last2_t[j][2] - tt[2]) * dir[2];
                v0 = (len - xm) / in.last_dt;
            }
            make_fade(in.pos[j], len, v0, frames, dir);
        }
        if (in.rot[j].tf > 0.0f || in.pos[j].tf > 0.0f) ++in.fading;
        in.t_end = std::max(in.t_end, std::max(in.rot[j].tf, in.pos[j].tf));
    }
    in.active = in.t_end > 0.0f;
}

// After the engine's local pass: start a requested fade, apply the running ones, record the pose shown.
void inert_pass(uintptr_t model) {
    ++g_inert_clock;
    Inert& in = inert_for(model);
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    const int n = (int)std::min<uint32_t>(at<uint32_t>(model, kJointNum), kInertJoints);
    if (!joints || n <= 0) return;
    if (n != in.n) { in.n = n; in.have_last = in.have_last2 = false; in.pending = in.active = false; }
    const bool new_step = !in.have_last || in.hist_step != in.step;   // a second local pass in the same tick repeats this one
    const float dt_raw = at<float>(model, pl::m_delta_time);
    const float dt = dt_raw > 0.0f ? dt_raw : 1.0f;

    if (new_step && in.active) in.t += dt;
    if (new_step && in.pending) {
        in.pending = false;
        if (in.have_last) {
            inert_start(in, joints, in.pending_frames);
            in.t = dt;   // the first shown step is one step on: the joints carry on from last step at their speed
        }
    }
    if (in.active && in.t >= in.t_end) in.active = false;
    if (in.active) {
        for (int j = 0; j < n; ++j) {
            const Fade& fr = in.rot[j];
            const Fade& fp = in.pos[j];
            const float xr = fade_at(fr, in.t), xp = fade_at(fp, in.t);
            if (xr == 0.0f && xp == 0.0f) continue;
            const uintptr_t jp = joints + j * kJointSize;
            float* q = &at<float>(jp, kJQuat);
            float* p = &at<float>(jp, kJTrans);
            std::memcpy(in.base_q[j], q, sizeof in.base_q[j]);
            std::memcpy(in.base_t[j], p, sizeof in.base_t[j]);
            if (xr != 0.0f) {
                const float h = 0.5f * xr, s = std::sin(h);
                const float dq[4] = { fr.axis[0] * s, fr.axis[1] * s, fr.axis[2] * s, std::cos(h) };
                float o[4];
                qmul(dq, q, o);
                const float len = std::sqrt(o[0] * o[0] + o[1] * o[1] + o[2] * o[2] + o[3] * o[3]);
                if (len > 1e-6f) for (int i = 0; i < 4; ++i) q[i] = o[i] / len;
            }
            if (xp != 0.0f) for (int i = 0; i < 3; ++i) p[i] += fp.axis[i] * xp;
            std::memcpy(in.out_q[j], q, sizeof in.out_q[j]);
            std::memcpy(in.out_t[j], p, sizeof in.out_t[j]);
            in.written[j] = true;
        }
    }
    // the pose shown this step (before the pokes): the start of the next fade
    if (new_step) {
        if (in.have_last) {
            std::memcpy(in.last2_q, in.last_q, sizeof(float) * 4 * n);
            std::memcpy(in.last2_t, in.last_t, sizeof(float) * 3 * n);
            in.have_last2 = true;
            in.last_dt = dt;
        }
        in.hist_step = in.step;
    }
    for (int j = 0; j < n; ++j) {
        const uintptr_t jp = joints + j * kJointSize;
        std::memcpy(in.last_q[j], &at<float>(jp, kJQuat), sizeof in.last_q[j]);
        std::memcpy(in.last_t[j], &at<float>(jp, kJTrans), sizeof in.last_t[j]);
    }
    in.have_last = true;
}

} // namespace

bool inertialize(uintptr_t player, float frames) {
    if (!player || frames <= 0.0f) return false;
    Inert* in = find_inert(player);
    if (!in || !in->have_last) return false;     // no pose recorded yet: nothing to fade from
    if (!in->pending || frames > in->pending_frames) in->pending_frames = frames;
    in->pending = true;
    return true;
}

bool add_provider(Provider fn) {
    if (!fn || g_provider_count >= 8) { spdlog::error("SpineAim: no room for another provider"); return false; }
    if (!install()) return false;
    g_providers[g_provider_count++] = fn;
    return true;
}

void __stdcall tick(uintptr_t pl) {
    if (!pl) return;
    ++g_tick;

    // who wants what
    Request req;
    for (int i = 0; i < g_provider_count; ++i) {
        Request r;
        if (!g_providers[i](pl, r)) continue;
        if (r.aim) req.aim = true;
        if (r.wanted_set) { req.wanted_set = true; req.wanted_yaw = r.wanted_yaw; req.ref_facing = r.ref_facing; req.aim_arms = r.aim_arms; }
        if (r.hips_set) { req.hips_set = true; req.hips_deg = r.hips_deg; req.return_deg = r.return_deg; }
        if (r.lean_deg != 0.0f) { req.lean_deg = r.lean_deg; req.lean_axis = r.lean_axis; }
    }
    const int upper = at<int>(pl, kBdyUpperNo);

    State* sp = find_state(pl);
    const bool wants = req.aim || req.wanted_set || req.hips_set || req.lean_deg != 0.0f;
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
    const float step = kSlewDeg * (dt > 0.0f ? dt : 1.0f);

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

    // the aim: the wanted yaw against the reference, clamped
    float err_deg = 0.0f, want = 0.0f, wanted = 0.0f;
    bool have_wanted = false;
    const uintptr_t target = at<uintptr_t>(pl, kpLockTarget);
    if (req.wanted_set) { wanted = req.wanted_yaw; have_wanted = true; }
    else if (req.aim && target) {                                               // the fallback: the lock-on point, as setLockOnTargetAngle reads it
        const MtVector3& tp = at<MtVector3>(target, kLockPos);
        const glm::vec3 d(tp.x - pm.x, 0.0f, tp.z - pm.z);
        if (glm::length(d) > 1.0f) { wanted = yaw_of(d); have_wanted = true; }
    }
    if (have_wanted) {
        const float ref = (chest_ok && !req.ref_facing) ? chest_yaw : at<float>(pl, pl::mRotateNow);
        // the turn that brings the reference onto the wanted yaw (a positive rotation about up raises the yaw; the
        // other sense twisted away from the target)
        err_deg = wrap_pi(wanted - ref) * 180.0f / 3.14159265f;
        if (std::fabs(err_deg) > 150.0f && s.aim * err_deg < 0.0f) err_deg = -err_deg;   // target behind: keep the side we are on
        if (std::fabs(err_deg) > kUnwindDeg) want = 0.0f;                               // too far round to aim at: back to neutral
        else want = clampf(err_deg, -kMaxDeg, kMaxDeg);
    }
    slew(s.aim, want, step);
    slew(s.lean, req.lean_deg, step);
    s.hips = req.hips_set ? req.hips_deg : 0.0f;   // exact: the legs must face their way every frame
    slew(s.ret, req.return_deg, kReturnSlewDeg * (dt > 0.0f ? dt : 1.0f));
    g_dbg.err = err_deg; g_dbg.want = want; g_dbg.now = s.aim; g_dbg.lean = s.lean; g_dbg.upper = upper;
    g_dbg.target = target != 0; g_dbg.asked = req.aim || req.wanted_set; g_dbg.from_provider = req.wanted_set; g_dbg.joints = 0;
    g_dbg.hips = s.hips; g_dbg.ret = s.ret;

    // The chain's orientations THIS tick, by forward kinematics over the local quaternions, recomputed after each poke
    // so a child's axis sees its parent's new turn. (Last tick's mWmat carried last tick's twist and lean into each
    // other's axis: a one-tick feedback between two rotations about different axes, which oscillated.)
    const glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::quat R = Rm;
    if (const uintptr_t j0 = joint(pl, 0)) {
        if (std::fabs(s.hips) >= 0.01f) {                                 // the hips (and so the legs) turned about world up
            const glm::quat R0 = R * mt_quat(&at<float>(j0, kJQuat));
            rotate_local(j0, glm::normalize(glm::inverse(R0) * up), s.hips);
        }
        R = R * mt_quat(&at<float>(j0, kJQuat));
    }
    // the spine's return twist, split by the shares within the joint limits; what a joint can't take goes to the others
    float ret_k[3] = { 0, 0, 0 };
    if (std::fabs(s.ret) >= 0.01f) {
        float left = s.ret;
        for (int k = 0; k < 3; ++k) { ret_k[k] = clampf(s.ret * kShare[k], -kJointMax[k], kJointMax[k]); left -= ret_k[k]; }
        for (int pass = 0; pass < 2 && std::fabs(left) > 0.01f; ++pass)
            for (int k = 0; k < 3; ++k) {
                const float room = kJointMax[k] - std::fabs(ret_k[k]);
                if (room <= 0.0f) continue;
                const float take = clampf(left, -room, room);
                ret_k[k] += take; left -= take;
            }
    }
    // the aim twist per joint: the shares within the joint limits; with aim_arms the whole twist on joints 1 and 2 (the
    // arms hang off 2 through the clavicles, so the neck's share turned only the head and a gun fell short by it), the
    // shares renormalised over the two and what one can't take spilled to the other
    float aim_k[3] = { 0, 0, 0 };
    if (std::fabs(s.aim) >= 0.01f) {
        if (req.aim_arms) {
            const float sum = kShare[0] + kShare[1];
            float left = s.aim;
            for (int k = 0; k < 2; ++k) { aim_k[k] = clampf(s.aim * kShare[k] / sum, -kJointMax[k], kJointMax[k]); left -= aim_k[k]; }
            for (int k = 0; k < 2 && std::fabs(left) > 0.01f; ++k) {
                const float room = kJointMax[k] - std::fabs(aim_k[k]);
                if (room <= 0.0f) continue;
                const float take = clampf(left, -room, room);
                aim_k[k] += take; left -= take;
            }
        } else {
            for (int k = 0; k < 3; ++k) aim_k[k] = clampf(s.aim * kShare[k], -kJointMax[k], kJointMax[k]);
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
        const float total = aim_k[k] + ret_k[k];
        if (std::fabs(total) >= 0.01f) {                                  // this joint's turn: aim share + return share,
            ++g_dbg.joints;                                               // about world up in this joint's frame as it is now
            rotate_local(jp, glm::normalize(glm::inverse(Rj) * up), total);
            Rj = R * mt_quat(&at<float>(jp, kJQuat));
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

// The players' local pass, then the pokes: uPlayerNero / uPlayerDante vtable slot 13 (+0x34, uModel::updateLocalMatrix
// 0x9E87E0, thiscall, no arguments) points here. uPlayer::updateMatrix calls it before the world pass; so does whatever
// re-runs the player's joints during a shot (with a hook inside updateMatrix only, the twist vanished for the shot).
// The pokes come off first: a second local pass in the same frame blended (interpolating slots lerp from the current
// value) from joints still holding the first pass's poke, and the poke went on again on top: the wall run's lean piled
// up and spun Dante's hips through rapid E&I shots again (2026-10-25).
// The inertialization offsets go on between the two (inert_pass), so the pokes measure from the faded pose.
void __fastcall local_pass(uintptr_t model, uintptr_t) {
    unpoke(model);
    ((void(__thiscall*)(uintptr_t))kUpdateLocalMatrix)(model);
    inert_pass(model);
    tick(model);
}

// The pokes come off first (they went on last), then the fades' offsets.
void __stdcall unpoke(uintptr_t pl) {
    if (!pl) return;
    if (State* s = find_state(pl)) {
        for (int k = 0; k < 4; ++k) {
            if (!s->has_poked[k]) continue;
            s->has_poked[k] = false;
            const uintptr_t jp = joint(pl, kPoked[k]);
            if (!jp) continue;
            float* q = &at<float>(jp, kJQuat);
            if (std::memcmp(q, s->poked[k], sizeof s->poked[k]) == 0) std::memcpy(q, s->base[k], sizeof s->base[k]);   // ours, untouched since
        }
    }
    if (Inert* in = find_inert(pl)) inert_undo(*in, pl);
}

void __stdcall step_begin(uintptr_t model) {
    unpoke(model);
    if (Inert* in = find_inert(model)) ++in->step;
}

// uModel::moveMotion 0x9F89D0 (__stdcall, the model on the stack), its entry: the 6 bytes replaced are
// `push ebp; mov ebp, esp; and esp, -8`.
naked void detour_unpoke() {
    _asm {
        pushad
        push dword ptr [esp + 36]   // the model (above the 8 saved registers and the return address)
        call step_begin             // __stdcall: pops its argument
        popad
        push ebp
        mov ebp, esp
        and esp, 0xFFFFFFF8
        jmp dword ptr [jmp_ret_unpoke]
    }
}

// The hooks, ModBase's install_hook_absolute / install_patch_absolute without a Mod: the players' vtable slot 13 points
// at local_pass, uModel::moveMotion's entry runs step_begin.
namespace {
std::unique_ptr<FunctionHook> g_hook_unpoke;
Patch::Ptr g_patch_local_nero, g_patch_local_dante;
int g_installed = 0;   // 0 not tried, 1 installed, -1 failed

Patch::Ptr slot_patch(uintptr_t slot) {
    const uintptr_t fn = (uintptr_t)&local_pass;
    std::vector<int16_t> bytes;
    for (int i = 0; i < 4; ++i) bytes.push_back((int16_t)((fn >> (8 * i)) & 0xFF));
    spdlog::info("SpineAim: patching vtable slot {:x}", slot);
    return Patch::create(slot, bytes, true);
}
} // namespace

bool install() {
    if (g_installed) return g_installed > 0;
    spdlog::info("SpineAim: installing hook at {:x}", kUnpokeSite);
    g_hook_unpoke = std::make_unique<FunctionHook>(kUnpokeSite, &detour_unpoke);
    if (!g_hook_unpoke->create()) {
        spdlog::error("SpineAim: failed to hook uModel::moveMotion ({:x})", kUnpokeSite);
        g_hook_unpoke.reset();
        g_installed = -1;
        return false;
    }
    jmp_ret_unpoke = kUnpokeSite + 6;   // `push ebp; mov ebp, esp; and esp, -8`
    g_patch_local_nero = slot_patch(kLocalSlotNero);
    g_patch_local_dante = slot_patch(kLocalSlotDante);
    g_installed = 1;
    return true;
}

void draw_readout(uintptr_t player) {
    ImGui::Text("spine: hips %.1f | return %.1f | aim err %.1f (%s) want %.1f now %.1f | lean %.1f | torso vs facing %.1f | upper %d | target %s | joints %d | fk check %.1f",
                g_dbg.hips, g_dbg.ret, g_dbg.err, g_dbg.from_provider ? "provider" : "own", g_dbg.want, g_dbg.now, g_dbg.lean,
                g_dbg.torso_deg, g_dbg.upper, g_dbg.target ? "yes" : "no", g_dbg.joints, g_dbg.fk_check);
    const Inert* in = player ? find_inert(player) : nullptr;
    if (in && in->active) ImGui::Text("spine: fading %d joints, %.1f / %.1f frames", in->fading, in->t, in->t_end);
    else ImGui::TextUnformatted("spine: fading none");
}

} // namespace spine_aim
