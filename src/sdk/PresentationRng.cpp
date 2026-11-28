#include "PresentationRng.hpp"
#include "MtRandom.hpp"

#include <cstdint>
#include <intrin.h>

namespace presentation_rng {
namespace {

constexpr uint32_t kWords = sizeof(MtRandom) / 4;   // the 624 words and mti

bool g_enabled = false;
unsigned long g_thread = 0;   // the thread set_enabled(true) ran on; 0 while off
unsigned long g_owner = 0;    // the thread between its outermost enter and its leave
int g_depth = 0;
bool g_swapped = false;     // the outermost enter swapped (leave swaps back even if set_enabled changed meanwhile)
bool g_seeded = false;
uint32_t g_block[kWords];   // the presentation stream while it isn't the live one

// word by word through volatile pointers: no vectorised copy, so no SSE register changes
__declspec(noinline) void swap_blocks()
{
    volatile uint32_t* live = (volatile uint32_t*)kMtRandom;
    volatile uint32_t* own = (volatile uint32_t*)g_block;
    for (uint32_t i = 0; i < kWords; ++i) {
        const uint32_t t = live[i];
        live[i] = own[i];
        own[i] = t;
    }
}

__declspec(noinline) void seed(uint32_t s = 0x2F6B1A93u)
{
    // the game's own seeding (sdk/MtRandom.hpp) over this block, with a seed of its own
    volatile uint32_t* w = (volatile uint32_t*)g_block;
    for (uint32_t i = 0; i < kMtRandomWords; ++i) {
        uint32_t v = s & 0xFFFF0000u;
        s = 69069u * s + 1u;
        v |= s >> 16;
        s = 69069u * s + 1u;
        w[i] = v;
    }
    w[kMtRandomWords] = kMtRandomWords;   // mti: the next draw twists
    g_seeded = true;
}

// the current thread's id, from the TIB (no call: touches no SSE or x87 state)
unsigned long self()
{
    return __readfsdword(0x24);
}

// whether this enter counts: the outermost one only on the ticks' thread while enabled, nested ones on that same thread
bool entering()
{
    const unsigned long t = self();
    if (g_depth == 0) {
        if (!g_enabled || t != g_thread) {
            return false;
        }
        g_owner = t;
        return true;
    }
    return t == g_owner;
}

} // namespace

void set_enabled(bool on)
{
    g_enabled = on;
    g_thread = on ? self() : 0;
}

void enter()
{
    if (entering() && g_depth++ == 0) {
        if (!g_seeded) {
            seed();
        }
        swap_blocks();
        g_swapped = true;
    }
}

void enter_seeded(uint32_t s)
{
    if (entering() && g_depth++ == 0) {
        seed(s);
        swap_blocks();
        g_swapped = true;
    }
}

void leave()
{
    if (g_depth <= 0 || self() != g_owner) {
        return;
    }
    if (--g_depth == 0) {
        if (g_swapped) {
            swap_blocks();
            g_swapped = false;
        }
        g_owner = 0;
    }
}

} // namespace presentation_rng
