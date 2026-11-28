#pragma once
// heap_snapshot - rollback state as the game's own memory.
//
// The game keeps its simulation in eleven MtHeapAllocator heaps, one VirtualAlloc region each, made by
// InitAllocators 0x8AFB20 through the heap constructor sub_8BB6A0 (its VirtualAlloc call at 0x8BB6F8). The
// allocator objects themselves (640 bytes each) come from the root allocator sub_8BB540 (0x8BB561). Saving
// and restoring those regions page by page restores every unit, effect, shot, collision list and the
// allocators' free lists together, with every pointer still valid, because the process and its addresses stay
// the same: a rollback cannot leave a unit from the mispredicted run behind, which a field-by-field snapshot
// can.
//
// How:
//   - install_early() (Main.cpp startup, before the game creates its heaps) redirects the VirtualAlloc import
//     cell so the heap regions are made with MEM_WRITE_WATCH; nothing else changes for the game.
//   - begin() copies every page written so far into a shadow (pages never written are still zero).
//   - save(frame): the pages written since the last save (GetWriteWatch) go into the shadow; their previous
//     shadow copy becomes the frame's undo entry. The allocator objects and the .data ranges listed in
//     kDataRanges are copied whole.
//   - load(frame): pages written since the last save are put back from the shadow, then the undo entries of
//     every newer frame are applied, newest first. Memory and shadow are then the state at save(frame).
//
// Left out on purpose: the Resource and Sound heaps (the loader and sound threads own them), and the
// renderer's, sound's and input devices' objects inside the other heaps (exclusions), so a restore never
// rewrites what another thread is using. Byte ranges inside an excluded object keep their current value.
#include <cstdint>
#include <cstddef>

namespace heap_snapshot {

// Around each outermost local_world::Scope (set up by begin()): the resource counts the scope's code changed (the
// resources local units take or let go of) go into every frame kept, so a load's count restore keeps them: local units
// aren't rewound, so neither are their references.
void local_refs_enter();
void local_refs_exit();

constexpr int kMaxHeaps = 16;
constexpr int kMaxMismatches = 400;

struct HeapStats {
    const char* name = "";
    uintptr_t base = 0;
    size_t size = 0;
    bool included = false;
    uint32_t shadow_pages = 0;   // pages the shadow holds (written at least once)
    uint32_t last_dirty = 0;     // pages written between the last two saves
    uint32_t max_dirty = 0;
};

struct Stats {
    HeapStats heaps[kMaxHeaps];
    int heap_count = 0;
    bool installed = false;      // the import cell is redirected
    bool active = false;         // between begin() and end()
    uint32_t records = 0;        // frames that can be loaded
    int oldest_frame = 0, newest_frame = 0;
    uint32_t saves = 0, loads = 0;
    uint32_t repeat_saves = 0;   // saves of a frame already held (ignored)
    float last_save_ms = 0.0f, max_save_ms = 0.0f;
    float last_load_ms = 0.0f, max_load_ms = 0.0f;
    uint32_t last_load_pages = 0;
    size_t pool_bytes = 0;       // shadow + undo buffers
    uint32_t direct_allocs = 0;  // root-allocator regions made while active (not snapshotted)
    uint32_t exclusions = 0;
    uint32_t pinned_resources = 0;   // resources held while active (their counts go with the frames)
    uint32_t locks = 0;              // critical sections the game made (their bytes are never restored)
    const char* last_error = "";
};

// Main.cpp startup_proc, before sDevil4Main creates the heaps.
void install_early();

bool available();                // the heaps were made under write watch
bool active();
bool begin(int max_records);     // keeps at most max_records loadable frames
void end();
bool save(int frame);
bool load(int frame);
bool has_frame(int frame);
// [address, address + bytes) as the save of `frame` left it, without loading it (false: not all in a kept heap)
bool read_at(int frame, uintptr_t address, void* out, size_t bytes);
bool in_heap(uintptr_t address);   // in one of the game's heaps, kept or not
bool block_bytes(uintptr_t block, uint32_t& bytes);   // a heap block that starts at `block`: its size, from its header
const Stats& stats();
void set_included(int heap, bool on);   // only while inactive
// The Resource and Sound heaps are out by default (loaded files, the renderer's and sound's objects); these put them in.
void set_optional_heaps(bool resource, bool sound);

// Probe support: compare the state at the newest save with what it was before a rewind.
struct Mismatch {
    uintptr_t address = 0;       // first differing byte
    uint32_t bytes = 0;          // differing bytes in that page
    const char* where = "";      // heap name, "allocator", ".data"
    uintptr_t block = 0;         // the heap block holding it (its user pointer), 0 = not found
    uint32_t block_bytes = 0;
    uint32_t first_word = 0;     // the block's first dword: an object's vtable
    uintptr_t referrer = 0;      // for a block without a vtable: the first heap block that points at it
    uint32_t referrer_word = 0;  // that block's first dword
    uint32_t referrer_offset = 0;
};
struct VerifyResult {
    uint32_t pages_checked = 0;
    uint32_t pages_differing = 0;
    uint32_t data_bytes_differing = 0;   // .data outside the restored ranges (report only)
    int mismatch_count = 0;
    Mismatch mismatches[kMaxMismatches];
};
// Call right before rewinding to `frame`; the newest save is the reference.
bool verify_begin(int frame);
// Call after re-simulating up to the same newest frame (its save done).
bool verify_end(VerifyResult& out);
void set_track_data(bool on);    // copy .data at every save (for verify's .data report)
// The heap holding `address` and its block (user pointer, bytes); null when no heap holds it (the resync's images, the
// diagnostics). A walk of the heap's blocks: not for every frame.
const char* locate(uintptr_t address, uintptr_t& block, uint32_t& block_bytes);
bool resource_live(uintptr_t resource);   // diagnostics: in sDevil4Resource's table
uintptr_t traced_resource();              // diagnostics: the resource DMC4_TRACE_RES names, once a save saw it
uintptr_t region_base(const char* name);   // 0 = no such heap
void heap_span(uintptr_t& lowest, uintptr_t& highest);   // the lowest and highest address of the heaps (resync, diagnostics)
// Diagnostics: how the snapshot treats `address` (heap, page exclusion, excluded byte, a tracked lock, a shadow copy).
void describe(uintptr_t address, char* out, size_t n);

} // namespace heap_snapshot
