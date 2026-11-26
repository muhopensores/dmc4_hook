#pragma once
// MtRandom - the game's MT19937 PRNG and the block that holds its state (RE/MtRandom_findings.md,
// IDB DevilMayCry4_DX9.exe.i64). The twist and the tempering are the canonical ones; the seeding is
// not. Per word i, with s the running LCG value:
//      mt[i] = s & 0xFFFF0000;  s = 69069 * s + 1;  mt[i] |= s >> 16;  s = 69069 * s + 1;
// so a stock MT19937 seeded with the same number does NOT reproduce the game's stream, while loading
// these 624 words into one does.
//
// Gameplay draws from ONE global block. 256 functions draw from it and 60 of those draws are
// compiler-inlined copies of next_table that address kMtRandom absolutely, so this state cannot be
// relocated or made per-player, and hooking next_table misses draws. Nothing in the exe seeds from
// time, PID, tick count or any session value: every seed in the binary is a compile-time constant.
// What has to be controlled in lockstep is the draw ORDER and COUNT (units run as jobs on up to 8
// threads, and sound, effects, camera and title draw from the same stream as enemy AI). This is a
// view of the game's own bytes: reseed in place, never copy the state somewhere else.

#include <cstdint>
#include <cstddef>

constexpr uint32_t kMtRandomWords = 624;            // MT19937 N
constexpr uintptr_t kMtRandom = 0x00EAC6B0;         // IDB: MtRandom::global_rng
constexpr uintptr_t kMtRandomMti = 0x00EAD070;      // kMtRandom + 0x9C0, next_table's [edx+9C0h]
constexpr uintptr_t kMtRandomMag01 = 0x00E16A98;    // IDB: MtRandom::mag01 = {0, 0x9908B0DF}
constexpr uint32_t kMtRandomStaticInitSeed = 0x1105; // 4357, the static initializer at 0x00B7AEF0

struct MtRandom {
    uint32_t mt[kMtRandomWords];    // 0x0000
    int32_t mti;                    // 0x09C0
};                                  // Size: 0x09C4
static_assert(sizeof(MtRandom) == 0x9C4, "the game's generator block is 2500 bytes");
static_assert(offsetof(MtRandom, mti) == 0x9C0, "next_table reads/writes [state+0x9C0]");

// sEffectBase instances each carry their OWN generator, which BattleSnapshot does not capture today:
// sDevil4Effect::mpInstance 0x00E559C8 and sEffect 0x00E559F8 hold an MtRandom at +0x8024 with a
// seed copy at +0x8020, reseeded by the EventMode property setter (block 0x00E5DA1C..0x00E5E3E3).
// Effects in EventMode therefore draw from state neither netplay peer agrees on.
constexpr uintptr_t kMtRandomSDevil4Effect = 0x00E559C8;
constexpr uintptr_t kMtRandomSEffect = 0x00E559F8;
constexpr uintptr_t kMtRandomEventRngOffset = 0x8024;
constexpr uintptr_t kMtRandomEventSeedOffset = 0x8020;

// The game's own functions, __usercall (verified in the IDB):
//   init        0x008BD2A0 (0x104 B)  init(seed@<ecx>, state@<esi>): fills all 624 words, then
//                                     sets mti = 624 (mov [esi+9C0h],270h). Clobbers eax/ecx/edx,
//                                     leaves esi alone, protects its own edi with push/pop.
//   generate    0x008BD3B0            generate(state@<edx>): the canonical twist, sets mti = 0.
//   next_table  0x004055E0            next_table(state@<edx>) -> <eax>: one tempered u32.
//   rand        0x004055C0            rand(state@<edx>) -> ST(0), the draw as a float in [0,1).
//   getRandPlusMinus 0x0045EAB0       cdecl float range; always draws from the global block.
// mt_random_next_table is for explicit calls and tracing only: the 60 inlined draws never reach it.
// rand/getRandPlusMinus get no wrapper here because an ST(0) return needs x87 marshalling that a
// stub could only get wrong.
void mt_random_init(uint32_t seed, MtRandom* state);
uint32_t mt_random_next_table(MtRandom* state);

// Views into the game's block.
inline MtRandom* mt_random_global()
{
    return (MtRandom*)kMtRandom;
}

inline int32_t mt_random_mti()
{
    return *(const int32_t*)kMtRandomMti;
}
