#include "Collision.hpp"

namespace collision {

namespace {
uintptr_t fn_findIntersection  = 0x0095B320;   // sDevil4Collision::findIntersection(query@eax, inst, seg*, bothSides, hitOut*)
uintptr_t fn_getNormalToTri    = 0x0048F1A0;   // sMath::getNormalToTri(result@eax, out@edi) -> out
uintptr_t fn_lineAny           = 0x0094B660;   // (filter@eax, inst@edi; seg*, typeMask) -> al, retn 8
uintptr_t fn_sphereContact     = 0x0094B480;   // (inst@ecx, filter@eax; sphere*, outPos*, outPlane*, typeMask) -> eax, retn 10h
uintptr_t fn_sphereOverlap     = 0x009564E0;   // (Param*@esi; inst, sphere*) -> al, retn 8
uintptr_t fn_sphereSlide       = 0x00956750;   // (Param*@eax; inst, pos*, old*, sphere*) -> eax, retn 10h
uintptr_t fn_capsuleSlideUtil  = 0x0045F650;   // cUtil::checkScrCollision(end@eax, start@ecx, out@edi; capsule*, filter) -> eax, caller cleans 8
uintptr_t fn_capsuleSlide      = 0x00956C90;   // (Param*@eax; inst, pos*, old*, capsule*) -> eax, retn 10h
uintptr_t fn_capsuleOverlap    = 0x0094F230;   // (capsule*@esi; inst, typeMask, groupLo, groupHi, callback*) -> al, retn 14h
uintptr_t fn_capsulePushOut    = 0x0094EF30;   // __fastcall(ref@ecx, inst@edx; capsule*, ContactDefine*, filter, bothSides) -> eax, retn 10h
uintptr_t fn_queryAllSbcs      = 0x0094B7C0;   // thiscall(inst@ecx; aabb*, callback*, ContactDefine*) -> eax, retn 0Ch
uintptr_t fn_getTriangle       = 0x00948660;   // (out float[12]@eax, SbcInfo*@ecx), retn
uintptr_t sDevil4Collision_ptr = 0x00E559D0;   // sDevil4Collision::mpInstance
constexpr uintptr_t kNarrowCapsule = 0x0094B7E0, kBoundsCapsule = 0x0094B830, kStoreCapsule = 0x0094C4E0;   // capsule_overlap's triple

// sCollision::ContactDefine (0x14): which .sbc kinds and groups a walk visits.
struct ContactDefine { uint32_t mode, type, group_lo, group_hi; int parts_filter; };
// sCollision::callback_struct_t (0x58), what ScrCollisionBeforeFunc fills: an identity matrix, then the narrow phase
// (int __cdecl(SbcInfo*, ctx), per triangle), the bounds callback (moving parts; may be 0), the store (may be 0), the
// context, the channel filter and a flags byte.
struct CallbackStruct {
    float     matrix[16];
    uintptr_t narrow, bounds, store;
    void*     ctx;
    uint32_t  filter;
    uint8_t   flags, pad[3];
};
static_assert(sizeof(ContactDefine) == 0x14 && sizeof(CallbackStruct) == 0x58, "collision callback layouts");
CallbackStruct make_callbacks(uintptr_t narrow, uintptr_t bounds, uintptr_t store, void* ctx, uint32_t filter) {
    CallbackStruct c{};
    c.matrix[0] = c.matrix[5] = c.matrix[10] = c.matrix[15] = 1.0f;
    c.narrow = narrow; c.bounds = bounds; c.store = store; c.ctx = ctx; c.filter = filter;
    return c;
}
} // namespace

CollParam line_param(uint32_t filter) {
    CollParam p{};
    p.type             = 127;
    p.filter           = (int)filter;
    p.handle_exclude_0 = -1;
    p.handle_exclude_1 = -1;
    p.correctionNum    = 10;
    p.bAdjMvRelSet     = 1;
    p.bAdjAntiStop     = 1;
    p.capsuleFwdLimit  = 3.4028235e38f;
    p.capsuleBwdLimit  = 3.4028235e38f;
    return p;
}

// __usercall: the registers and the stack pointer are saved around the call, whoever cleans the arguments.
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

MtVector3 tri_normal(const LineResult* hit) {
    MtVector3 out{};
    MtVector3* p = &out;
    _asm {
        pushad
        mov eax, hit
        mov edi, p
        call fn_getNormalToTri
        popad
    }
    return out;
}

// The wrappers below follow find_intersection: the stack pointer saved in ebx and every register around the call, the
// stack arguments pushed last-first, then the register arguments loaded, the result stored before popad.

bool line_hits(const MtLineSegment& segment, uint32_t filter, uint32_t type_mask) {
    const MtLineSegment* segm = &segment;
    bool hit = false;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push type_mask
        push segm
        mov eax, filter
        mov edi, sDevil4Collision_ptr
        mov edi, [edi]
        call fn_lineAny
        mov byte ptr hit, al
        popad
        mov esp, ebx
        pop ebx
    }
    return hit;
}

