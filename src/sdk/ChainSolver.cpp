#include "ChainSolver.hpp"
#include "Collision.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <chrono>
#include <climits>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace chain_solver {

namespace {
// cCnsChain (RE note chain_cnschain.md section 2)
constexpr uintptr_t kBlend = 0x04;              // the current blend (fades toward +0x10)
constexpr uintptr_t kFixedStep = 0x18, kEnable = 0x19;
constexpr uintptr_t kBones = 0x20;              // mBone[32], s16 joint numbers, -1 between tendrils
constexpr uintptr_t kDir = 0x4E0, kUp = 0x4E4, kTailLength = 0x4E8, kGravity = 0x4F0;
constexpr uintptr_t kDamping = 0x53C, kMaxSpeed = 0x540, kFloorLevel = 0x544, kStretch = 0x548, kStretchLimit = 0x54C;
constexpr uintptr_t kSpring = 0x538;
constexpr uintptr_t kWindNow = 0x550;           // mWind + mWindSoftCtrl + turbulence, this frame (cCnsChain::move)
constexpr uintptr_t kMats = 0x560, kVels = 0xDA0;   // mat[33] (0x40), vel[33] (0x10)
constexpr uintptr_t kModel = 0xFB4, kCollModel = 0xFB8, kCollShape = 0xFBC;
constexpr uintptr_t kCollSize = 0xFC0, kParentMode = 0xFC4;
constexpr uintptr_t kPosLocal = 0xFC8, kPosLocalY = 0xFC9, kRotLocal = 0xFCA, kWindLocal = 0xFCB, kGravityLocal = 0xFCC;
constexpr uintptr_t kMuteCollision = 0xFCD, kMuteWind = 0xFCE, kDemoDriven = 0xFD8;
constexpr uintptr_t kResetState = 0xFD0;       // byte: 1 while the chain copies the animation (the byte after it is set at runtime)
constexpr uintptr_t kStep = 0x10B0;             // t = work rate x delta x time scale, clamped to 2 (move)
constexpr int kSlots = 32;
// uModel / its joints (0x90 each)
constexpr uintptr_t kModelWmat = 0xA0, kCoordParent = 0x18;
constexpr uintptr_t kJointNum = 0x2E0, kpJoint = 0x2E4, kJointTable = 0x2E8, kJointSize = 0x90;
constexpr uintptr_t kJParent = 0x05, kJNo = 0x07, kJQuat = 0x20, kJScale = 0x30, kJTrans = 0x40, kJWmat = 0x50;
// rCollisionShape / cCollGroup / cCollPrim -> the file's 64-byte prim
constexpr uintptr_t kShapeGroupNum = 0x64, kShapeGroups = 0x68, kGroupPrimNum = 0x04, kGroupPrims = 0x08, kPrimData = 0x0C;
constexpr uintptr_t kDevil4Main = 0xE5574C;     // sDevil4Main::mpInstance, +0x68 mDeltaTime (60 fps frames)
constexpr float kMaxPending = 4.0f;             // frames one solve may cover
constexpr float kTeleport = 500.0f;             // cm a root may jump in a frame before the chain snaps to the animation

template <class T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }
glm::vec3 v3(uintptr_t p) { return { at<float>(p, 0), at<float>(p, 4), at<float>(p, 8) }; }

// An MT matrix: rows x, y, z (the axes, scaled), position; a point p maps to p.x row0 + p.y row1 + p.z row2 + row3.
struct Rows {
    glm::vec3 r[3], t;
    static Rows load(const float* m) {
        Rows o;
        for (int i = 0; i < 3; ++i) o.r[i] = { m[i * 4], m[i * 4 + 1], m[i * 4 + 2] };
        o.t = { m[12], m[13], m[14] };
        return o;
    }
    void store(float* m) const {
        for (int i = 0; i < 3; ++i) { m[i * 4] = r[i].x; m[i * 4 + 1] = r[i].y; m[i * 4 + 2] = r[i].z; m[i * 4 + 3] = 0.0f; }
        m[12] = t.x; m[13] = t.y; m[14] = t.z; m[15] = 1.0f;
    }
    glm::vec3 point(const glm::vec3& p) const { return p.x * r[0] + p.y * r[1] + p.z * r[2] + t; }
    glm::vec3 dir(const glm::vec3& d) const { return d.x * r[0] + d.y * r[1] + d.z * r[2]; }
};

glm::vec3 safe_normalize(const glm::vec3& v, const glm::vec3& fallback) {
    const float l = glm::length(v);
    return l > 1e-6f ? v / l : fallback;
}

// rotation rows <-> quaternion (glm column form: the rows are the columns)
glm::quat rows_quat(const glm::vec3 r[3]) { return glm::quat_cast(glm::mat3(r[0], r[1], r[2])); }
void quat_rows(const glm::quat& q, glm::vec3 r[3]) {
    const glm::mat3 m = glm::mat3_cast(q);
    r[0] = m[0]; r[1] = m[1]; r[2] = m[2];
}

