#include "ModelCns.hpp"
#include "ModelAttach.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>

namespace model_cns {

namespace {
// DX9 (model_constraints.md)
constexpr uintptr_t kVtblMatrix = 0x00B9919C, kVtblOffset = 0x00B99214, kVtblDirPos = 0x00B990B8;
constexpr uintptr_t fn_matrix_adjust = 0x00443430, fn_dirpos_adjust = 0x00439910;   // thiscall(joint, model), retn 8
constexpr int kVtblSlots = 18, kSlotAdjust = 5;
// uModel / Joint
constexpr uintptr_t kJointNum = 0x2E0, kpJoint = 0x2E4, kJointTable = 0x2E8, kJointSize = 0x90, kJCns = 0x0C, kJWmat = 0x50;
// cConstraint
constexpr uintptr_t kBlendWeight = 0x04, kBlendVel = 0x0C, kBlend = 0x10, kRate = 0x14, kEnable = 0x19;
// cCnsMatrix (0x70), our blend after it
constexpr uintptr_t kOfsMat = 0x20, kMat = 0x60, kMode = 0x64, kPinBlend = 0x70;
// cCnsOffset (0x60)
constexpr uintptr_t kOffRot = 0x20, kOffScale = 0x30, kOffTrans = 0x40, kOffAdd = 0x50;
// cCnsDirPos (0x80), our source after it
constexpr uintptr_t kPosEnable = 0x20, kDirEnable = 0x21, kUpEnable = 0x22, kBoneDir = 0x24, kBoneUp = 0x28, kLimitEnable = 0x2C;
constexpr uintptr_t kDir = 0x40, kUp = 0x50, kLimitMin = 0x60, kLimitMax = 0x70, kAimSource = 0x80;
constexpr size_t kBlockSize = 0x90;            // the largest of the three with its extra field
constexpr float kDegToRad = 3.14159265f / 180.0f;

template <class T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

uintptr_t g_pin_vtbl[kVtblSlots], g_aim_vtbl[kVtblSlots];
std::vector<uintptr_t> g_free;
std::mutex g_lock;

// a and b are model matrices (rows x, y, z, position; row 3 w = 1): rotation by slerp, row lengths and position linearly
void blend_matrix(float* out, const float* a, const float* b, float t) {
    glm::vec3 ra[3], rb[3];
    float la[3], lb[3];
    for (int i = 0; i < 3; ++i) {
        ra[i] = glm::vec3(a[i * 4], a[i * 4 + 1], a[i * 4 + 2]);
        rb[i] = glm::vec3(b[i * 4], b[i * 4 + 1], b[i * 4 + 2]);
        la[i] = glm::length(ra[i]);
        lb[i] = glm::length(rb[i]);
        if (la[i] < 1e-6f || lb[i] < 1e-6f) return;     // a collapsed matrix has no rotation: leave b
        ra[i] /= la[i];
        rb[i] /= lb[i];
    }
    const glm::quat q = glm::normalize(glm::slerp(glm::quat_cast(glm::mat3(ra[0], ra[1], ra[2])),
                                                  glm::quat_cast(glm::mat3(rb[0], rb[1], rb[2])), t));
    const glm::mat3 r = glm::mat3_cast(q);
    for (int i = 0; i < 3; ++i) {
        const float len = la[i] + (lb[i] - la[i]) * t;
        out[i * 4] = r[i].x * len;
        out[i * 4 + 1] = r[i].y * len;
        out[i * 4 + 2] = r[i].z * len;
        out[i * 4 + 3] = 0.0f;
    }
    for (int i = 0; i < 3; ++i) out[12 + i] = a[12 + i] + (b[12 + i] - a[12 + i]) * t;
    out[15] = 1.0f;
}

// __fastcall with a dummy edx = __thiscall, and the callee pops the same 8 bytes
void __fastcall pin_adjust(uintptr_t self, int, uintptr_t joint, uintptr_t model) {
    const auto stock = (void(__fastcall*)(uintptr_t, int, uintptr_t, uintptr_t))fn_matrix_adjust;
    const float blend = at<float>(self, kPinBlend);
    if (blend >= 1.0f) { stock(self, 0, joint, model); return; }
    if (blend <= 0.0f) return;
    float fk[16], out[16];
    float* m = &at<float>(joint, kJWmat);
    std::memcpy(fk, m, sizeof(fk));
    stock(self, 0, joint, model);
    std::memcpy(out, m, sizeof(out));
    blend_matrix(out, fk, m, blend);
    std::memcpy(m, out, sizeof(out));
}

void __fastcall aim_adjust(uintptr_t self, int, uintptr_t joint, uintptr_t model) {
    const float* src = at<const float*>(self, kAimSource);
    if (!src) return;
    const float* m = &at<float>(joint, kJWmat);
    float* dir = &at<float>(self, kDir);
    dir[0] = src[12]; dir[1] = src[13]; dir[2] = src[14];
    const float dx = dir[0] - m[12], dy = dir[1] - m[13], dz = dir[2] - m[14];
    if (dx * dx + dy * dy + dz * dz < 1e-6f) return;     // on top of its target: no direction, the orientation would be NaN
    ((void(__fastcall*)(uintptr_t, int, uintptr_t, uintptr_t))fn_dirpos_adjust)(self, 0, joint, model);
}

void init_vtables() {
    static bool ready = false;
    if (ready) return;
    std::memcpy(g_pin_vtbl, (const void*)kVtblMatrix, sizeof(g_pin_vtbl));
    std::memcpy(g_aim_vtbl, (const void*)kVtblDirPos, sizeof(g_aim_vtbl));
    g_pin_vtbl[kSlotAdjust] = (uintptr_t)&pin_adjust;
    g_aim_vtbl[kSlotAdjust] = (uintptr_t)&aim_adjust;
    ready = true;
}

uintptr_t new_block(uintptr_t vtable) {            // the cConstraint defaults every game constructor writes
    init_vtables();
    uintptr_t b = 0;
    {
        std::lock_guard<std::mutex> lock(g_lock);
        if (!g_free.empty()) { b = g_free.back(); g_free.pop_back(); }
    }
    if (!b) b = (uintptr_t)_aligned_malloc(kBlockSize, 16);
    if (!b) return 0;
    std::memset((void*)b, 0, kBlockSize);
    at<uintptr_t>(b, 0) = vtable;
    at<float>(b, kBlendWeight) = 1.0f;
    at<float>(b, kBlendVel) = 0.02f;
    at<float>(b, kBlend) = 1.0f;
    at<float>(b, kRate) = 1.0f;
    at<uint8_t>(b, kEnable) = 1;
    return b;
}

// S . R . T as the game's sub_8B92D0 builds it (row vectors): rot in degrees about X, then Y, then Z
void offset_matrix(float* m, const float* pos, const float* rot, const float* scale) {
    const float sx = std::sin(rot[0] * kDegToRad), cx = std::cos(rot[0] * kDegToRad);
    const float sy = std::sin(rot[1] * kDegToRad), cy = std::cos(rot[1] * kDegToRad);
    const float sz = std::sin(rot[2] * kDegToRad), cz = std::cos(rot[2] * kDegToRad);
    const float r[3][3] = {
        { cz * cy, sz * cy, -sy },
        { cz * sy * sx - sz * cx, sz * sy * sx + cz * cx, cy * sx },
        { cz * sy * cx + sz * sx, sz * sy * cx - cz * sx, cy * cx },
    };
    for (int i = 0; i < 3; ++i) {
        for (int k = 0; k < 3; ++k) m[i * 4 + k] = r[i][k] * scale[i];
        m[i * 4 + 3] = 0.0f;
    }
    m[12] = pos[0]; m[13] = pos[1]; m[14] = pos[2]; m[15] = 1.0f;
}
} // namespace

uintptr_t make_pin(const Pin& p) {
    const uintptr_t b = new_block((init_vtables(), (uintptr_t)g_pin_vtbl));
    if (!b) return 0;
    offset_matrix(&at<float>(b, kOfsMat), p.pos, p.rot, p.scale);
    at<uint32_t>(b, kMode) = p.rotation_only ? 1 : 0;
    at<float>(b, kPinBlend) = p.blend;
    return b;
}

uintptr_t make_offset(const Offset& o) {
    const uintptr_t b = new_block(kVtblOffset);
    if (!b) return 0;
    float* r = &at<float>(b, kOffRot);   r[0] = o.rot[0];   r[1] = o.rot[1];   r[2] = o.rot[2];       // degrees
    float* s = &at<float>(b, kOffScale); s[0] = o.scale[0]; s[1] = o.scale[1]; s[2] = o.scale[2];
    float* t = &at<float>(b, kOffTrans); t[0] = o.pos[0];   t[1] = o.pos[1];   t[2] = o.pos[2];
    at<uint8_t>(b, kOffAdd) = o.add_motion ? 1 : 0;
    return b;
}

uintptr_t make_aim(const Aim& a) {
    const uintptr_t b = new_block((init_vtables(), (uintptr_t)g_aim_vtbl));
    if (!b) return 0;
    at<float>(b, kBlendWeight) = a.blend;
    at<float>(b, kBlend) = a.blend;
    at<uint8_t>(b, kPosEnable) = 0;
    at<uint8_t>(b, kDirEnable) = 1;
    at<uint8_t>(b, kUpEnable) = 0;               // the up axis stays as FK left it
    at<uint32_t>(b, kBoneDir) = a.axis;
    at<uint32_t>(b, kBoneUp) = a.up;
    at<float>(b, kUp + 4) = 1.0f;
    at<uint8_t>(b, kLimitEnable) = a.limited ? 1 : 0;
    if (a.limited)
        for (int i = 0; i < 3; ++i) {            // radians in the class
            (&at<float>(b, kLimitMin))[i] = -a.limit[i] * kDegToRad;
            (&at<float>(b, kLimitMax))[i] = a.limit[i] * kDegToRad;
        }
    return b;
}

void release(uintptr_t block) {
    if (!block) return;
    at<uint8_t>(block, kEnable) = 0;
    std::lock_guard<std::mutex> lock(g_lock);
    g_free.push_back(block);
}

void bind(uintptr_t block, const float* source) {
    if (at<uintptr_t>(block, 0) == (uintptr_t)g_pin_vtbl) at<const float*>(block, kMat) = source;
    else if (at<uintptr_t>(block, 0) == (uintptr_t)g_aim_vtbl) at<const float*>(block, kAimSource) = source;
}

namespace {
uintptr_t joint_of(uintptr_t model, int joint_no) {
    if (!model || joint_no < 0 || joint_no > 255) return 0;
    const uintptr_t joints = at<uintptr_t>(model, kpJoint);
    const uint8_t* table = at<const uint8_t*>(model, kJointTable);
    if (!joints || !table) return 0;
    const uint8_t i = table[joint_no];
    if (i == 0xFF || i >= at<uint32_t>(model, kJointNum)) return 0;
    return joints + i * kJointSize;
}
} // namespace

Install install(uintptr_t block, uintptr_t model, int joint_no) {
    const uintptr_t j = joint_of(model, joint_no);
    if (!j) return NO_JOINT;
    uintptr_t& c = at<uintptr_t>(j, kJCns);
    if (c && c != block && !model_attach::is_skin_constraint(model, c)) return TAKEN;   // a skin joint gives way: its move_after
    c = block;                                                                         // skips a joint someone else holds
    return INSTALLED;
}

void remove(uintptr_t block, uintptr_t model, int joint_no) {
    const uintptr_t j = joint_of(model, joint_no);
    if (j && at<uintptr_t>(j, kJCns) == block) at<uintptr_t>(j, kJCns) = 0;
}

const float* joint_matrix(uintptr_t model, int joint_no) {
    const uintptr_t j = joint_of(model, joint_no);
    return j ? &at<float>(j, kJWmat) : nullptr;
}

} // namespace model_cns
