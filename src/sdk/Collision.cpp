#include "Collision.hpp"

namespace collision {

namespace {
uintptr_t fn_findIntersection  = 0x0095B320;   // sDevil4Collision::findIntersection(query@eax, inst, seg*, bothSides, hitOut*)
uintptr_t fn_getNormalToTri    = 0x0048F1A0;   // sMath::getNormalToTri(result@eax, out@edi) -> out
uintptr_t sDevil4Collision_ptr = 0x00E559D0;   // sDevil4Collision::mpInstance
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

} // namespace collision