// The world pass's FK for a joint under `parent` (uModel::updateWorldMatrix 0x9EA8B0): the parent's rows normalised, the
// joint's row i scaled by its own scale i times the parent's row-i length, its translation by the parent's row lengths.
Rows fk(uintptr_t joint, const Rows& parent) {
    float l[3];
    glm::vec3 pn[3];
    for (int i = 0; i < 3; ++i) {
        l[i] = glm::length(parent.r[i]);
        pn[i] = l[i] >= 1.1920929e-7f ? parent.r[i] / l[i] : parent.r[i];
    }
    const float x = at<float>(joint, kJQuat), y = at<float>(joint, kJQuat + 4), z = at<float>(joint, kJQuat + 8), w = at<float>(joint, kJQuat + 12);
    const glm::vec3 q[3] = {
        { 1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + w * z), 2.0f * (x * z - w * y) },
        { 2.0f * (x * y - w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + w * x) },
        { 2.0f * (x * z + w * y), 2.0f * (y * z - w * x), 1.0f - 2.0f * (x * x + y * y) },
    };
    const glm::vec3 s = v3(joint + kJScale), tr = v3(joint + kJTrans);
    Rows o;
    for (int i = 0; i < 3; ++i) o.r[i] = (q[i].x * pn[0] + q[i].y * pn[1] + q[i].z * pn[2]) * (s[i] * l[i]);
    o.t = tr.x * l[0] * pn[0] + tr.y * l[1] * pn[1] + tr.z * l[2] * pn[2] + parent.t;
    return o;
}

glm::vec3 axis_of(const Rows& m, int axis) {   // 0-5: +x +y +z -x -y -z, unit
    const glm::vec3 a = safe_normalize(m.r[axis % 3], glm::vec3(0.0f, 1.0f, 0.0f));
    return axis >= 3 ? -a : a;
}

// The closest points of segments p0-p1 and q0-q1 (Ericson 5.1.9): s, t in [0, 1].
void closest(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& q0, const glm::vec3& q1, float& s, float& t) {
    const glm::vec3 d1 = p1 - p0, d2 = q1 - q0, r = p0 - q0;
    const float a = glm::dot(d1, d1), e = glm::dot(d2, d2), f = glm::dot(d2, r);
    s = t = 0.0f;
    if (a <= 1e-8f && e <= 1e-8f) return;
    if (a <= 1e-8f) { t = std::clamp(f / e, 0.0f, 1.0f); return; }
    const float c = glm::dot(d1, r);
    if (e <= 1e-8f) { s = std::clamp(-c / a, 0.0f, 1.0f); return; }
    const float b = glm::dot(d1, d2), denom = a * e - b * b;
    s = denom > 1e-8f ? std::clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
    t = (b * s + f) / e;
    if (t < 0.0f) { t = 0.0f; s = std::clamp(-c / a, 0.0f, 1.0f); }
    else if (t > 1.0f) { t = 1.0f; s = std::clamp((b - c) / a, 0.0f, 1.0f); }
}

struct Capsule { glm::vec3 a, b; float r; };
struct Plane { glm::vec3 n, p; };

// The .col shapes on the collision model, in world space (cCnsChain::collide 0x438DB0's reading: type 0 sphere = mPos0 in
// joint mBone0's frame, type 3 capsule = mPos0 in mBone0's to mPos1 in mBone1's; others and joint 255 skipped).
void gather_shapes(uintptr_t chain, std::vector<Capsule>& out) {
    const uintptr_t shape = at<uintptr_t>(chain, kCollShape), model = at<uintptr_t>(chain, kCollModel);
    if (!shape || !model || at<uint8_t>(chain, kMuteCollision)) return;
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    const uint8_t* table = at<const uint8_t*>(model, kJointTable);
    const uint32_t num = at<uint32_t>(model, kJointNum);
    if (!joints || !table || !num) return;
    auto frame = [&](int no, Rows& m) {
        if (no < 0 || no >= 255) return false;
        const uint8_t i = table[no];
        if (i == 0xFF || i >= num) return false;
        m = Rows::load(&at<float>(joints + i * kJointSize, kJWmat));
        return true;
    };
    const uint32_t groups = at<uint32_t>(shape, kShapeGroupNum);
    const uintptr_t* g = at<const uintptr_t*>(shape, kShapeGroups);
    for (uint32_t gi = 0; g && gi < groups; ++gi) {
        if (!g[gi]) continue;
        const uint32_t prims = at<uint32_t>(g[gi], kGroupPrimNum);
        const uintptr_t* p = at<const uintptr_t*>(g[gi], kGroupPrims);
        for (uint32_t pi = 0; p && pi < prims; ++pi) {
            const uintptr_t d = p[pi] ? at<uintptr_t>(p[pi], kPrimData) : 0;
            if (!d) continue;
            const int type = at<int>(d, 0x00), b0 = at<int>(d, 0x04), b1 = at<int>(d, 0x08);
            Rows m0, m1;
            if (type == 0 && frame(b0, m0)) {
                const glm::vec3 c = m0.point(v3(d + 0x10));
                out.push_back({ c, c, at<float>(d, 0x0C) });
            } else if (type == 3 && frame(b0, m0) && frame(b1, m1)) {
                out.push_back({ m0.point(v3(d + 0x10)), m1.point(v3(d + 0x20)), at<float>(d, 0x0C) });
            }
        }
    }
}

