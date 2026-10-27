#include "ModelAttach.hpp"
#include "Devil4.hpp"
#include <cstring>

namespace model_attach {

namespace {
// DX9 (model_attachments.md)
constexpr uintptr_t kDtiRModel        = 0x00EADF48;   // rModel::DTI
constexpr uintptr_t fn_ctor           = 0x005226F0;   // uDevil4Model::uDevil4Model(this@ecx), retn
constexpr uintptr_t fn_setModel       = 0x009E6D70;   // uModel::setModel(this@ecx, rModel*), retn 4: resets every joint's constraint
constexpr uintptr_t fn_move           = 0x00522CA0;   // uDevil4Model::move(this@ecx): motion, local, world pass
constexpr uintptr_t kVtbl             = 0x00BC8B38;   // uDevil4Model::vftable, 32 slots
constexpr uintptr_t kNullsub          = 0x00507330;
constexpr uintptr_t kCnsMatrixVtbl    = 0x00B9919C;
constexpr size_t    kModelSize        = 0xEA0;
// uModel / uCoord
constexpr uintptr_t kFlags = 0x04, kParent = 0x18, kParentNo = 0x1C, kPos = 0x30, kQuat = 0x40, kScale = 0x50;
constexpr uintptr_t kLightGroup = 0x130, kShadowCast = 0x134, kShadowRecv = 0x138, kPriorityBias = 0x294;
constexpr uintptr_t kJointNum = 0x2E0, kpJoint = 0x2E4, kJointTable = 0x2E8, kCameraTrans = 0xD08;
constexpr uintptr_t kJointSize = 0x90, kJNo = 0x07, kJCns = 0x0C, kJWmat = 0x50;
constexpr uintptr_t kRModelJointNum = 0x64;
constexpr uintptr_t kActorTrans = 0x12DC;             // uActor::mbActorTrans: the players draw only while it is 1
constexpr uint32_t  ATTR_DRAW = 0x800, ATTR_VISIBLE = 0x4000;
// our block after the model: [0xEA0, 0xEC0) the fields below, then one cCnsMatrix (0x70) per joint
constexpr uintptr_t kOwner = 0xEA0, kMode = 0xEA4, kCount = 0xEA8, kTag = 0xEAC, kActor = 0xEB0, kCns = 0xEC0, kCnsSize = 0x70;
constexpr uint32_t  kTagValue = 0x5458454D;           // "MEXT"

template <class T> T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

uintptr_t g_vtbl[32];
bool g_vtbl_ready = false;

void detach_joints(uintptr_t self) {                    // our constraints off the model's joints (others' left alone)
    const uint32_t n = at<uint32_t>(self, kJointNum);
    const uintptr_t joints = at<uintptr_t>(self, kpJoint);
    if (!joints) return;
    const uintptr_t lo = self + kCns, hi = lo + at<uint32_t>(self, kCount) * kCnsSize;
    for (uint32_t i = 0; i < n; ++i) {
        uintptr_t& c = at<uintptr_t>(joints + i * kJointSize, kJCns);
        if (c >= lo && c < hi) c = 0;
    }
}

bool derives_from_actor(uintptr_t unit) {
    static uintptr_t s_yes[16] = {}, s_no[16] = {};
    MtDTI* dti = devil4_sdk::get_DTI((void*)unit);
    for (uintptr_t d : s_yes) if (d == (uintptr_t)dti) return true;
    for (uintptr_t d : s_no)  if (d == (uintptr_t)dti) return false;
    bool yes = false;
    for (uintptr_t d = (uintptr_t)dti; d && !yes; d = at<uintptr_t>(d, 0x10)) {   // MtDTI: +4 name, +0x10 parent
        const char* name = at<const char*>(d, 0x04);
        if (name && std::strcmp(name, "uActor") == 0) yes = true;
    }
    uintptr_t* table = yes ? s_yes : s_no;
    for (int i = 0; i < 16; ++i) if (!table[i]) { table[i] = (uintptr_t)dti; break; }
    return yes;
}

// moveAfter (vtbl[8]): owner check, visibility, the skin constraints, then the model's own update.
void __fastcall move_after(uintptr_t self, uintptr_t) {
    const uintptr_t owner = at<uintptr_t>(self, kOwner);
    const uint32_t ostate = owner ? (at<uint32_t>(owner, kFlags) & 7) : 0;
    if (!owner || (ostate != 1 && ostate != 2)) {       // the owner is dying (its memory lasts through states 3 / 4)
        despawn(self);
        return;
    }
    // visibility follows the owner: drawn and visible, and for an actor its "draw me" flag
    const uint32_t of = at<uint32_t>(owner, kFlags);
    bool visible = (of & (ATTR_DRAW | ATTR_VISIBLE)) == (ATTR_DRAW | ATTR_VISIBLE);
    if (visible && at<uint32_t>(self, kActor)) visible = at<uint8_t>(owner, kActorTrans) == 1;
    uint32_t& sf = at<uint32_t>(self, kFlags);
    sf = visible ? (sf | ATTR_VISIBLE) : (sf & ~ATTR_VISIBLE);
    if (at<int>(self, kMode) == SKIN) {
        const uint32_t n = at<uint32_t>(self, kJointNum), on = at<uint32_t>(owner, kJointNum);
        const uintptr_t joints = at<uintptr_t>(self, kpJoint), ojoints = at<uintptr_t>(owner, kpJoint);
        const uint8_t* otable = at<const uint8_t*>(owner, kJointTable);
        const uintptr_t lo = self + kCns, hi = lo + at<uint32_t>(self, kCount) * kCnsSize;
        if (joints && ojoints && otable) {
            for (uint32_t i = 0; i < n && i < at<uint32_t>(self, kCount); ++i) {
                const uintptr_t jp = joints + i * kJointSize;
                uintptr_t& c = at<uintptr_t>(jp, kJCns);
                if (c && (c < lo || c >= hi)) continue;            // a chain (or another constraint) holds it
                const uint8_t oi = otable[at<uint8_t>(jp, kJNo)];
                const uintptr_t cns = lo + i * kCnsSize;
                if (oi == 0xFF || oi >= on) { if (c == cns) c = 0; continue; }   // the owner lacks it: plain FK
                at<uintptr_t>(cns, 0x60) = ojoints + oi * kJointSize + kJWmat;     // re-bound each frame: an owner setModel
                c = cns;                                                          // frees its joints, a chain dtor clears ours
            }
        }
    }
    ((void(__fastcall*)(uintptr_t, int))fn_move)(self, 0);
}

void init_cns(uintptr_t c) {
    std::memset((void*)c, 0, kCnsSize);
    at<uintptr_t>(c, 0x00) = kCnsMatrixVtbl;
    at<float>(c, 0x04) = 1.0f;        // blend weight
    at<float>(c, 0x0C) = 0.02f;       // blend velocity
    at<float>(c, 0x10) = 1.0f;        // blend
    at<float>(c, 0x14) = 1.0f;        // rate
    at<uint8_t>(c, 0x19) = 1;         // mEnable
    float* m = &at<float>(c, 0x20);   // mOfsMat = identity: the source matrix verbatim
    m[0] = m[5] = m[10] = m[15] = 1.0f;
    // +0x60 mpMat (bound per frame), +0x64 mMode 0 = the whole matrix
}
} // namespace

uintptr_t spawn(uintptr_t owner, const char* mod_path, Mode mode, int joint, const Offset* offset, int line) {
    if (!owner || !mod_path || !*mod_path || line < 0 || line > 31) return 0;
    const uintptr_t r = (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)kDtiRModel, (char*)mod_path, 1);
    if (!r) return 0;
    const uint32_t n = at<uint32_t>(r, kRModelJointNum);
    const size_t size = kCns + (size_t)n * kCnsSize;
    const uintptr_t self = (uintptr_t)devil4_sdk::mt_allocate_heap(size, 16);
    if (!self) { devil4_sdk::release_resource((cResource*)r); return 0; }
    std::memset((void*)self, 0, size);
    ((void(__fastcall*)(uintptr_t, int))fn_ctor)(self, 0);
    ((void(__fastcall*)(uintptr_t, int, uintptr_t))fn_setModel)(self, 0, r);   // takes its own reference
    devil4_sdk::release_resource((cResource*)r);
    if (!g_vtbl_ready) {                       // no move-pass update; the moveAfter pass keeps it attached and updates it
        std::memcpy(g_vtbl, (const void*)kVtbl, sizeof(g_vtbl));
        g_vtbl[6] = kNullsub;
        g_vtbl[8] = (uintptr_t)&move_after;
        g_vtbl_ready = true;
    }
    at<uintptr_t>(self, 0) = (uintptr_t)g_vtbl;
    at<uintptr_t>(self, kOwner) = owner;
    at<int>(self, kMode) = mode;
    at<uint32_t>(self, kCount) = n;
    at<uint32_t>(self, kTag) = kTagValue;
    at<uint32_t>(self, kActor) = derives_from_actor(owner) ? 1 : 0;
    // the owner's look: light / shadow groups, the players' parts' draw priority, no camera fade
    at<uint32_t>(self, kLightGroup)  = at<uint32_t>(owner, kLightGroup);
    at<uint32_t>(self, kShadowCast)  = at<uint32_t>(owner, kShadowCast);
    at<uint32_t>(self, kShadowRecv)  = at<uint32_t>(owner, kShadowRecv);
    at<int>(self, kPriorityBias)     = -100;
    at<uint8_t>(self, kCameraTrans)  = 0;
    at<uintptr_t>(self, kParent) = owner;
    if (mode == RIGID) {
        const Offset o = offset ? *offset : Offset{};
        at<int>(self, kParentNo) = joint;
        float* p = &at<float>(self, kPos);   p[0] = o.pos[0];   p[1] = o.pos[1];   p[2] = o.pos[2];
        float* q = &at<float>(self, kQuat);  q[0] = o.quat[0];  q[1] = o.quat[1];  q[2] = o.quat[2];  q[3] = o.quat[3];
        float* s = &at<float>(self, kScale); s[0] = o.scale[0]; s[1] = o.scale[1]; s[2] = o.scale[2];
    } else {
        at<int>(self, kParentNo) = -1;       // the root (and culling) follow the owner, as the coat's
        for (uint32_t i = 0; i < n; ++i) init_cns(self + kCns + i * kCnsSize);
    }
    devil4_sdk::sUnit_spawn((void*)self, line);
    return self;
}

void despawn(uintptr_t model) {
    if (!is_attachment(model)) return;
    detach_joints(model);
    at<uintptr_t>(model, kOwner) = 0;
    at<uintptr_t>(model, kParent) = 0;
    uint32_t& f = at<uint32_t>(model, kFlags);
    if ((f & 7) == 1 || (f & 7) == 2) f = (f & ~7u) | 3;   // dying: sUnit runs the deleting destructor (it frees the block)
}

bool is_attachment(uintptr_t model) { return model && at<uintptr_t>(model, 0) == (uintptr_t)g_vtbl && at<uint32_t>(model, kTag) == kTagValue; }
uintptr_t owner_of(uintptr_t model) { return is_attachment(model) ? at<uintptr_t>(model, kOwner) : 0; }

} // namespace model_attach
