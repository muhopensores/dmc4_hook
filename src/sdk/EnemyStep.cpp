#include "EnemyStep.hpp"
#include <vector>

#define naked __declspec(naked)

namespace enemy_step {

bool hook_installed = false;
uintptr_t jmp_ret = 0;

namespace {

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

constexpr uintptr_t kSUnit      = 0xE552CC;   // sUnit::mpInstance (the instruction the hook replaces reads it)
constexpr uintptr_t kHit        = 0x804A7C;   // checkEmJumpBegin's hit path, esi = the target
constexpr uintptr_t kUnitFlags  = 0x4;        // cUnit: bits 0-2 the state (2 = moving), 0x400
constexpr uintptr_t kActorStat  = 0xEA8;      // uActor: bit 7 = no step
constexpr uintptr_t kPos        = 0x30;
constexpr uintptr_t kJoints     = 0x2E4;      // uModel::mpJoint (0x90 bytes each, world matrix at +0x50, its row 3 at +0x80)
constexpr uintptr_t kJointTable = 0x2E8;      // uModel::mJointTable: joint number -> index, 0xFF = none

std::vector<TargetFn>& sources() { static std::vector<TargetFn> v; return v; }

uintptr_t sunit_ptr = kSUnit;
uintptr_t hit_addr  = kHit;

} // namespace

bool add_target_source(TargetFn fn) {
    if (!fn) return false;
    for (TargetFn f : sources()) if (f == fn) return true;
    sources().push_back(fn);
    return true;
}

bool in_reach(uintptr_t jumper, uintptr_t target, int joint, float radius) {
    if (!jumper || !target || jumper == target || joint < 0 || joint > 0xFF) return false;
    if (at<uint8_t>(target, kActorStat) & 0x80) return false;
    const uint32_t flags = at<uint32_t>(target, kUnitFlags);
    if ((flags & 7) != 2 || !(flags & 0x400)) return false;
    const uintptr_t table = at<uintptr_t>(target, kJointTable), joints = at<uintptr_t>(target, kJoints);
    if (!table || !joints) return false;
    const uint8_t index = at<uint8_t>(table, (uintptr_t)joint);
    if (index == 0xFF) return false;
    const float* m = reinterpret_cast<const float*>(joints + 0x90 * index + 0x50);
    const float w = m[15] != 0.0f ? m[15] : 1.0f;
    const float dx = m[12] / w - at<float>(jumper, kPos);
    const float dy = m[13] / w - (at<float>(jumper, kPos + 4) + 85.0f);
    const float dz = m[14] / w - at<float>(jumper, kPos + 8);
    return dx * dx + dy * dy + dz * dz < radius * radius;
}

uintptr_t __stdcall find_target(uintptr_t jumper) {
    if (!jumper) return 0;
    for (TargetFn f : sources()) {
        if (const uintptr_t t = f(jumper)) return t;
    }
    return 0;
}

naked void detour() {
    __asm {
        pushad
        push edi
        call find_target          // __stdcall: pops its argument
        mov [esp+4], eax          // the saved esi (pushad: edi at [esp], esi at [esp+4])
        popad
        test esi, esi
        jz enemies
        jmp dword ptr [hit_addr]
    enemies:
        mov eax, [sunit_ptr]
        mov eax, [eax]
        jmp dword ptr [jmp_ret]
    }
}

} // namespace enemy_step