struct State {                                  // per chain; the per-tendril parts are indexed by the root's slot
    float pending[kSlots] = {};                 // frames since the tendril last solved
    bool ref_valid[kSlots] = {};
    Rows ref_prev[kSlots];
    bool seen[kSlots + 1] = {};                 // the node has a previous animated place
    glm::vec3 anim_prev[kSlots + 1];
    Rows out[kSlots];
    bool out_valid[kSlots] = {};
};

std::mutex g_lock;
std::unordered_map<uintptr_t, std::unique_ptr<State>> g_states;
float g_last_ms = 0.0f;

std::mutex g_cloth_lock;                        // the sheets, swapped in by set_cloths (main thread), read by the solves
std::vector<Cloth> g_cloths;

State* state_of(uintptr_t chain) {
    std::lock_guard<std::mutex> lock(g_lock);
    auto& s = g_states[chain];
    if (!s) s = std::make_unique<State>();
    return s.get();
}

int bone(uintptr_t chain, int slot) { return slot >= 0 && slot < kSlots ? at<int16_t>(chain, kBones + slot * 2) : -1; }
bool is_root(uintptr_t chain, int slot) { return bone(chain, slot) >= 0 && (slot == 0 || bone(chain, slot - 1) < 0); }

float rate_per(float rate, float h) { return 1.0f - std::pow(1.0f - std::clamp(rate, 0.0f, 1.0f), h); }

// The FK world matrix of joint `index` of `model` while the world pass is at joint `current`: joints up to `current`
// already hold theirs (constraints applied); a later one is composed from its parents' as the pass will (the model's own
// matrix for a root joint).
Rows fk_world(uintptr_t model, int index, int current) {
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    const uintptr_t j = joints + index * kJointSize;
    if (index <= current) return Rows::load(&at<float>(j, kJWmat));
    const uint8_t parent = at<uint8_t>(j, kJParent);
    if (parent == 0xFF || parent >= at<uint32_t>(model, kJointNum)) {
        // updateWorldMatrix's root case: the joint's local rows (scaled) times the model's matrix, scale and all
        const Rows m = Rows::load(&at<float>(model, kModelWmat));
        Rows unit;
        unit.r[0] = { 1, 0, 0 }; unit.r[1] = { 0, 1, 0 }; unit.r[2] = { 0, 0, 1 }; unit.t = { 0, 0, 0 };
        const Rows local = fk(j, unit);
        Rows o;
        for (int i = 0; i < 3; ++i) o.r[i] = m.dir(local.r[i]);
        o.t = m.point(local.t);
        return o;
    }
    return fk(j, fk_world(model, parent, current));
}

// One tendril being solved: its chain's settings and its points, root..tail.
struct Tendril {
    uintptr_t chain = 0, model = 0;
    State* st = nullptr;
    Settings s;
    int root = 0, tip = 0, tail = 0, axis = 0, up = 0;
    int index[kSlots] = {};
    Rows A[kSlots + 1];
    glm::vec3 a[kSlots + 1], x[kSlots + 1], v[kSlots + 1], old[kSlots + 1], contact[kSlots + 1], at_[kSlots + 1];
    float L[kSlots + 1] = {};
    bool touched[kSlots + 1] = {};
    bool reset = false;
    // per frame
    glm::vec3 g, wind;
    float damp_rate, max_speed, r, spring_rate, floor_level, stretch_limit;
    bool stretch;
    std::vector<Capsule> shapes;
    Plane ground[kSlots + 1];
    bool has_ground[kSlots + 1] = {};
    struct Seg { glm::vec3 a, b; };
    std::vector<Seg> others;
    std::vector<int> neighbours;                // root slots of this chain's sheet neighbours (no self collision with them)
    // per substep
    float damp, pin, stiff, fric;

    uintptr_t jptr(int k) const { return at<uintptr_t>(model, kpJoint) + index[k] * kJointSize; }
    int nodes() const { return tail - root; }   // after the root
    float weight(int k) const { return k == root ? 0.0f : 1.0f; }

