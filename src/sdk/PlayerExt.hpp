#pragma once

#include <cstdint>

// Per-player state for mods: dmc4_hook's own blocks, hung off pointers the game never touches. Mods that kept a
// global (one value for every player) keep it here instead, so two players using the same move keep their own.
//
//   every uPlayer        [player + 149Ch] -> UPlayerExt   (uPlayer padding before mPosOld)
//   Dante (and the boss) [dante  + 14DF8h] -> DanteExt    (uPlayerDante padding before mBdyCollMgr)
//   Nero                 [nero   + 0CD84h] -> NeroExt     (uPlayerNero padding before mTmpPos)
//
// The three slots are alignment padding no game code or mod reads or writes (checked over the player code
// 0x7A0000-0x830000 and src/). uPlayer::setup 0x7A7A00 (every player class's setup reaches it) allocates the blocks,
// or resets them when the same player is set up again, picking the character block by mPlayerID (+0x1494, set by
// the constructors: 0 Dante, 1 Nero); uPlayer::~uPlayer 0x7A66F0 (every player class's destructor ends in it) frees
// them. Before setup the slots hold garbage: code that can run earlier goes through the accessors, which only trust
// players they allocated for. Asm hooks in a player's own moves read the slot directly.
//
// A mod installs the two hooks once (whichever needs it first):
//     if (!player_ext::hook_installed) {
//         install_hook_absolute(player_ext::kSetupSite, hook_a, &player_ext::detour_setup, &player_ext::jmp_ret_setup, 6);
//         install_hook_absolute(player_ext::kDtorSite, hook_b, &player_ext::detour_dtor, &player_ext::jmp_ret_dtor, 6);
//         player_ext::hook_installed = true;
//     }
namespace player_ext {

constexpr uintptr_t kPlayerSlot = 0x149C;    // UPlayerExt*
constexpr uintptr_t kDanteSlot  = 0x14DF8;   // DanteExt*
constexpr uintptr_t kNeroSlot   = 0xCD84;    // NeroExt*

constexpr uintptr_t kSetupSite = 0x7A7A00;   // push ebp; mov ebp, esp; and esp, -16 (6 bytes), ecx = the player
constexpr uintptr_t kDtorSite  = 0x7A66F0;   // push ecx/ebx/ebp/esi; mov esi, ecx (6 bytes), ecx = the player

// ####################################################################################################################
// ##   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   ##
// ##   !!   NOT GAME MEMORY.  These three structs are dmc4_hook's own; the game never reads or writes them.   !!   ##
// ##   !!                                                                                                     !!   ##
// ##   !!   ASM HOOKS READ THE FIELDS BY OFFSET (the k* constants under each struct):                        !!   ##
// ##   !!     - add new fields ONLY at the end, out of `reserved`, keeping the size;                         !!   ##
// ##   !!     - NEVER reorder, resize or remove a field: other mods' hooks would read the wrong bytes;       !!   ##
// ##   !!     - every field gets a k* offset and a static_assert in PlayerExt.cpp;                           !!   ##
// ##   !!     - the defaults below are what setup resets them to.                                            !!   ##
// ##   !!                                                                                                     !!   ##
// ##   !!   THE SLOTS ARE GARBAGE BEFORE uPlayer::setup. Use the accessors unless the code runs inside the   !!   ##
// ##   !!   player's own moves.                                                                               !!   ##
// ##   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!   ##
// ####################################################################################################################

// ---- UPlayerExt: every player, whichever character ---------------------------- [player + 149Ch] ---------------------
struct UPlayerExt {
    uint8_t reserved[0x100] = {};
};
static_assert(sizeof(UPlayerExt) == 0x100, "UPlayerExt: take new fields out of reserved, keep the size");

// ---- DanteExt: Dantes only (the player's, the boss, co-op's, the benchmark's) -------- [dante + 14DF8h] --------------
struct DanteExt {
    // DanteJdc: the Judgement Cut this Dante's Yamato aerial rave is making, and its shell's multi-hit
    uint8_t jdc_flag = 0;          // +0x00 the rave makes a Judgement Cut
    uint8_t jdc_ready = 0;         // +0x01 the cut may spawn (set before the spawn frame, cleared at the spawn)
    uint8_t pad_02[2] = {};
    float jdc_pos[4] = {};         // +0x04 where (the lock-on target's position at the rave's start)
    int32_t jdc_hits_left = 5;     // +0x14 the cut's hits still to land
    float jdc_offset = 0.0f;       // +0x18 the shell timer the next hit waits for

    uint8_t reserved[0x100 - 0x1C] = {};
};
static_assert(sizeof(DanteExt) == 0x100, "DanteExt: take new fields out of reserved, keep the size");
constexpr uintptr_t kJdcFlag = 0x00, kJdcReady = 0x01, kJdcPos = 0x04, kJdcHitsLeft = 0x14, kJdcOffset = 0x18;

// ---- NeroExt: Neros only (the player's, co-op's, the tutorial's, the benchmark's) ----- [nero + 0CD84h] --------------
struct NeroExt {
    uint8_t reserved[0x100] = {};
};
static_assert(sizeof(NeroExt) == 0x100, "NeroExt: take new fields out of reserved, keep the size");

// The blocks of a player set up since dmc4_hook loaded; null otherwise (and DanteExt for a Nero, NeroExt for a Dante).
UPlayerExt* player(uintptr_t player);
DanteExt* dante(uintptr_t player);
NeroExt* nero(uintptr_t player);

extern bool hook_installed;
extern uintptr_t jmp_ret_setup;   // kSetupSite + 6
extern uintptr_t jmp_ret_dtor;    // kDtorSite + 6
void detour_setup();
void detour_dtor();

} // namespace player_ext