int sphere_contact(const MtSphere& world, uint32_t filter, MtVector3* out_pos, float* out_plane, uint32_t type_mask) {
    const MtSphere* sph = &world;
    int result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push type_mask
        push out_plane
        push out_pos
        push sph
        mov eax, filter
        mov ecx, sDevil4Collision_ptr
        mov ecx, [ecx]
        call fn_sphereContact
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result;
}

bool sphere_overlap(CollParam* param, const MtSphere& world) {
    const MtSphere* sph = &world;
    bool hit = false;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push sph
        mov eax, sDevil4Collision_ptr
        push dword ptr [eax]
        mov esi, param
        call fn_sphereOverlap
        mov byte ptr hit, al
        popad
        mov esp, ebx
        pop ebx
    }
    return hit;
}

int sphere_slide(CollParam* param, MtVector3* pos, const MtVector3& old, const MtSphere& offset) {
    const MtVector3* op = &old;
    const MtSphere* sph = &offset;
    int result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push sph
        push op
        push pos
        mov eax, sDevil4Collision_ptr
        push dword ptr [eax]
        mov eax, param
        call fn_sphereSlide
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result;
}

uint32_t capsule_slide(const MtCapsule& offset, const MtVector3& start, const MtVector3& end, MtVector3* out, uint32_t filter) {
    const MtCapsule* cp = &offset;
    const MtVector3* start_p = &start;   // (not `st` / `endp`: an FPU register and a MASM keyword in _asm)
    const MtVector3* end_p = &end;
    uint32_t result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push filter
        push cp
        mov eax, end_p
        mov ecx, start_p
        mov edi, out
        call fn_capsuleSlideUtil
        add esp, 8                  // the caller cleans its arguments
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result;
}

int capsule_slide(CollParam* param, MtVector3* pos, const MtVector3& old, const MtCapsule& offset) {
    const MtVector3* op = &old;
    const MtCapsule* cp = &offset;
    int result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push cp
        push op
        push pos
        mov eax, sDevil4Collision_ptr
        push dword ptr [eax]
        mov eax, param
        call fn_capsuleSlide
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result;
}

bool capsule_overlap(const MtCapsule& world, uint32_t filter, uint32_t type_mask) {
    const MtCapsule* cp = &world;
    CallbackStruct cbs = make_callbacks(kNarrowCapsule, kBoundsCapsule, kStoreCapsule, nullptr, filter);   // ctx: the function sets it
    CallbackStruct* cb = &cbs;
    bool hit = false;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push cb
        push 0xFFFFFFFF             // groups: all
        push 0xFFFFFFFF
        push type_mask
        mov eax, sDevil4Collision_ptr
        push dword ptr [eax]
        mov esi, cp
        call fn_capsuleOverlap      // ~0x2E18 bytes of stack
        mov byte ptr hit, al
        popad
        mov esp, ebx
        pop ebx
    }
    return hit;
}

bool capsule_push_out(MtCapsule* cap, const MtCapsule& ref, uint32_t filter, bool both_sides, uint32_t type_mask) {
    ContactDefine def{ 2, type_mask, 0xFFFFFFFFu, 0xFFFFFFFFu, 0 };   // mode 2, as the camera's call
    ContactDefine* dp = &def;
    const MtCapsule* rp = &ref;
    const int sides = both_sides ? 1 : 0;
    int result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push sides
        push filter
        push dp
        push cap
        mov ecx, rp
        mov edx, sDevil4Collision_ptr
        mov edx, [edx]
        call fn_capsulePushOut
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result != 0;
}

int triangles_in_aabb(const MtAABB& box, TriCallback cb, void* ctx, uint32_t filter, uint32_t type_mask) {
    const MtAABB* boxp = &box;
    CallbackStruct cbs = make_callbacks((uintptr_t)cb, 0, 0, ctx, filter);
    CallbackStruct* cbp = &cbs;
    ContactDefine def{ 0, type_mask, 0xFFFFFFFFu, 0xFFFFFFFFu, 0 };
    ContactDefine* dp = &def;
    int result = 0;
    _asm {
        push ebx
        mov ebx, esp
        pushad
        push dp
        push cbp
        push boxp
        mov ecx, sDevil4Collision_ptr
        mov ecx, [ecx]
        call fn_queryAllSbcs
        mov result, eax
        popad
        mov esp, ebx
        pop ebx
    }
    return result;
}

void get_triangle(const SbcInfo* tri, float out[12]) {
    _asm {
        pushad
        mov eax, out
        mov ecx, tri
        call fn_getTriangle
        popad
    }
}

} // namespace collision