    // Steps 1-4 of chain_solver.md: the animated pose (the root's FK as the world pass has it at joint `current`), the
    // points, the reset, the inertia carry. False: a joint is missing (the tendril is left to its FK).
    bool prepare(uintptr_t chain_, uintptr_t model_, int root_, const Settings& s_, int current) {
        chain = chain_; model = model_; root = root_; s = s_;
        st = state_of(chain);
        const uintptr_t joints = at<uintptr_t>(model, kpJoint);
        const uint8_t* table = at<const uint8_t*>(model, kJointTable);
        const uint32_t jnum = at<uint32_t>(model, kJointNum);
        if (!joints || !table || !jnum || !at<uint8_t>(chain, kEnable)) return false;
        tip = root;
        while (tip + 1 < kSlots && bone(chain, tip + 1) >= 0) ++tip;
        tail = tip + 1;                         // the tail sits in the -1 slot after the tip
        for (int k = root; k <= tip; ++k) {
            const uint8_t i = table[bone(chain, k) & 0xFF];
            if (i == 0xFF || i >= jnum) { for (int j = root; j <= tip; ++j) st->out_valid[j] = false; return false; }
            index[k] = i;
        }
        axis = s.axis >= 0 ? s.axis : std::clamp((int)at<uint32_t>(chain, kDir), 0, 5);
        up = (int)(at<uint32_t>(chain, kUp) % 3);

        A[root] = fk_world(model, index[root], current);
        for (int k = root + 1; k <= tip; ++k) A[k] = fk(jptr(k), A[k - 1]);
        {
            const Rows& m = A[tip];
            const float scale = (glm::length(m.r[0]) + glm::length(m.r[1]) + glm::length(m.r[2])) / 3.0f;
            A[tail] = m;
            A[tail].t = m.t + axis_of(m, axis) * at<float>(chain, kTailLength) * scale;
        }
        for (int k = root; k <= tail; ++k) a[k] = A[k].t;
        for (int k = root; k < tail; ++k) L[k] = glm::length(a[k + 1] - a[k]);

        reset = !st->seen[root] || (at<uint8_t>(chain, kResetState) == 1 && !at<uint8_t>(chain, kDemoDriven))
                || glm::length(a[root] - st->anim_prev[root]) > kTeleport;
        for (int k = root; k <= tail; ++k) {
            const float* m = &at<float>(chain, kMats + k * 0x40);
            x[k] = reset ? a[k] : glm::vec3(m[12], m[13], m[14]);
            v[k] = reset ? glm::vec3(0.0f) : v3(chain + kVels + k * 0x10);
            if (reset || !st->seen[k]) st->anim_prev[k] = a[k];
            st->seen[k] = true;
        }
        x[root] = a[root];

        // the reference frame: the model's world matrix, or its parent's (mParentMode 1)
        uintptr_t ref_unit = model;
        if (at<uint32_t>(chain, kParentMode) == 1 && at<uintptr_t>(model, kCoordParent)) ref_unit = at<uintptr_t>(model, kCoordParent);
        const Rows ref = Rows::load(&at<float>(ref_unit, kModelWmat));
        glm::vec3 ref_rot[3];
        for (int i = 0; i < 3; ++i) ref_rot[i] = safe_normalize(ref.r[i], glm::vec3(i == 0, i == 1, i == 2));
        auto ref_dir = [&](const glm::vec3& d) { return d.x * ref_rot[0] + d.y * ref_rot[1] + d.z * ref_rot[2]; };
        if (!reset) {
            // inertia: the part of the frame's reference motion the points are carried by (Local flags: all of it)
            float inertia = s.inertia;
            if (inertia < 0.0f) inertia = (at<uint8_t>(chain, kPosLocal) || at<uint8_t>(chain, kPosLocalY) || at<uint8_t>(chain, kRotLocal)) ? 0.0f : 1.0f;
            const float carry = 1.0f - std::clamp(inertia, 0.0f, 1.0f);
            if (carry > 0.0f && st->ref_valid[root] && st->pending[root] > 0.0f) {
                // p -> p . inv(prev) . cur, with the rows normalised (scale changes aren't carried)
                const Rows& prev = st->ref_prev[root];
                glm::vec3 pr[3];
                for (int i = 0; i < 3; ++i) pr[i] = safe_normalize(prev.r[i], glm::vec3(i == 0, i == 1, i == 2));
                auto carry_dir = [&](const glm::vec3& d) {
                    const glm::vec3 local(glm::dot(d, pr[0]), glm::dot(d, pr[1]), glm::dot(d, pr[2]));
                    return local.x * ref_rot[0] + local.y * ref_rot[1] + local.z * ref_rot[2];
                };
                for (int k = root + 1; k <= tail; ++k) {
                    x[k] = glm::mix(x[k], carry_dir(x[k] - prev.t) + ref.t, carry);
                    v[k] = glm::mix(v[k], carry_dir(v[k]), carry);
                }
            }
        }
        st->ref_prev[root] = ref;
        st->ref_valid[root] = true;

        g = v3(chain + kGravity);
        if (at<uint8_t>(chain, kGravityLocal)) g = ref_dir(g);
        wind = at<uint8_t>(chain, kMuteWind) ? glm::vec3(0.0f) : v3(chain + kWindNow);
        if (at<uint8_t>(chain, kWindLocal)) wind = ref_dir(wind);
        // the speed kept per frame. The game's mDamping can't be used as it is: its solver drops the speed a length
        // correction would give (RE note 3.2g), so 1.0 (Nero's and Dante's coats) still loses energy there; here the
        // corrections are speed, and 1.0 would swing for good
        damp_rate = 1.0f - (s.damping >= 0.0f ? s.damping : std::max(0.1f, 1.0f - std::clamp(at<float>(chain, kDamping), 0.0f, 1.0f)));
        max_speed = at<float>(chain, kMaxSpeed);
        r = s.radius >= 0.0f ? s.radius : at<float>(chain, kCollSize);
        stretch = at<uint8_t>(chain, kStretch) != 0;
        stretch_limit = at<float>(chain, kStretchLimit);
        spring_rate = at<float>(chain, kSpring);
        floor_level = at<float>(chain, kFloorLevel);
        return true;
    }

