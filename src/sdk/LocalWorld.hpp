#pragma once
// local_world - the part of the game only this machine sees: units that change nothing the game plays with (ModelExtension's
// chains and attachments), kept out of rollback netplay.
//
// Rollback netplay restores the game's heaps page by page (dev/netplay/HeapSnapshot.hpp). A unit made by a visual mod in
// those heaps would be "freed" by every rewind and its lists rewired. While the local world is on:
//   - its units live in their own heap ("Local", made with the game's heap constructor sub_8BB6A0), which no snapshot
//     takes: a Scope points the game's allocator globals (all but Resource and Sound) at it while local code runs, and
//     every MtHeapAllocator free (0x8BBAF0) goes to the heap that owns the address, whichever allocator was named.
//   - they move on their own line (kLine), paused for the game's own passes (every per-line pass skips a paused line) and
//     run by tick() after each forward tick, before the frame is drawn; the line's header is left out of the snapshot.
// Local units may read the synced world and write only what the game never reads back (joints of dress, hair,
// accessories). Off (the default), nothing changes: mods spawn on their usual lines.
#include <cstdint>

namespace local_world {

constexpr int kLine = 31;   // empty in the stages checked; not one of the lines the game's pause stops

// Main.cpp startup, before the game starts its threads: the allocator hooks (inert until the local heap exists).
void install_early();
void set_enabled(bool on);  // netplay / the rollback probe: before the stage's units are spawned
bool enabled();

// While alive (game thread), the game's heaps are the local one. Nested scopes are fine.
struct Scope {
    Scope();
    ~Scope();
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;
    bool ok;                // false: the local heap couldn't be made, nothing was swapped
};

void tick();                // the local line's three passes; called once per forward tick
// Presentation code that allocates from the synced heaps itself, every frame, outside any Scope (the GPU particles in the
// frame's prelude): allocations returning into these ranges are made in the local heap too.
void route_code(uintptr_t begin, uintptr_t end);
bool contains(uintptr_t p); // inside the local heap
uintptr_t allocator();      // the local heap's MtHeapAllocator, 0 before it exists

struct Stats {
    bool ready = false;
    uintptr_t base = 0;
    uint32_t size = 0;
    uint32_t used = 0;              // bytes allocated (the allocator's count)
    uint32_t units = 0;             // on the local line
    uint32_t ticks = 0;
    uint32_t frees_rerouted = 0;    // frees named on another heap than the block's
    uint32_t routed_allocs = 0;     // allocations route_code sent to the local heap
    uint32_t outside_allocs = 0;    // allocations made in a Scope from a heap that wasn't swapped
    uintptr_t outside_from = 0;     // the first one's return address
};
Stats stats();

// Diagnostics: the call stack that allocated `p` in the local heap, if it is among the latest allocations (frames written).
int origin(uintptr_t p, void** stack, int max);

// Diagnostics: the code sites that allocate from a heap (by its global) while counting is on, with their counts.
struct Site {
    uintptr_t from;
    uint32_t count;
    uint32_t bytes;
};
void count_sites(uintptr_t heap_global, bool on);   // clears the table when turned on
int sites(Site* out, int max);                       // most first

} // namespace local_world
