#include "MtRandom.hpp"

// MSVC inline asm declares no register clobbers, so these bodies keep the register loads adjacent to
// the call and take the result straight from eax: nothing compiler-generated may sit between them,
// and no live value may occupy eax/ecx/edx across init (the game clobbers all three). Same shape as
// the CoopOnline reseed wrapper this module replaced.
void mt_random_init(uint32_t seed, MtRandom* state)
{
    constexpr uintptr_t fn_init = 0x008BD2A0; // MtRandom::init
    _asm {
        mov ecx, seed
        mov esi, state
        call fn_init
    }
}

uint32_t mt_random_next_table(MtRandom* state)
{
    uint32_t result = 0;
    constexpr uintptr_t fn_next_table = 0x004055E0; // MtRandom::nextTable
    _asm {
        mov edx, state
        call fn_next_table
        mov result, eax
    }
    return result;
}