    // Before the substeps: the shapes, the ground under each point, the other tendrils' segments (self).
    void gather() {
        shapes.clear();
        gather_shapes(chain, shapes);
        for (int k = root + 1; k <= tail; ++k) {
            has_ground[k] = false;
            if (!s.ground) continue;
            const float top = std::max(x[k].y, a[k].y) + 50.0f, bottom = std::min(x[k].y, a[k].y) - 100.0f;
            MtLineSegment seg{};
            seg.p0 = glm::vec3(x[k].x, top, x[k].z);
            seg.p1 = glm::vec3(x[k].x, bottom, x[k].z);
            collision::CollParam q = collision::line_param(collision::COLL_GROUND);
            collision::LineResult hit{};
            if (collision::find_intersection(&q, &seg, false, &hit)) {
                const MtVector3 nn = collision::tri_normal(&hit);
                glm::vec3 nrm(nn.x, nn.y, nn.z);
                if (nrm.y < 0.0f) nrm = -nrm;
                ground[k] = { safe_normalize(nrm, glm::vec3(0.0f, 1.0f, 0.0f)), glm::vec3(hit.hitPos.x, hit.hitPos.y, hit.hitPos.z) };
                has_ground[k] = true;
            }
        }
        others.clear();
        if (s.self)
            for (int k = 0; k < kSlots; ++k) {
                if (bone(chain, k) < 0 || (k >= root && k <= tip)) continue;
                int owner = k;                  // the root of k's tendril: sheet neighbours don't collide
                while (owner > 0 && bone(chain, owner - 1) >= 0) --owner;
                if (std::find(neighbours.begin(), neighbours.end(), owner) != neighbours.end()) continue;
                const float* m0 = &at<float>(chain, kMats + k * 0x40);
                const float* m1 = &at<float>(chain, kMats + (k + 1) * 0x40);
                others.push_back({ { m0[12], m0[13], m0[14] }, { m1[12], m1[13], m1[14] } });
            }
    }

    void begin_substep(float h, float f) {
        damp = std::pow(damp_rate, h);
        pin = rate_per(s.pin, h);
        stiff = rate_per(s.stiffness, h);
        fric = rate_per(s.friction, h);
        for (int k = root; k <= tail; ++k) at_[k] = glm::mix(st->anim_prev[k], a[k], f);
        x[root] = at_[root];
        // the root pushed out of the shapes as a sphere of the chain's radius: the game keeps a root's push too (RE note
        // 3.2g step 3), and a root that sits inside a shape at bind (Nero's coat _02 / _03) would hold its first segment
        // in it for good
        for (const Capsule& c : shapes) {
            float u, w;
            closest(x[root], x[root], c.a, c.b, u, w);
            const glm::vec3 q = glm::mix(c.a, c.b, w), d = x[root] - q;
            const float dist = glm::length(d), pen = r + c.r - dist;
            if (pen > 0.0f) x[root] += safe_normalize(d, glm::vec3(0.0f, 1.0f, 0.0f)) * pen;
        }
        for (int k = root + 1; k <= tail; ++k) {
            v[k] = (v[k] + (g + wind) * h) * damp;
            old[k] = x[k];
            x[k] += v[k] * h;
            touched[k] = false;
        }
    }

    // One constraint pass up to the lengths (stiffness and pin in the first pass only).
    void constrain(bool first) {
        if (first && (pin > 0.0f || stiff > 0.0f))
            for (int k = root + 1; k <= tail; ++k) {
                if (stiff > 0.0f) {
                    const float len = glm::length(x[k] - x[k - 1]);
                    const glm::vec3 d = safe_normalize(at_[k] - at_[k - 1], glm::vec3(0.0f, -1.0f, 0.0f));
                    x[k] += (x[k - 1] + d * len - x[k]) * stiff;
                }
                if (pin > 0.0f) x[k] += (at_[k] - x[k]) * pin;
            }
        // shapes: each segment's closest point pushed out, split over its two ends (the root doesn't move)
        for (int k = root; k < tail; ++k)
            for (const Capsule& c : shapes) {
                float u, w;
                closest(x[k], x[k + 1], c.a, c.b, u, w);
                const glm::vec3 p = glm::mix(x[k], x[k + 1], u), q = glm::mix(c.a, c.b, w);
                const float dist = glm::length(p - q), pen = r + c.r - dist;
                if (pen <= 0.0f) continue;
                const glm::vec3 nrm = safe_normalize(p - q, safe_normalize(x[k + 1] - c.a, glm::vec3(0.0f, 1.0f, 0.0f)));
                const float wa = k == root ? 0.0f : 1.0f - u, wb = u, den = wa * wa + wb * wb;
                if (den < 1e-6f) continue;
                const float lam = pen / den;
                if (k != root) { x[k] += nrm * (lam * wa); touched[k] = true; contact[k] = nrm; }
                x[k + 1] += nrm * (lam * wb);
                touched[k + 1] = true;
                contact[k + 1] = nrm;
            }
        for (int k = root; k < tail && !others.empty(); ++k)
            for (const Seg& o : others) {
                float u, w;
                closest(x[k], x[k + 1], o.a, o.b, u, w);
                const glm::vec3 p = glm::mix(x[k], x[k + 1], u), q = glm::mix(o.a, o.b, w);
                const float dist = glm::length(p - q), pen = 2.0f * r - dist;
                if (pen <= 0.0f || dist < 1e-5f) continue;
                const glm::vec3 nrm = (p - q) / dist;
                const float wa = k == root ? 0.0f : 1.0f - u, wb = u, den = wa * wa + wb * wb;
                if (den < 1e-6f) continue;
                const float lam = 0.5f * pen / den;     // half: the other tendril takes the other half when it solves
                if (k != root) x[k] += nrm * (lam * wa);
                x[k + 1] += nrm * (lam * wb);
            }
        for (int k = root + 1; k <= tail; ++k) {
            if (has_ground[k]) {
                const float d = glm::dot(x[k] - ground[k].p, ground[k].n) - r;
                if (d < 0.0f) { x[k] -= ground[k].n * d; touched[k] = true; contact[k] = ground[k].n; }
            }
            if (floor_level > -9000.0f && x[k].y < floor_level + r) {
                x[k].y = floor_level + r;
                touched[k] = true;
                contact[k] = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
        if (s.limit > 0.0f) {
            const float lim = glm::radians(std::min(s.limit, 180.0f)), cl = std::cos(lim), sl = std::sin(lim);
            for (int k = root + 1; k <= tail; ++k) {
                const glm::vec3 seg = x[k] - x[k - 1];
                const float len = glm::length(seg);
                if (len < 1e-5f) continue;
                const glm::vec3 d = safe_normalize(at_[k] - at_[k - 1], seg / len);
                if (glm::dot(seg / len, d) >= cl) continue;
                const glm::vec3 side = safe_normalize(seg - d * glm::dot(seg, d), glm::vec3(0.0f));
                if (side == glm::vec3(0.0f)) continue;
                x[k] = x[k - 1] + (d * cl + side * sl) * len;
            }
        }
    }

    // Lengths last, root to tail, each child moved onto its parent's sphere.
    void lengths(float h) {
        const float spring = rate_per(spring_rate, h);
        for (int k = root; k < tail; ++k) {
            const glm::vec3 d = x[k + 1] - x[k];
            const float len = glm::length(d);
            const glm::vec3 dir = len > 1e-5f ? d / len : safe_normalize(at_[k + 1] - at_[k], glm::vec3(0.0f, -1.0f, 0.0f));
            float want = L[k];
            if (stretch) {
                want = len > L[k] ? len + (L[k] - len) * spring : len;
                if (stretch_limit >= 0.0f) want = std::min(want, L[k] * stretch_limit);
            }
            x[k + 1] = x[k] + dir * want;
        }
    }

    void end_substep(float h) {
        for (int k = root + 1; k <= tail; ++k) {
            v[k] = (x[k] - old[k]) / h;
            if (touched[k]) {
                const float into = glm::dot(v[k], contact[k]);
                if (into < 0.0f) v[k] -= contact[k] * into;
                const glm::vec3 slide = v[k] - contact[k] * glm::dot(v[k], contact[k]);
                v[k] -= slide * fric;
            }
            const float speed = glm::length(v[k]);
            if (max_speed > 0.0f && speed > max_speed) v[k] *= max_speed / speed;
        }
    }

    // Step 6: the joints (the aim axis down the chain, the up axis nearest the FK's under the solved parent, blended with
    // the FK), the chain's points and speeds.
    void output() {
        for (int k = root; k <= tail; ++k) st->anim_prev[k] = a[k];
        const float blend = std::clamp(at<float>(chain, kBlend), 0.0f, 1.0f);
        for (int k = root; k <= tip; ++k) {
            Rows F = A[k];
            if (k > root && at<uint8_t>(jptr(k), kJParent) == index[k - 1]) F = fk(jptr(k), st->out[k - 1]);
            const glm::vec3 pos = x[k];         // the root: its animated place, pushed out of the shapes
            float len[3];
            glm::vec3 fr[3], sr[3];
            for (int i = 0; i < 3; ++i) { len[i] = glm::length(F.r[i]); fr[i] = safe_normalize(F.r[i], glm::vec3(i == 0, i == 1, i == 2)); }
            const int ia = axis % 3;
            const glm::vec3 aim = safe_normalize(x[k + 1] - pos, axis_of(F, axis)) * (axis >= 3 ? -1.0f : 1.0f);
            if (ia == up) {
                for (int i = 0; i < 3; ++i) sr[i] = fr[i];
            } else {
                sr[ia] = aim;
                sr[up] = safe_normalize(fr[up] - aim * glm::dot(fr[up], aim), fr[3 - ia - up]);
                const int ic = 3 - ia - up;
                sr[ic] = glm::cross(sr[(ic + 1) % 3], sr[(ic + 2) % 3]);
            }
            Rows o;
            quat_rows(glm::slerp(rows_quat(fr), rows_quat(sr), blend), o.r);
            for (int i = 0; i < 3; ++i) o.r[i] *= len[i];
            o.t = glm::mix(F.t, pos, blend);
            st->out[k] = o;
            st->out_valid[k] = true;
            o.store(&at<float>(chain, kMats + k * 0x40));
        }
        for (int k = root + 1; k <= tail; ++k) {
            if (k == tail) {
                float* m = &at<float>(chain, kMats + k * 0x40);
                m[12] = x[k].x; m[13] = x[k].y; m[14] = x[k].z;
            }
            float* vv = &at<float>(chain, kVels + k * 0x10);
            vv[0] = v[k].x; vv[1] = v[k].y; vv[2] = v[k].z;
        }
    }
};

// A distance constraint between two points of the set, softened to `k` (this pass's share), roots unmoved.
void link(glm::vec3& p, float wp, glm::vec3& q, float wq, float rest, float k) {
    if (k <= 0.0f || wp + wq <= 0.0f) return;
    const glm::vec3 d = q - p;
    const float len = glm::length(d);
    if (len < 1e-5f) return;
    const glm::vec3 c = d * ((len - rest) / len * k / (wp + wq));
    p += c * wp;
    q -= c * wq;
}

// Steps 5-6 for a set of tendrils: one, or a <cloth>'s sheet (its links between the pass's other constraints and the
// lengths). `T` is the time the solve covers; the tendrils are prepared.
void solve_set(std::vector<Tendril>& set, const Cloth* cloth, float T) {
    if (set.empty()) return;
    bool any_reset = false;
    int substeps = 1, iterations = 1;
    for (const Tendril& t : set) {
        any_reset |= t.reset;
        substeps = std::max(substeps, std::clamp(t.s.substeps, 1, 16));
        iterations = std::max(iterations, std::clamp(t.s.iterations, 1, 8));
    }
    if (T > 0.0f && !any_reset) {
        const int steps = std::clamp((int)std::ceil(T * substeps - 1e-4f), 1, 64);
        const float h = T / steps;
        for (Tendril& t : set) t.gather();
        // the sheet's links: neighbours point by point at the same depth (stretch), the diagonals (shear), the tendril
        // after next (bend); rest lengths from the animated pose, tails included, up to the shorter tendril
        struct Link { int ta, ka, tb, kb; float rest, rate; };
        std::vector<Link> links;
        if (cloth && set.size() > 1) {
            const int n = (int)set.size();
            auto add = [&](int ta, int da, int tb, int db, float rate) {
                if (rate <= 0.0f || da > set[ta].nodes() || db > set[tb].nodes() || (da == 0 && db == 0)) return;
                const int ka = set[ta].root + da, kb = set[tb].root + db;
                links.push_back({ ta, ka, tb, kb, glm::length(set[tb].a[kb] - set[ta].a[ka]), rate });
            };
            for (int i = 0; i < n; ++i) {
                const int j = i + 1 < n ? i + 1 : (cloth->closed && n > 2 ? 0 : -1);
                if (j >= 0) {
                    const int depth = std::min(set[i].nodes(), set[j].nodes());
                    for (int d = 1; d <= depth; ++d) add(i, d, j, d, cloth->stretch);
                    for (int d = 0; d < depth; ++d) { add(i, d, j, d + 1, cloth->shear); add(i, d + 1, j, d, cloth->shear); }
                }
                const int b = i + 2 < n ? i + 2 : (cloth->closed && n > 3 ? (i + 2) % n : -1);
                if (b >= 0) {
                    const int depth = std::min(set[i].nodes(), set[b].nodes());
                    for (int d = 1; d <= depth; ++d) add(i, d, b, d, cloth->bend);
                }
            }
        }
        for (int step = 1; step <= steps; ++step) {
            for (Tendril& t : set) t.begin_substep(h, (float)step / steps);
            for (int it = 0; it < iterations; ++it) {
                // the sheet's links first, so the members' shapes / ground then push out what they pulled in (after the
                // shapes they ended passes inside the body: albam-9d's preview, 0.53 cm against 0.09 alone)
                for (const Link& l : links) {
                    // a per-frame rate, as a share per substep, spread over the passes
                    const float k = 1.0f - std::pow(1.0f - rate_per(l.rate, h), 1.0f / iterations);
                    Tendril &ta = set[l.ta], &tb = set[l.tb];
                    link(ta.x[l.ka], ta.weight(l.ka), tb.x[l.kb], tb.weight(l.kb), l.rest, k);
                }
                for (Tendril& t : set) t.constrain(it == 0);
                for (Tendril& t : set) t.lengths(h);
            }
            for (Tendril& t : set) t.end_substep(h);
        }
    }
    for (Tendril& t : set) t.output();
}

// The sheet `chain`'s tendril at `slot` belongs to (copied out), false when it belongs to none.
bool cloth_of(uintptr_t chain, int slot, Cloth& out) {
    std::lock_guard<std::mutex> lock(g_cloth_lock);
    for (const Cloth& c : g_cloths)
        for (const ClothMember& m : c.members)
            if (m.chain == chain && m.slot == slot) { out = c; return true; }
    return false;
}
} // namespace

void on_move(uintptr_t chain) {
    float t = at<float>(chain, kStep);
    if (at<uint8_t>(chain, kFixedStep)) {           // players: one step per drawn frame in the game; the real frame here
        const uintptr_t main = *reinterpret_cast<uintptr_t*>(kDevil4Main);
        t = main ? std::min(at<float>(main, 0x68), 2.0f) : 1.0f;
    }
    State* st = state_of(chain);
    for (float& p : st->pending) p = std::min(p + std::max(t, 0.0f), kMaxPending);
}

void calc_joint(uintptr_t chain, uintptr_t joint, uintptr_t model, const Settings& s) {
    if (!at<uint8_t>(chain, kEnable) || !model) return;
    const int no = at<uint8_t>(joint, kJNo);
    int slot = -1;
    for (int k = 0; k < kSlots; ++k) if (bone(chain, k) == no) { slot = k; break; }
    if (slot < 0) return;
    State* st = state_of(chain);
    if (is_root(chain, slot)) {
        const auto t0 = std::chrono::high_resolution_clock::now();
        const uintptr_t joints = at<uintptr_t>(model, kpJoint);
        const int current = joints ? (int)((joint - joints) / kJointSize) : 0;
        Cloth cloth;
        if (cloth_of(chain, slot, cloth) && cloth.model == model) {
            // a sheet solves at its first root the world pass reaches (the lowest joint index); the others' calls only
            // write their results
            const uint8_t* table = at<const uint8_t*>(model, kJointTable);
            const uint32_t jnum = at<uint32_t>(model, kJointNum);
            int first = INT_MAX;
            for (const ClothMember& m : cloth.members) {
                const uint8_t i = table ? table[bone(m.chain, m.slot) & 0xFF] : 0xFF;
                if (i != 0xFF && i < jnum) first = std::min(first, (int)i);
            }
            if (first == current) {
                std::vector<Tendril> set(cloth.members.size());
                std::vector<Tendril> ok;
                ok.reserve(set.size());
                float T = std::min(st->pending[slot], kMaxPending);
                for (size_t i = 0; i < cloth.members.size(); ++i) {
                    const ClothMember& m = cloth.members[i];
                    if (!set[i].prepare(m.chain, model, m.slot, m.settings, current)) continue;
                    for (size_t j = 0; j < cloth.members.size(); ++j)   // its sheet neighbours on the same chain
                        if (cloth.members[j].chain == m.chain && j != i
                            && (j + 1 == i || i + 1 == j || (cloth.closed && (j + 1) % cloth.members.size() == i)
                                || (cloth.closed && (i + 1) % cloth.members.size() == j)))
                            set[i].neighbours.push_back(cloth.members[j].slot);
                    ok.push_back(set[i]);
                }
                for (const Tendril& t : ok) t.st->pending[t.root] = 0.0f;
                solve_set(ok, &cloth, T);
            }
        } else {
            std::vector<Tendril> one(1);
            if (one[0].prepare(chain, model, slot, s, current)) {
                const float T = std::min(st->pending[slot], kMaxPending);
                st->pending[slot] = 0.0f;
                solve_set(one, nullptr, T);
            }
        }
        g_last_ms = std::chrono::duration<float, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
    }
    if (st->out_valid[slot]) st->out[slot].store(&at<float>(joint, kJWmat));
}

void set_cloths(std::vector<Cloth> cloths) {
    std::lock_guard<std::mutex> lock(g_cloth_lock);
    g_cloths.swap(cloths);
}

void forget(uintptr_t chain) {
    {
        std::lock_guard<std::mutex> lock(g_cloth_lock);
        g_cloths.erase(std::remove_if(g_cloths.begin(), g_cloths.end(), [&](const Cloth& c) {
            return std::any_of(c.members.begin(), c.members.end(), [&](const ClothMember& m) { return m.chain == chain; });
        }), g_cloths.end());
    }
    std::lock_guard<std::mutex> lock(g_lock);
    g_states.erase(chain);
}

void forget_all() {
    {
        std::lock_guard<std::mutex> lock(g_cloth_lock);
        g_cloths.clear();
    }
    std::lock_guard<std::mutex> lock(g_lock);
    g_states.clear();
}

Stats stats() {
    std::lock_guard<std::mutex> lock(g_lock);
    return { (int)g_states.size(), g_last_ms };
}

} // namespace chain_solver
