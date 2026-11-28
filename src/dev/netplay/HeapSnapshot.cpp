#include "HeapSnapshot.hpp"
#include <cstdio>
#include <cstdlib>

#include <windows.h>
#include <intrin.h>

#include <algorithm>
#include <cstring>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "utility/CommandLineArgs.hpp"
#include "utility/FunctionHook.hpp"
#include "sdk/LocalWorld.hpp"
#include "sdk/Rollback.hpp"

#include <memory>
#include <set>

#pragma intrinsic(_ReturnAddress)

namespace heap_snapshot {
namespace {

constexpr size_t kPage = 0x1000;

constexpr uintptr_t kVirtualAllocCell = 0x00B850CC;   // the VirtualAlloc import cell
constexpr uintptr_t kHeapSiteReturn = 0x008BB6FE;     // after sub_8BB6A0's call ds:VirtualAlloc (0x8BB6F8): a heap
constexpr uintptr_t kRootSiteReturn = 0x008BB567;     // after sub_8BB540's call (0x8BB561): the root allocator

// MtHeapAllocator (heap constructor sub_8BB6A0, alloc sub_8BB910, memSize 0x8BBA80): 640 bytes, the lock at
// +0x1C (a CRITICAL_SECTION, taken when the flags word +0x16 says so), the region at +0x34, then the free-list
// bins. A block's user pointer p has its unit count at p - 16 and its padding at p - 6.
constexpr size_t kAllocatorBytes = 640;
constexpr uintptr_t kAllocatorLockBegin = 0x1C;
constexpr uintptr_t kAllocatorLockEnd = 0x34;
constexpr uintptr_t kAllocatorRegion = 0x34;

struct HeapSlot {
    uintptr_t global;     // the static MtHeapAllocator* (InitAllocators)
    const char* name;
    bool include;
};
constexpr HeapSlot kHeapSlots[] = {
    {0x00C307A8, "Global", true},
    {0x00C307AC, "Temp", false},       // scratch, and the sound thread streams through it (a rewind crashed its decoder)
    {0x00E14348, "Resource", false},   // loaded files and the renderer's and sound's objects for them: shared, never rewound
    {0x00E143E4, "Material", true},
    {0x00E14378, "System", true},
    {0x00E1434C, "Unit", true},
    {0x00E14344, "Area", true},
    {0x00E143E0, "Trans", true},
    {0x00E142FC, "Array", true},        // 0xE142F8 (String) is the same allocator
    {0x00E143DC, "Sound", false},      // the sound thread's
};

// .data ranges that are simulation state and are restored with the heaps.
struct DataRange {
    uintptr_t address;
    size_t bytes;
};
constexpr DataRange kDataRanges[] = {
    {0x00EAC6B0, 0x9C4},   // MtRandom global_rng (+ mti), sdk/MtRandom.hpp
};
constexpr uintptr_t kDataBegin = 0x00C20000;   // .data + .bss of DevilMayCry4_DX9.exe
constexpr uintptr_t kDataEnd = 0x00EB3000;

// Objects that belong to other threads, the hardware or the picture: their bytes are never restored. Each entry is the
// static that holds the object (with `member`, a member of the object that static points at); `fields` are members of it
// that point at further blocks to leave alone.
struct Owned {
    uintptr_t global;
    uint32_t member;      // then *(object + member), if not 0
    uint32_t member2;     // then *(object + member2), if not 0
    const char* name;
    uint32_t fields[4];
    bool sound;           // the sound system's: rolled back with the Sound heap when that is in
};
constexpr Owned kOwned[] = {
    {0x00E552D8, 0, 0, "sRender", {0x15914, 0x15918, 0x1591C, 0x15920}, false},   // the draw-command buffers
    {0x00E559D4, 0, 0, "sPrim", {0x1E94, 0x1E9C, 0x1EA0, 0}, false},                // primitive sort lists (sPrim::sPrim 0xA340E0)
    {0x00E55700, 0, 0, "sShader", {0, 0, 0, 0}, false},
    {0x00E47D38, 0, 0, "sDevil4Sound", {0, 0, 0, 0}, true},
    {0x00E559D8, 0, 0, "sSoundN", {0, 0, 0, 0}, true},
    {0x00E552D0, 0, 0, "sDevil4Resource", {0, 0, 0, 0}, false},
    {0x00E559C0, 0, 0, "sKeyboard", {0, 0, 0, 0}, false},
    {0x00E559DC, 0, 0, "sMouse", {0, 0, 0, 0}, false},
    {0x00E559C4, 0, 0, "sDevil4Pad", {0, 0, 0, 0}, false},                          // netplay writes the recorded input itself
    // sDevil4Main members the prelude runs outside the tick: the GPU particles (0x8AEF4F: their cClothVertex at +0x20,
    // whose +0x20 vertex buffer applyWorldOffset moves) and the rumble (0x1038C)
    {0x00E5574C, 0x10378, 0, "GPU particles", {0x20, 0, 0, 0}, false},
    {0x00E5574C, 0x10378, 0x20, "GPU particle vertices", {0x20, 0, 0, 0}, false},
    {0x00E5574C, 0x1038C, 0, "sVibration", {0, 0, 0, 0}, false},
};

// Classes whose objects are the renderer's, found by their vtable when the snapshot starts (nothing points at them from a
// known static): the object and the blocks its `fields` point at are left out.
struct OwnedClass {
    uintptr_t vtable;
    const char* name;
    uint32_t fields[2];
};
constexpr OwnedClass kOwnedClasses[] = {
    {0x00C03498, "render vertex buffer", {0x20, 0}},   // +0x20: its 2 MiB CPU copy, rewritten every frame
};

using VirtualAllocFn = LPVOID(WINAPI*)(LPVOID, SIZE_T, DWORD, DWORD);
VirtualAllocFn g_real_virtual_alloc = nullptr;

// Every CRITICAL_SECTION the game makes (its InitializeCriticalSection / DeleteCriticalSection import cells): a restore
// keeps their bytes as they are. Copied back, a lock another thread held at the save stays held for good (the sound
// system's deadlocked the game).
constexpr uintptr_t kInitLockCell = 0x00B850C8;
constexpr uintptr_t kDeleteLockCell = 0x00B8509C;
constexpr size_t kLockBytes = sizeof(CRITICAL_SECTION);
using LockFn = void(WINAPI*)(LPCRITICAL_SECTION);
LockFn g_real_init_lock = nullptr;
LockFn g_real_delete_lock = nullptr;
SRWLOCK g_locks_guard = SRWLOCK_INIT;
std::set<uintptr_t>* g_locks = nullptr;   // allocated at install: the CRT may not have run its initialisers yet

void WINAPI snap_init_lock(LPCRITICAL_SECTION cs)
{
    g_real_init_lock(cs);
    AcquireSRWLockExclusive(&g_locks_guard);
    g_locks->insert((uintptr_t)cs);
    ReleaseSRWLockExclusive(&g_locks_guard);
}

void WINAPI snap_delete_lock(LPCRITICAL_SECTION cs)
{
    AcquireSRWLockExclusive(&g_locks_guard);
    g_locks->erase((uintptr_t)cs);
    ReleaseSRWLockExclusive(&g_locks_guard);
    g_real_delete_lock(cs);
}

// The locks that overlap [begin, end) (caller holds g_locks_guard shared).
template <typename F>
void for_locks_in(uintptr_t begin, uintptr_t end, F&& f)
{
    if (!g_locks) {
        return;
    }
    for (auto it = g_locks->lower_bound(begin >= kLockBytes ? begin - kLockBytes + 1 : 0); it != g_locks->end() && *it < end;
         ++it) {
        f(*it);
    }
}

bool redirect_cell(uintptr_t cell_address, const char* name, void* wrapper, void** real)
{
    DWORD* cell = (DWORD*)cell_address;
    const HMODULE mods[] = {GetModuleHandleA("kernel32.dll"), GetModuleHandleA("KernelBase.dll"),
        GetModuleHandleA("ntdll.dll")};
    const char* rtl = std::strcmp(name, "InitializeCriticalSection") == 0 ? "RtlInitializeCriticalSection"
        : std::strcmp(name, "DeleteCriticalSection") == 0 ? "RtlDeleteCriticalSection" : name;
    bool known = false;
    for (HMODULE m : mods) {
        if (m && (*cell == (DWORD)(uintptr_t)GetProcAddress(m, name) || *cell == (DWORD)(uintptr_t)GetProcAddress(m, rtl))) {
            known = *cell != 0;
        }
    }
    if (!known) {
        return false;
    }
    *real = (void*)(uintptr_t)*cell;
    DWORD old = 0;
    if (!VirtualProtect(cell, sizeof(DWORD), PAGE_READWRITE, &old)) {
        return false;
    }
    *cell = (DWORD)(uintptr_t)wrapper;
    VirtualProtect(cell, sizeof(DWORD), old, &old);
    return true;
}

struct Region {
    const char* name = "";
    uint8_t* base = nullptr;
    size_t size = 0;
    size_t pages = 0;
    bool include = false;
    uintptr_t allocator = 0;              // its MtHeapAllocator object
    std::vector<uint8_t*> shadow;         // per page, the content at the newest save; null = all zero
    std::vector<uint8_t> excluded;        // per page: 0 none, 1 partly, 2 whole page
    uint32_t last_dirty = 0, max_dirty = 0, shadow_pages = 0;
};

Region g_regions[kMaxHeaps];
volatile LONG g_region_lock = 0;
int g_region_count = 0;
bool g_bound = false;
bool g_with_resource = false;
bool g_with_sound = false;
bool g_installed = false;
bool g_active = false;
bool g_track_data = false;
volatile LONG g_direct_allocs = 0;

std::vector<std::pair<uintptr_t, uintptr_t>> g_exclusions;   // [begin, end), sorted, merged
std::vector<std::pair<uintptr_t, uintptr_t>> g_sound_blocks;  // the sound system's objects, while the Sound heap is in
std::vector<PVOID> g_watch;                                    // GetWriteWatch output

struct BlobPart {
    uintptr_t address;
    size_t bytes;
    uintptr_t skip_begin, skip_end;   // bytes not restored (offsets inside the part)
    const char* where;
};
std::vector<BlobPart> g_blob_parts;
size_t g_blob_bytes = 0;

struct UndoEntry {
    uint16_t region;
    uint32_t page;
    uint8_t* old;   // the page before this frame's writes; null = all zero
};
struct Record {
    int frame = 0;
    std::vector<UndoEntry> undo;
    std::vector<uint8_t> blobs;
    std::vector<std::pair<uintptr_t, int32_t>> resources;   // each resource's count, without the hold
    std::vector<uint8_t> mods;                                // the mods' state (sdk/Rollback.hpp)
    std::vector<uintptr_t> sound_units;                       // the unit each sound voice / command followed
};
std::deque<Record> g_records;
size_t g_max_records = 16;

std::vector<uint8_t*> g_free;
std::vector<void*> g_chunks;
size_t g_pool_bytes = 0;

std::vector<uint8_t> g_data_copy;   // .data at the newest save (with g_track_data)

Stats g_stats;

// Resources: the Resource heap is never rewound (loaded files, the renderer's and sound's objects for them), but who holds
// a resource is: its count (+0x44) goes with the frames. Every save records the count of each resource in sDevil4Resource's
// table (+0x1038: 0x800 buckets of 4, sResource::releaseTable 0x8DFC90), every load puts them back, and while the snapshot
// runs each resource also carries kHold more, so none is destroyed while a rewind could bring back what held it (173
// places release one inline, so a hook on cResource::release alone let an effect's .efl go: its next frame read the freed
// record). end() takes the hold off again.
constexpr uintptr_t kResourceManager = 0x00E552D0;
constexpr uintptr_t kResourceTable = 0x1038, kResourceLock = 0x4, kResourceLockFlag = 0x1C;
constexpr int kResourceSlots = 0x800 * 4;
constexpr uintptr_t kResourceCount = 0x44;
constexpr int32_t kHold = 0x100000;
std::unordered_set<uintptr_t> g_held;
std::unordered_map<uintptr_t, int> g_unheld_since;   // resources nothing but the hold keeps: the first frame they were so
uint32_t g_freed_resources = 0;
uintptr_t g_traced = 0;

// cResource::release 0x8DDA00: eax = the resource manager, edi = the resource.
void release_resource(uintptr_t manager, uintptr_t resource)
{
    const uintptr_t fn = 0x008DDA00;
    __asm {
        mov eax, manager
        mov edi, resource
        call fn
    }
}

struct ResourceLock {
    LPCRITICAL_SECTION cs = nullptr;
    ResourceLock()
    {
        const uintptr_t m = *(const uintptr_t*)kResourceManager;
        if (m) {
            cs = (LPCRITICAL_SECTION)(m + kResourceLock);
            EnterCriticalSection(cs);
        }
    }
    ~ResourceLock()
    {
        if (cs) {
            LeaveCriticalSection(cs);
        }
    }
};

template <typename F>
void for_resources(F&& f)
{
    const uintptr_t m = *(const uintptr_t*)kResourceManager;
    if (!m) {
        return;
    }
    const uintptr_t* slots = (const uintptr_t*)(m + kResourceTable);
    for (int i = 0; i < kResourceSlots; ++i) {
        if (slots[i]) {
            f(slots[i]);
        }
    }
}

// Sound is never rewound, but it counts references to resources (the holders sSoundN's reset sub_A42000 releases): each
// queued command (requestSe 0xA47AFC: +0x2C, and +0x30; released when the tail clears the queue, 0xA49E50), each queued
// stream (requestStream 0xA4898E: +0x2C) and each voice (sub_A47E20: its bank +0x320 through sub_A3FDE0, its data +0x324
// at 0xA48137), the voices' kept until they are reused. A count put back as it was at the save forgot the references sound took or gave
// back since, and a bank freed that way while its enemy still used it (a later command read the effect list that took
// its memory). So the counts are recorded without sound's references, and a load adds the ones sound holds then.
constexpr uintptr_t kSound = 0x00E559D8;   // sSoundN::mpInstance (requestSe's object)
constexpr uintptr_t kSoundLock = 0x1082C;
struct SoundHolders { uintptr_t first; uint32_t stride; int count; uintptr_t fields[2]; };
constexpr SoundHolders kSoundHolders[] = {
    {0x24, 60, 0x400, {0x2C, 0x30}},       // commands
    {0xF028, 48, 0x80, {0x2C, 0}},         // streams
    {0x10850, 0x500, 0x80, {0x320, 0x324}},   // voices
    {0x38850, 0x7C0, 8, {0x320, 0x324}},      // stream voices (the same voice class first)
};

void count_sound_references(std::unordered_map<uintptr_t, int32_t>& out)
{
    out.clear();
    const uintptr_t sound = *(const uintptr_t*)kSound;
    if (!sound) {
        return;
    }
    // taken before the resource lock, as requestSe does (sound, then the resource's count)
    EnterCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
    for (const SoundHolders& h : kSoundHolders) {
        for (int i = 0; i < h.count; ++i) {
            for (uintptr_t field : h.fields) {
                if (!field) {
                    continue;
                }
                const uintptr_t r = *(const uintptr_t*)(sound + h.first + i * h.stride + field);
                if (r) {
                    ++out[r];
                }
            }
        }
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
}

int32_t sound_count(const std::unordered_map<uintptr_t, int32_t>& refs, uintptr_t r)
{
    const auto it = refs.find(r);
    return it == refs.end() ? 0 : it->second;
}

// New resources get the hold; the counts without it go into `out`. A resource nothing but the hold has kept since before
// the oldest frame still kept is released for real: no rewind can bring back a holder of it any more (the hold kept every
// short-lived resource otherwise, and a heap ran out).
void hold_and_record(std::vector<std::pair<uintptr_t, int32_t>>& out, int frame, int oldest)
{
    std::unordered_map<uintptr_t, int32_t> sound_refs;
    count_sound_references(sound_refs);
    ResourceLock lock;
    out.clear();
    std::vector<uintptr_t> done;
    for_resources([&](uintptr_t r) {
        int32_t& count = *(int32_t*)(r + kResourceCount);
        if (g_held.insert(r).second) {
            count += kHold;
        }
        const int32_t own = count - kHold;
        if (own > 0) {
            g_unheld_since.erase(r);
        } else {
            auto it = g_unheld_since.emplace(r, frame).first;
            if (it->second < oldest) {
                done.push_back(r);
            }
        }
        out.emplace_back(r, own - sound_count(sound_refs, r));
        static const char* trace = std::getenv("DMC4_TRACE_RES");   // diagnostics: one resource's count at every save
        if (trace && std::strstr((const char*)r + 4, trace)) {
            g_traced = r;
            if (FILE* f = std::fopen("netplay_probe.log", "a")) {
                std::fprintf(f, "res f%d %08X own %d sound %d\n", frame, (unsigned)r, own, sound_count(sound_refs, r));
                std::fclose(f);
            }
        }
    });
    const uintptr_t manager = *(const uintptr_t*)kResourceManager;
    for (uintptr_t r : done) {
        if (FILE* f = std::fopen("netplay_probe.log", "a")) {   // diagnostics
            char path[72] = {};
            std::memcpy(path, (const char*)r + 4, 64);
            std::fprintf(f, "hold: released %08X (%08X) %s at save f%d, count %d\n", (unsigned)r, *(const uint32_t*)r, path,
                frame, *(const int32_t*)(r + kResourceCount));
            std::fclose(f);
        }
        g_unheld_since.erase(r);
        g_held.erase(r);
        *(int32_t*)(r + kResourceCount) = 1;
        release_resource(manager, r);   // its last reference: sResource::releaseTable and the destructor
        ++g_freed_resources;
    }
    // the records may still name them (count 0): a load skips what the table no longer has
}

bool in_rewound_heap(uintptr_t a)
{
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (a >= (uintptr_t)r.base && a < (uintptr_t)r.base + r.size) {
            return r.include;
        }
    }
    return false;
}

// A voice follows the unit its command named (sub_A47E20: command +0x14 / +0x18 -> voice +0xA0 unit, +0xA4 joint; the
// voice job sub_A3EAC0 reads the unit's joint through vtable +0x3C each frame unless the unit is 0). Sound isn't
// rewound, so after a load a voice started in the undone frames can name a unit those frames made, whose memory is
// another object's now (a dead unit's garbage vtable, or a live unit of another class read with a joint it lacks). Each
// save records the unit of every voice and queued command; a load takes the unit off those that changed since (the
// sound plays on where it is).
struct SoundFollow { uintptr_t first; uint32_t stride; int count; uintptr_t unit; };
constexpr SoundFollow kSoundFollows[] = {
    {0x24, 60, 0x400, 0x14},        // commands
    {0x10850, 0x500, 0x80, 0xA0},   // voices
    {0x38850, 0x7C0, 8, 0xA0},      // stream voices
};

void record_sound_units(std::vector<uintptr_t>& out)
{
    out.clear();
    const uintptr_t sound = *(const uintptr_t*)kSound;
    if (!sound) {
        return;
    }
    EnterCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
    for (const SoundFollow& f : kSoundFollows) {
        for (int i = 0; i < f.count; ++i) {
            out.push_back(*(const uintptr_t*)(sound + f.first + i * f.stride + f.unit));
        }
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
}

void drop_new_sound_units(const std::vector<uintptr_t>& saved)
{
    const uintptr_t sound = *(const uintptr_t*)kSound;
    if (!sound || saved.empty()) {
        return;
    }
    size_t k = 0;
    EnterCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
    for (const SoundFollow& f : kSoundFollows) {
        for (int i = 0; i < f.count && k < saved.size(); ++i, ++k) {
            uintptr_t* unit = (uintptr_t*)(sound + f.first + i * f.stride + f.unit);
            if (*unit && *unit != saved[k] && in_rewound_heap(*unit)) {
                *unit = 0;
            }
        }
    }
    LeaveCriticalSection((LPCRITICAL_SECTION)(sound + kSoundLock));
}

void restore_counts(const std::vector<std::pair<uintptr_t, int32_t>>& counts)
{
    std::unordered_map<uintptr_t, int32_t> sound_refs;
    count_sound_references(sound_refs);
    ResourceLock lock;
    std::unordered_set<uintptr_t> live;
    for_resources([&](uintptr_t r) { live.insert(r); });
    for (const auto& [r, count] : counts) {
        if (live.count(r) && g_held.count(r)) {
            *(int32_t*)(r + kResourceCount) = count + sound_count(sound_refs, r) + kHold;
            static const char* trace = std::getenv("DMC4_TRACE_RES");
            if (trace && std::strstr((const char*)r + 4, trace)) {
                if (FILE* f = std::fopen("netplay_probe.log", "a")) {
                    std::fprintf(f, "res load %08X game %d sound %d\n", (unsigned)r, count, sound_count(sound_refs, r));
                    std::fclose(f);
                }
            }
        }
    }
}

void release_hold()
{
    g_unheld_since.clear();
    ResourceLock lock;
    for_resources([&](uintptr_t r) {
        if (g_held.count(r)) {
            int32_t& count = *(int32_t*)(r + kResourceCount);
            count = std::max<int32_t>(count - kHold, 1);   // never 0 here: nothing would free it; it stays loaded
        }
    });
    g_held.clear();
}

uint8_t* pool_get()
{
    if (g_free.empty()) {
        constexpr size_t kChunk = 0x100000;
        void* chunk = VirtualAlloc(nullptr, kChunk, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!chunk) {
            return nullptr;
        }
        g_chunks.push_back(chunk);
        g_pool_bytes += kChunk;
        for (size_t off = kChunk; off > 0; off -= kPage) {
            g_free.push_back((uint8_t*)chunk + off - kPage);
        }
    }
    uint8_t* page = g_free.back();
    g_free.pop_back();
    return page;
}

void pool_put(uint8_t* page)
{
    if (page) {
        g_free.push_back(page);
    }
}

void pool_release()
{
    for (void* chunk : g_chunks) {
        VirtualFree(chunk, 0, MEM_RELEASE);
    }
    g_chunks.clear();
    g_free.clear();
    g_pool_bytes = 0;
}

double now_ms()
{
    LARGE_INTEGER f, c;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&c);
    return (double)c.QuadPart * 1000.0 / (double)f.QuadPart;
}

LPVOID WINAPI snap_virtual_alloc(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect)
{
    const uintptr_t from = (uintptr_t)_ReturnAddress();
    if (from == kHeapSiteReturn && address == nullptr && (type & MEM_COMMIT) != 0) {
        LPVOID region = g_real_virtual_alloc(nullptr, bytes, type | MEM_RESERVE | MEM_WRITE_WATCH, protect);
        if (!region) {
            return g_real_virtual_alloc(address, bytes, type, protect);
        }
        while (InterlockedCompareExchange(&g_region_lock, 1, 0) != 0) {
            YieldProcessor();
        }
        if (g_region_count < kMaxHeaps) {
            Region& r = g_regions[g_region_count++];
            r.base = (uint8_t*)region;
            r.size = bytes;
            r.pages = (bytes + kPage - 1) / kPage;
        }
        InterlockedExchange(&g_region_lock, 0);
        return region;
    }
    if (from == kRootSiteReturn && g_active) {
        InterlockedIncrement(&g_direct_allocs);
    }
    return g_real_virtual_alloc(address, bytes, type, protect);
}

// A block the game allocated from one of the heaps: [p, p + size).
bool block_range(uintptr_t p, uintptr_t& begin, uintptr_t& end)
{
    if (p < 0x10000 || (p & 0xF) != 0) {
        return false;
    }
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        const uintptr_t base = (uintptr_t)r.base;
        if (p >= base + 32 && p < base + r.size) {
            const uint32_t units = *(const uint32_t*)(p - 16);
            const uint16_t pad = *(const uint16_t*)(p - 6);
            const uintptr_t bytes = (uintptr_t)units * 16 - pad;
            if (units == 0 || p + bytes > base + r.size) {
                return false;
            }
            begin = p;
            end = p + bytes;
            return true;
        }
    }
    return false;
}

void bind()
{
    if (g_bound) {
        return;
    }
    for (const HeapSlot& slot : kHeapSlots) {
        const uintptr_t allocator = *(const uintptr_t*)slot.global;
        if (!allocator) {
            continue;
        }
        const uintptr_t base = *(const uintptr_t*)(allocator + kAllocatorRegion);
        for (int i = 0; i < g_region_count; ++i) {
            if ((uintptr_t)g_regions[i].base == base) {
                g_regions[i].name = slot.name;
                g_regions[i].include = slot.include;
                g_regions[i].allocator = allocator;
            }
        }
    }
    for (int i = 0; i < g_region_count; ++i) {
        Region& r = g_regions[i];
        if (r.name && std::strcmp(r.name, "Resource") == 0) {
            r.include = g_with_resource;
        } else if (r.name && std::strcmp(r.name, "Sound") == 0) {
            r.include = g_with_sound;
        }
    }
    for (int i = 0; i < g_region_count; ++i) {
        if (!g_regions[i].allocator) {
            g_regions[i].name = local_world::contains((uintptr_t)g_regions[i].base) ? "Local" : "?";
            g_regions[i].include = false;
        }
    }
    g_bound = true;
}

void build_exclusions()
{
    g_exclusions.clear();
    g_sound_blocks.clear();
    auto add = [](uintptr_t p) {
        uintptr_t b = 0, e = 0;
        if (block_range(p, b, e)) {
            g_exclusions.emplace_back(b, e);
        }
    };
    for (const Owned& owned : kOwned) {
        if (owned.sound && g_with_sound) {
            uintptr_t b = 0, e = 0;
            const uintptr_t object = *(const uintptr_t*)owned.global;
            if (object && block_range(object, b, e)) {
                g_sound_blocks.emplace_back(b, e);
            }
            continue;
        }
        if (owned.global == 0x00E5574C && owned.member == 0x10378 && GetEnvironmentVariableA("DMC4_GPU_IN", nullptr, 0)) {
            continue;   // test: the GPU particles rewound with the effects
        }
        uintptr_t object = *(const uintptr_t*)owned.global;
        if (object && owned.member) {
            object = *(const uintptr_t*)(object + owned.member);
        }
        if (object && owned.member2) {
            object = *(const uintptr_t*)(object + owned.member2);
        }
        if (!object) {
            continue;
        }
        add(object);
        for (uint32_t field : owned.fields) {
            if (field) {
                add(*(const uintptr_t*)(object + field));
            }
        }
    }
    // the renderer's objects found by class: walk every included heap's blocks (32-byte headers, +4 next, +16 units)
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (!r.include) {
            continue;
        }
        const uintptr_t base = (uintptr_t)r.base, end = base + r.size;
        for (uintptr_t h = base; h >= base && h < end;) {
            const uint32_t units = *(const uint32_t*)(h + 16);
            const uintptr_t user = h + 32;
            if (units > 2 && user + 4 <= end) {
                const uintptr_t vt = *(const uintptr_t*)user;
                for (const OwnedClass& c : kOwnedClasses) {
                    if (vt == c.vtable) {
                        add(user);
                        for (uint32_t field : c.fields) {
                            if (field) {
                                add(*(const uintptr_t*)(user + field));
                            }
                        }
                    }
                }
            }
            const uintptr_t next = *(const uintptr_t*)(h + 4);
            if (next <= h) {
                break;
            }
            h = next;
        }
    }
    // the local world's line header (sdk/LocalWorld.hpp): its units are not the synced world's
    if (const uintptr_t su = *(const uintptr_t*)0x00E552CC) {
        const uintptr_t line = su + 0x20 + 0x18 * local_world::kLine;
        g_exclusions.emplace_back(line, line + 0x18);
    }
    std::sort(g_exclusions.begin(), g_exclusions.end());
    std::vector<std::pair<uintptr_t, uintptr_t>> merged;
    for (const auto& range : g_exclusions) {
        if (!merged.empty() && range.first <= merged.back().second) {
            merged.back().second = std::max(merged.back().second, range.second);
        } else {
            merged.push_back(range);
        }
    }
    g_exclusions.swap(merged);
    for (int i = 0; i < g_region_count; ++i) {
        Region& r = g_regions[i];
        r.excluded.assign(r.pages, 0);
        const uintptr_t base = (uintptr_t)r.base;
        for (const auto& range : g_exclusions) {
            if (range.second <= base || range.first >= base + r.size) {
                continue;
            }
            const uintptr_t b = std::max(range.first, base) - base;
            const uintptr_t e = std::min(range.second, base + r.size) - base;
            for (uintptr_t page = b / kPage; page * kPage < e; ++page) {
                const uintptr_t pb = page * kPage, pe = pb + kPage;
                const bool whole = b <= pb && e >= pe;
                if (whole) {
                    r.excluded[page] = 2;
                } else if (r.excluded[page] == 0) {
                    r.excluded[page] = 1;
                }
            }
        }
    }
}

void build_blob_layout()
{
    g_blob_parts.clear();
    g_blob_bytes = 0;
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (r.include && r.allocator) {
            g_blob_parts.push_back({r.allocator, kAllocatorBytes, kAllocatorLockBegin, kAllocatorLockEnd, "allocator"});
        }
    }
    for (const DataRange& range : kDataRanges) {
        g_blob_parts.push_back({range.address, range.bytes, 0, 0, ".data"});
    }
    for (const BlobPart& part : g_blob_parts) {
        g_blob_bytes += part.bytes;
    }
}

// Every page written since the last reset (and resets the watch).
size_t collect_dirty(const Region& r)
{
    ULONG_PTR count = g_watch.size();
    ULONG granularity = 0;
    if (GetWriteWatch(WRITE_WATCH_FLAG_RESET, r.base, r.size, g_watch.data(), &count, &granularity) != 0) {
        return 0;
    }
    return (size_t)count;
}

// Writes `src` (null = zeros) over page `page` of `r`, keeping excluded bytes as they are.
// [from, to) of a page back from `src` (null = zeros), around the excluded ranges.
void restore_span(uint8_t* dst, const uint8_t* src, uintptr_t from, uintptr_t to, bool partly_excluded)
{
    const uintptr_t pb = (uintptr_t)dst;
    if (!partly_excluded) {
        if (src) {
            std::memcpy(dst + (from - pb), src + (from - pb), to - from);
        } else {
            std::memset(dst + (from - pb), 0, to - from);
        }
        return;
    }
    for (uintptr_t a = from; a < to;) {
        auto it = std::upper_bound(g_exclusions.begin(), g_exclusions.end(), std::make_pair(a, UINTPTR_MAX));
        bool inside = false;
        uintptr_t next = to;
        if (it != g_exclusions.begin()) {
            auto prev = std::prev(it);
            if (a < prev->second) {
                inside = true;
                next = std::min(to, prev->second);
            }
        }
        if (!inside && it != g_exclusions.end()) {
            next = std::min(to, it->first);
        }
        if (!inside) {
            const size_t off = a - pb;
            if (src) {
                std::memcpy(dst + off, src + off, next - a);
            } else {
                std::memset(dst + off, 0, next - a);
            }
        }
        a = next;
    }
}

// A lock's owner field names a thread of this process (a stale entry's memory holds other data by now).
bool live_thread(uint32_t id)
{
    static std::unordered_map<uint32_t, bool> known;
    if (id == 0 || (id & 3) != 0) {
        return false;
    }
    auto it = known.find(id);
    if (it != known.end() && it->second) {
        return true;
    }
    bool ours = false;
    if (HANDLE h = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, id)) {
        ours = GetProcessIdOfThread(h) == GetCurrentProcessId();
        CloseHandle(h);
    }
    known[id] = ours;
    return ours;
}

void restore_page(const Region& r, uint32_t page, const uint8_t* src)
{
    uint8_t* dst = r.base + (size_t)page * kPage;
    if (r.excluded[page] == 2) {
        return;
    }
    // The game's locks in this page are never written while they are held, now or in the copy: another thread may enter
    // or leave one while the page is copied, and bytes put back over that leave it owned by a thread that has let it go
    // (or free under one that holds it), so the next EnterCriticalSection waits for good. A lock free in both is
    // restored like any data (the memory may hold something else by now: objects that made a lock aren't always deleted
    // with DeleteCriticalSection), unless its bytes are the same anyway.
    struct Span {
        uintptr_t b, e;
    } skip[32];
    int skip_n = 0;
    const uintptr_t pb = (uintptr_t)dst, pe = pb + kPage;
    AcquireSRWLockShared(&g_locks_guard);
    for_locks_in(pb, pe, [&](uintptr_t lock) {
        const uintptr_t b = std::max(lock, pb), e = std::min(lock + kLockBytes, pe);
        if (e <= b) {
            return;
        }
        constexpr uintptr_t kOwner = offsetof(RTL_CRITICAL_SECTION, OwningThread);
        bool keep;
        if (lock >= pb && lock + kOwner + 4 <= pe) {
            const uint32_t now = *(const uint32_t*)(lock + kOwner);
            uint32_t then = 0;
            if (src) {
                std::memcpy(&then, src + (lock + kOwner - pb), 4);
            }
            keep = live_thread(now) || live_thread(then) ||
                (src ? std::memcmp((const void*)b, src + (b - pb), e - b) == 0 : false);
        } else {
            keep = true;   // straddles the page: left alone rather than guessed
        }
        if (keep && skip_n < 32) {
            skip[skip_n++] = {b, e};
        }
    });
    ReleaseSRWLockShared(&g_locks_guard);
    std::sort(skip, skip + skip_n, [](const Span& x, const Span& y) { return x.b < y.b; });
    const bool partly = r.excluded[page] != 0;
    uintptr_t a = pb;
    for (int i = 0; i < skip_n; ++i) {
        if (skip[i].b > a) {
            restore_span(dst, src, a, skip[i].b, partly);
        }
        a = std::max(a, skip[i].e);
    }
    if (a < pe) {
        restore_span(dst, src, a, pe, partly);
    }
}

bool byte_excluded(uintptr_t a)
{
    auto it = std::upper_bound(g_exclusions.begin(), g_exclusions.end(), std::make_pair(a, UINTPTR_MAX));
    return it != g_exclusions.begin() && a < std::prev(it)->second;
}

void capture_blobs(std::vector<uint8_t>& out)
{
    out.resize(g_blob_bytes);
    size_t off = 0;
    for (const BlobPart& part : g_blob_parts) {
        std::memcpy(out.data() + off, (const void*)part.address, part.bytes);
        off += part.bytes;
    }
}

void apply_blobs(const std::vector<uint8_t>& in)
{
    size_t off = 0;
    for (const BlobPart& part : g_blob_parts) {
        uint8_t* dst = (uint8_t*)part.address;
        const uint8_t* src = in.data() + off;
        if (part.skip_end > part.skip_begin) {
            std::memcpy(dst, src, part.skip_begin);
            std::memcpy(dst + part.skip_end, src + part.skip_end, part.bytes - part.skip_end);
        } else {
            std::memcpy(dst, src, part.bytes);
        }
        off += part.bytes;
    }
}

void drop_oldest()
{
    Record& rec = g_records.front();
    for (const UndoEntry& e : rec.undo) {
        pool_put(e.old);
    }
    g_records.pop_front();
}

void fail(const char* why)
{
    g_stats.last_error = why;
}

void refresh_stats()
{
    g_stats.installed = g_installed;
    g_stats.active = g_active;
    g_stats.heap_count = g_region_count;
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        HeapStats& h = g_stats.heaps[i];
        h.name = r.name ? r.name : "?";
        h.base = (uintptr_t)r.base;
        h.size = r.size;
        h.included = r.include;
        h.shadow_pages = r.shadow_pages;
        h.last_dirty = r.last_dirty;
        h.max_dirty = r.max_dirty;
    }
    g_stats.records = (uint32_t)g_records.size();
    g_stats.oldest_frame = g_records.empty() ? 0 : g_records.front().frame;
    g_stats.newest_frame = g_records.empty() ? 0 : g_records.back().frame;
    g_stats.pool_bytes = g_pool_bytes;
    g_stats.direct_allocs = (uint32_t)g_direct_allocs;
    g_stats.exclusions = (uint32_t)g_exclusions.size();
    g_stats.pinned_resources = (uint32_t)g_held.size();
    if (g_locks) {
        AcquireSRWLockShared(&g_locks_guard);
        g_stats.locks = (uint32_t)g_locks->size();
        ReleaseSRWLockShared(&g_locks_guard);
    }
}

// ---- verify (probe) ----
struct RefPage {
    uint16_t region;
    uint32_t page;
    uint8_t* copy;   // null = zero page
};
std::vector<RefPage> g_ref_pages;
std::unordered_set<uint64_t> g_ref_keys;
std::vector<uint8_t> g_ref_blobs;
std::vector<uint8_t> g_ref_data;
int g_verify_frame = 0;
int g_verify_newest = 0;
bool g_verifying = false;

uint64_t key_of(uint16_t region, uint32_t page)
{
    return ((uint64_t)region << 32) | page;
}

void verify_clear()
{
    for (RefPage& ref : g_ref_pages) {
        pool_put(ref.copy);
    }
    g_ref_pages.clear();
    g_ref_keys.clear();
    g_ref_blobs.clear();
    g_ref_data.clear();
    g_verifying = false;
}

// The block of heap `r` that holds `address`. Block headers (32 bytes, sub_8BB910) form an address-ordered list from
// the region's base: +4 next header, +16 the block's 16-byte units (header included); the user pointer follows the header.
bool find_block(const Region& r, uintptr_t address, uintptr_t& block, uint32_t& bytes)
{
    uintptr_t h = (uintptr_t)r.base;
    const uintptr_t end = h + r.size;
    for (int guard = 0; h && h >= (uintptr_t)r.base && h < end && guard < 4000000; ++guard) {
        const uint32_t units = *(const uint32_t*)(h + 16);
        if (address >= h && address < h + (uintptr_t)units * 16) {
            block = h + 32;
            bytes = units * 16 - 32;
            return true;
        }
        const uintptr_t next = *(const uintptr_t*)(h + 4);
        if (next <= h) {
            break;
        }
        h = next;
    }
    return false;
}

// The first dword in the included heaps equal to the block's address, and the block holding it.
void find_referrer(Mismatch& m)
{
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (!r.include) {
            continue;
        }
        const uint32_t* p = (const uint32_t*)r.base;
        const size_t n = r.size / 4;
        for (size_t k = 0; k < n; ++k) {
            if (p[k] == (uint32_t)m.block && (r.shadow.empty() || r.shadow[(k * 4) / kPage])) {
                const uintptr_t at = (uintptr_t)(p + k);
                uintptr_t block = 0;
                uint32_t bytes = 0;
                if (find_block(r, at, block, bytes)) {
                    m.referrer = block;
                    m.referrer_word = *(const uint32_t*)block;
                    m.referrer_offset = (uint32_t)(at - block);
                    return;
                }
            }
        }
    }
}

void note_mismatch(VerifyResult& out, uintptr_t address, uint32_t bytes, const char* where, const Region* r = nullptr)
{
    if (out.mismatch_count >= kMaxMismatches) {
        return;
    }
    Mismatch m{address, bytes, where};
    if (r && find_block(*r, address, m.block, m.block_bytes)) {
        m.first_word = *(const uint32_t*)m.block;
        const bool vtable = m.first_word >= 0x00B85390 && m.first_word < 0x00C20000;
        // the scan reads every included heap: only for the first few distinct data blocks
        int distinct = 0;
        for (int i = 0; i < out.mismatch_count; ++i) {
            distinct += out.mismatches[i].referrer != 0;
        }
        const bool repeat = out.mismatch_count > 0 && out.mismatches[out.mismatch_count - 1].block == m.block;
        if (!vtable && distinct < 6 && !repeat) {
            find_referrer(m);
        } else if (repeat) {
            m.referrer = out.mismatches[out.mismatch_count - 1].referrer;
            m.referrer_word = out.mismatches[out.mismatch_count - 1].referrer_word;
            m.referrer_offset = out.mismatches[out.mismatch_count - 1].referrer_offset;
        }
    }
    out.mismatches[out.mismatch_count++] = m;
}

bool in_lock(uintptr_t a);

// A value that points into one of the heaps (the local one too): the same object can sit at another address after a
// re-run (the forward frames' presentation work allocates too), so two such pointers aren't a difference in what the
// game plays.
bool heap_pointer(uint32_t v)
{
    for (int i = 0; i < g_region_count; ++i) {
        const uintptr_t b = (uintptr_t)g_regions[i].base;
        if (v >= b && v < b + g_regions[i].size) {
            return true;
        }
    }
    return false;
}

// Compares a page of `r` held in `a` against `b` (null = zero), outside the exclusions.
void compare_page(const Region& r, uint32_t page, const uint8_t* a, const uint8_t* b, VerifyResult& out)
{
    if (r.name && std::strcmp(r.name, "Temp") == 0) {
        return;   // per-frame scratch: restored, not compared
    }
    static const uint8_t zero[kPage] = {};
    const uint8_t* x = a ? a : zero;
    const uint8_t* y = b ? b : zero;
    ++out.pages_checked;
    if (std::memcmp(x, y, kPage) == 0) {
        return;
    }
    const uintptr_t pb = (uintptr_t)r.base + (size_t)page * kPage;
    uint32_t differing = 0;
    uintptr_t first = 0;
    for (size_t i = 0; i < kPage; ++i) {
        if (x[i] != y[i]) {
            const size_t w = i & ~(size_t)3;
            uint32_t vx, vy;
            std::memcpy(&vx, x + w, 4);
            std::memcpy(&vy, y + w, 4);
            if (heap_pointer(vx) && heap_pointer(vy)) {
                continue;
            }
        }
        if (x[i] != y[i] && (r.excluded[page] == 0 || !byte_excluded(pb + i)) && !in_lock(pb + i)) {
            if (!differing) {
                first = pb + i;
            }
            ++differing;
        }
    }
    if (differing) {
        ++out.pages_differing;
        note_mismatch(out, first, differing, r.name, &r);
    }
}

// While the Sound heap is rolled back, the sound thread must not be inside the sound system's state: every lock in the
// sound system's objects is taken (all or none, retried, so no lock order can deadlock) around a save or a load.
struct SoundLocks {
    LPCRITICAL_SECTION held[32];
    int n = 0;
    bool take()
    {
        if (g_sound_blocks.empty()) {
            return true;
        }
        LPCRITICAL_SECTION wanted[32];
        int count = 0;
        AcquireSRWLockShared(&g_locks_guard);
        for (const auto& block : g_sound_blocks) {
            for_locks_in(block.first, block.second, [&](uintptr_t lock) {
                if (lock >= block.first && lock + kLockBytes <= block.second && count < 32) {
                    wanted[count++] = (LPCRITICAL_SECTION)lock;
                }
            });
        }
        ReleaseSRWLockShared(&g_locks_guard);
        const double deadline = now_ms() + 200.0;
        for (;;) {
            n = 0;
            for (int i = 0; i < count; ++i) {
                if (!TryEnterCriticalSection(wanted[i])) {
                    break;
                }
                held[n++] = wanted[i];
            }
            if (n == count) {
                return true;
            }
            release();
            if (now_ms() > deadline) {
                return false;
            }
            SwitchToThread();
        }
    }
    void release()
    {
        while (n > 0) {
            LeaveCriticalSection(held[--n]);
        }
    }
    ~SoundLocks()
    {
        release();
    }
};

bool in_lock(uintptr_t a)
{
    bool inside = false;
    AcquireSRWLockShared(&g_locks_guard);
    for_locks_in(a, a + 1, [&](uintptr_t lock) {
        inside = inside || (a >= lock && a < lock + kLockBytes);
    });
    ReleaseSRWLockShared(&g_locks_guard);
    return inside;
}

} // namespace

void install_early()
{
    if (g_installed) {
        return;
    }
    // the netplay harness's page observer redirects the same cell
    if (utility::check_argument("-netplay-page-watch")) {
        fail("left to the harness's page observer");
        return;
    }
    g_locks = new std::set<uintptr_t>();
    if (!redirect_cell(kInitLockCell, "InitializeCriticalSection", (void*)&snap_init_lock, (void**)&g_real_init_lock)
        || !redirect_cell(kDeleteLockCell, "DeleteCriticalSection", (void*)&snap_delete_lock, (void**)&g_real_delete_lock)) {
        fail("the critical section import cells do not hold the kernel's functions");
        return;
    }
    DWORD* cell = (DWORD*)kVirtualAllocCell;
    const HMODULE k32 = GetModuleHandleA("kernel32.dll");
    const HMODULE kbase = GetModuleHandleA("KernelBase.dll");
    const DWORD real = k32 ? (DWORD)(uintptr_t)GetProcAddress(k32, "VirtualAlloc") : 0;
    const DWORD base_real = kbase ? (DWORD)(uintptr_t)GetProcAddress(kbase, "VirtualAlloc") : 0;
    if (*cell == 0 || (*cell != real && *cell != base_real)) {
        fail("the VirtualAlloc import cell does not hold VirtualAlloc");
        return;
    }
    g_real_virtual_alloc = (VirtualAllocFn)(uintptr_t)*cell;
    DWORD old = 0;
    if (!VirtualProtect(cell, sizeof(DWORD), PAGE_READWRITE, &old)) {
        fail("VirtualProtect on the import cell failed");
        return;
    }
    *cell = (DWORD)(uintptr_t)&snap_virtual_alloc;
    VirtualProtect(cell, sizeof(DWORD), old, &old);
    g_installed = true;
}

bool available()
{
    return g_installed && g_region_count > 0;
}

bool active()
{
    return g_active;
}

void set_included(int heap, bool on)
{
    if (g_active || heap < 0 || heap >= g_region_count) {
        return;
    }
    bind();
    g_regions[heap].include = on && g_regions[heap].allocator != 0;
    refresh_stats();
}

void set_optional_heaps(bool resource, bool sound)
{
    if (g_active) {
        return;
    }
    g_with_resource = resource;
    g_with_sound = sound;
    g_bound = false;
}

void set_track_data(bool on)
{
    g_track_data = on;
    if (!on) {
        g_data_copy.clear();
    }
}

void heap_span(uintptr_t& lowest, uintptr_t& highest)
{
    lowest = ~(uintptr_t)0;
    highest = 0;
    for (int i = 0; i < g_region_count; ++i) {
        lowest = std::min(lowest, (uintptr_t)g_regions[i].base);
        highest = std::max(highest, (uintptr_t)g_regions[i].base + g_regions[i].size);
    }
}

uintptr_t traced_resource()
{
    return g_traced;
}

bool resource_live(uintptr_t resource)
{
    bool live = false;
    for_resources([&](uintptr_t r) { live |= r == resource; });
    return live;
}

const char* locate(uintptr_t address, uintptr_t& block, uint32_t& block_bytes)
{
    block = 0;
    block_bytes = 0;
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (address >= (uintptr_t)r.base && address < (uintptr_t)r.base + r.size) {
            find_block(r, address, block, block_bytes);
            return r.name;
        }
    }
    return nullptr;
}

void describe(uintptr_t a, char* out, size_t n)
{
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (a < (uintptr_t)r.base || a >= (uintptr_t)r.base + r.size) {
            continue;
        }
        const size_t page = (a - (uintptr_t)r.base) / kPage;
        bool lock = false;
        AcquireSRWLockShared(&g_locks_guard);
        for_locks_in(a, a + 4, [&](uintptr_t) { lock = true; });
        ReleaseSRWLockShared(&g_locks_guard);
        snprintf(out, n, "%s%s page %u: excluded %d, byte excluded %d, lock %d, shadow %d", r.name, r.include ? "" : " (not restored)",
            (unsigned)page, page < r.excluded.size() ? r.excluded[page] : -1, (int)byte_excluded(a), (int)lock,
            page < r.shadow.size() ? (r.shadow[page] != nullptr) : -1);
        return;
    }
    snprintf(out, n, "in no heap");
}

uintptr_t region_base(const char* name)
{
    for (int i = 0; i < g_region_count; ++i) {
        if (std::strcmp(g_regions[i].name, name) == 0) {
            return (uintptr_t)g_regions[i].base;
        }
    }
    return 0;
}

// Effect lights (sub_AA82C0: 832-byte objects, freed at 0xAA7950 / 0xAA926F, the only users of off_E145A0) come from the
// game's default allocator (0xE13EB0, plain process memory), which is never rewound, while the light list (the light
// system 0xE559F4) and the generators holding the lights (Generator +0xD8) are: after a load a generator could hold a
// light freed and reused since, and ending it would write into whatever has the memory (a fault at 0x9DDA8D). The slot
// gets an allocator that takes lights from the Unit heap, so they go back with the frames; a light made before the
// switch goes back to the old allocator. Installed once and kept: lights made through it must free back to the Unit
// heap.
namespace light_heap {
constexpr uintptr_t kSlot = 0x00E145A0;
constexpr uintptr_t kUnitSlot = 0x00E1434C;
using AllocFn = void*(__thiscall*)(uintptr_t, size_t, int);
using FreeFn = void(__thiscall*)(uintptr_t, void*);
uintptr_t g_old = 0;
void* g_vtable[32];
struct Allocator { void** vtable; } g_allocator{g_vtable};

void* __fastcall alloc(uintptr_t, void*, size_t size, int align)
{
    const uintptr_t unit = *(const uintptr_t*)kUnitSlot;
    return ((AllocFn)(*(void***)unit)[5])(unit, size, align);
}

void __fastcall free(uintptr_t, void*, void* p)
{
    if (!p) {
        return;
    }
    uintptr_t block = 0;
    uint32_t bytes = 0;
    const bool game_heap = locate((uintptr_t)p, block, bytes) || local_world::contains((uintptr_t)p);
    const uintptr_t owner = game_heap ? *(const uintptr_t*)kUnitSlot : g_old;   // the Unit heap's free hands it to its heap
    ((FreeFn)(*(void***)owner)[6])(owner, p);
}

void install()
{
    const uintptr_t current = *(const uintptr_t*)kSlot;
    if (g_old || !current || !*(const uintptr_t*)kUnitSlot) {
        return;
    }
    g_old = current;
    std::memcpy(g_vtable, *(void***)current, sizeof g_vtable);
    g_vtable[5] = (void*)&alloc;
    g_vtable[6] = (void*)&free;
    DWORD old_protect = 0;
    VirtualProtect((void*)kSlot, 4, PAGE_EXECUTE_READWRITE, &old_protect);
    *(uintptr_t*)kSlot = (uintptr_t)&g_allocator;
    VirtualProtect((void*)kSlot, 4, old_protect, &old_protect);
}
} // namespace light_heap

bool begin(int max_records)
{
    light_heap::install();
    if (g_active) {
        end();
    }
    g_stats.last_error = "";
    if (!available()) {
        fail(g_installed ? "no heap was made under write watch" : "not installed");
        refresh_stats();
        return false;
    }
    bind();
    build_exclusions();
    build_blob_layout();
    size_t largest = 0;
    for (int i = 0; i < g_region_count; ++i) {
        largest = std::max(largest, g_regions[i].pages);
    }
    g_watch.assign(largest + 1, nullptr);
    for (int i = 0; i < g_region_count; ++i) {
        Region& r = g_regions[i];
        r.shadow.clear();
        r.shadow_pages = 0;
        r.last_dirty = r.max_dirty = 0;
        if (!r.include) {
            continue;
        }
        r.shadow.assign(r.pages, nullptr);
        // never reset before: every page the game ever wrote; the others are still zero
        const size_t n = collect_dirty(r);
        for (size_t k = 0; k < n; ++k) {
            const uint32_t page = (uint32_t)(((uint8_t*)g_watch[k] - r.base) / kPage);
            if (r.excluded[page] == 2) {
                continue;
            }
            uint8_t* copy = pool_get();
            if (!copy) {
                fail("out of memory for the shadow");
                end();
                return false;
            }
            std::memcpy(copy, g_watch[k], kPage);
            r.shadow[page] = copy;
            ++r.shadow_pages;
        }
    }
    g_held.clear();
    g_records.clear();
    g_max_records = (size_t)std::max(2, max_records);
    g_direct_allocs = 0;
    g_stats.saves = g_stats.loads = g_stats.repeat_saves = 0;
    g_stats.max_save_ms = g_stats.max_load_ms = 0.0f;
    g_active = true;
    refresh_stats();
    return true;
}

void end()
{
    if (g_active) {
        release_hold();
    }
    verify_clear();
    g_records.clear();
    for (int i = 0; i < g_region_count; ++i) {
        g_regions[i].shadow.clear();
        g_regions[i].shadow.shrink_to_fit();
        g_regions[i].shadow_pages = 0;
    }
    pool_release();
    g_data_copy.clear();
    g_data_copy.shrink_to_fit();
    g_active = false;
    refresh_stats();
}

bool has_frame(int frame)
{
    for (const Record& rec : g_records) {
        if (rec.frame == frame) {
            return true;
        }
    }
    return false;
}

bool block_bytes(uintptr_t block, uint32_t& bytes)
{
    uintptr_t b = 0, e = 0;
    if (!block_range(block, b, e) || b != block) {
        return false;
    }
    bytes = (uint32_t)(e - b);
    return true;
}

bool in_heap(uintptr_t address)
{
    for (int i = 0; i < g_region_count; ++i) {
        const Region& r = g_regions[i];
        if (address >= (uintptr_t)r.base && address < (uintptr_t)r.base + r.size) {
            return true;
        }
    }
    return false;
}

bool read_at(int frame, uintptr_t address, void* out, size_t bytes)
{
    if (!g_active || !has_frame(frame)) {
        return false;
    }
    uint8_t* dst = (uint8_t*)out;
    while (bytes) {
        int ri = -1;
        for (int i = 0; i < g_region_count; ++i) {
            const Region& r = g_regions[i];
            if (address >= (uintptr_t)r.base && address < (uintptr_t)r.base + r.size) {
                ri = i;
                break;
            }
        }
        if (ri < 0 || !g_regions[ri].include) {
            return false;
        }
        const Region& r = g_regions[ri];
        const size_t off = address - (uintptr_t)r.base;
        const uint32_t page = (uint32_t)(off / kPage);
        const size_t in_page = off % kPage;
        const size_t n = std::min(bytes, kPage - in_page);
        // the page as that save left it: the copy the first later save took before its writes, else the newest save's
        const uint8_t* src = r.shadow[page];
        bool found = false;
        if (r.excluded[page] == 2) {
            src = r.base + (size_t)page * kPage;   // never kept: as it is now
            found = true;
        }
        for (auto rec = g_records.begin(); !found && rec != g_records.end(); ++rec) {
            if (rec->frame <= frame) {
                continue;
            }
            for (const UndoEntry& e : rec->undo) {
                if (e.region == (uint16_t)ri && e.page == page) {
                    src = e.old;
                    found = true;
                    break;
                }
            }
        }
        if (src) {
            std::memcpy(dst, src + in_page, n);
        } else {
            std::memset(dst, 0, n);
        }
        dst += n;
        address += n;
        bytes -= n;
    }
    return true;
}

bool save(int frame)
{
    if (!g_active) {
        return false;
    }
    // GekkoNet's health, checksum and replay captures save the frame it just saved again: the pages
    // already hold it (a rewind always loads before saving older frames)
    if (!g_records.empty() && frame <= g_records.back().frame) {
        ++g_stats.repeat_saves;
        return true;
    }
    const double t0 = now_ms();
    SoundLocks sound;
    if (!sound.take()) {
        fail("the sound system kept its locks");
        return false;
    }
    Record rec;
    rec.frame = frame;
    for (int i = 0; i < g_region_count; ++i) {
        Region& r = g_regions[i];
        if (!r.include) {
            if (r.allocator) {
                r.last_dirty = (uint32_t)collect_dirty(r);
                r.max_dirty = std::max(r.max_dirty, r.last_dirty);
            }
            continue;
        }
        const size_t n = collect_dirty(r);
        r.last_dirty = (uint32_t)n;
        r.max_dirty = std::max(r.max_dirty, r.last_dirty);
        for (size_t k = 0; k < n; ++k) {
            const uint32_t page = (uint32_t)(((uint8_t*)g_watch[k] - r.base) / kPage);
            if (r.excluded[page] == 2) {
                continue;
            }
            uint8_t* copy = pool_get();
            if (!copy) {
                fail("out of memory for a save");
                return false;
            }
            std::memcpy(copy, g_watch[k], kPage);
            if (!r.shadow[page]) {
                ++r.shadow_pages;
            }
            rec.undo.push_back({(uint16_t)i, page, r.shadow[page]});
            r.shadow[page] = copy;
        }
    }
    capture_blobs(rec.blobs);
    hold_and_record(rec.resources, frame, g_records.empty() ? frame : g_records.front().frame);
    rollback::capture(rec.mods);
    if (!g_with_sound) {
        record_sound_units(rec.sound_units);
    }
    if (g_track_data) {
        g_data_copy.resize(kDataEnd - kDataBegin);
        std::memcpy(g_data_copy.data(), (const void*)kDataBegin, kDataEnd - kDataBegin);
    }
    g_records.push_back(std::move(rec));
    while (g_records.size() > g_max_records && !g_verifying) {
        drop_oldest();
    }
    const float ms = (float)(now_ms() - t0);
    ++g_stats.saves;
    g_stats.last_save_ms = ms;
    g_stats.max_save_ms = std::max(g_stats.max_save_ms, ms);
    refresh_stats();
    return true;
}

bool load(int frame)
{
    if (!g_active) {
        return false;
    }
    if (!has_frame(frame)) {
        fail("load of a frame that is no longer kept");
        return false;
    }
    const double t0 = now_ms();
    SoundLocks sound;
    if (!sound.take()) {
        fail("the sound system kept its locks");
        return false;
    }
    uint32_t pages = 0;
    // pages written after the newest save go back to it
    for (int i = 0; i < g_region_count; ++i) {
        Region& r = g_regions[i];
        if (!r.include) {
            continue;
        }
        const size_t n = collect_dirty(r);
        for (size_t k = 0; k < n; ++k) {
            const uint32_t page = (uint32_t)(((uint8_t*)g_watch[k] - r.base) / kPage);
            restore_page(r, page, r.shadow[page]);
            ++pages;
        }
    }
    // then every newer frame is undone, newest first
    while (g_records.back().frame > frame) {
        Record& rec = g_records.back();
        for (auto it = rec.undo.rbegin(); it != rec.undo.rend(); ++it) {
            Region& r = g_regions[it->region];
            restore_page(r, it->page, it->old);
            pool_put(r.shadow[it->page]);
            r.shadow[it->page] = it->old;
            if (!it->old) {
                --r.shadow_pages;
            }
            ++pages;
        }
        g_records.pop_back();
    }
    apply_blobs(g_records.back().blobs);
    restore_counts(g_records.back().resources);
    if (!g_with_sound) {
        drop_new_sound_units(g_records.back().sound_units);
    }
    rollback::apply(g_records.back().mods);
    // the restore itself was watched
    for (int i = 0; i < g_region_count; ++i) {
        if (g_regions[i].include) {
            ResetWriteWatch(g_regions[i].base, g_regions[i].size);
        }
    }
    const float ms = (float)(now_ms() - t0);
    ++g_stats.loads;
    g_stats.last_load_ms = ms;
    g_stats.max_load_ms = std::max(g_stats.max_load_ms, ms);
    g_stats.last_load_pages = pages;
    refresh_stats();
    return true;
}

const Stats& stats()
{
    if (!g_active) {
        refresh_stats();
    }
    return g_stats;
}

bool verify_begin(int frame)
{
    verify_clear();
    if (!g_active || g_records.empty() || !has_frame(frame)) {
        return false;
    }
    g_verify_frame = frame;
    g_verify_newest = g_records.back().frame;
    for (const Record& rec : g_records) {
        if (rec.frame <= frame) {
            continue;
        }
        for (const UndoEntry& e : rec.undo) {
            const uint64_t key = key_of(e.region, e.page);
            if (!g_ref_keys.insert(key).second) {
                continue;
            }
            const uint8_t* current = g_regions[e.region].shadow[e.page];
            uint8_t* copy = nullptr;
            if (current) {
                copy = pool_get();
                if (!copy) {
                    verify_clear();
                    return false;
                }
                std::memcpy(copy, current, kPage);
            }
            g_ref_pages.push_back({e.region, e.page, copy});
        }
    }
    g_ref_blobs = g_records.back().blobs;
    g_ref_data = g_data_copy;
    g_verifying = true;
    return true;
}

bool verify_end(VerifyResult& out)
{
    out = VerifyResult{};
    if (!g_verifying || g_records.empty() || g_records.back().frame != g_verify_newest) {
        verify_clear();
        return false;
    }
    // pages the original run wrote after the rewind point: they must hold what they held
    for (const RefPage& ref : g_ref_pages) {
        const Region& r = g_regions[ref.region];
        compare_page(r, ref.page, r.shadow[ref.page], ref.copy, out);
    }
    // pages only the re-run wrote: the original left them as they were at the rewind point
    std::unordered_set<uint64_t> seen;
    for (const Record& rec : g_records) {
        if (rec.frame <= g_verify_frame) {
            continue;
        }
        for (const UndoEntry& e : rec.undo) {
            const uint64_t key = key_of(e.region, e.page);
            if (g_ref_keys.count(key) || !seen.insert(key).second) {
                continue;
            }
            const Region& r = g_regions[e.region];
            compare_page(r, e.page, r.shadow[e.page], e.old, out);
        }
    }
    // allocator objects (the lock excluded) and restored .data ranges
    const std::vector<uint8_t>& now = g_records.back().blobs;
    size_t off = 0;
    for (const BlobPart& part : g_blob_parts) {
        uint32_t differing = 0;
        uintptr_t first = 0;
        for (size_t i = 0; i < part.bytes; ++i) {
            if (i >= part.skip_begin && i < part.skip_end) {
                continue;
            }
            if (now[off + i] != g_ref_blobs[off + i]) {
                if (!differing) {
                    first = part.address + i;
                }
                ++differing;
            }
        }
        if (differing) {
            ++out.pages_differing;
            note_mismatch(out, first, differing, part.where);
        }
        off += part.bytes;
    }
    // .data outside the restored ranges: reported, not restored
    if (!g_ref_data.empty() && g_ref_data.size() == g_data_copy.size()) {
        auto restored = [](uintptr_t a) {
            for (const DataRange& range : kDataRanges) {
                if (a >= range.address && a < range.address + range.bytes) {
                    return true;
                }
            }
            return false;
        };
        uintptr_t run_begin = 0;
        uint32_t run_bytes = 0;
        for (size_t i = 0; i < g_data_copy.size(); ++i) {
            const uintptr_t a = kDataBegin + i;
            const bool differs = g_data_copy[i] != g_ref_data[i] && !restored(a);
            if (differs) {
                ++out.data_bytes_differing;
                if (run_bytes && a - (run_begin + run_bytes) <= 16) {
                    run_bytes = (uint32_t)(a - run_begin + 1);
                } else {
                    if (run_bytes) {
                        note_mismatch(out, run_begin, run_bytes, ".data (not restored)");
                    }
                    run_begin = a;
                    run_bytes = 1;
                }
            }
        }
        if (run_bytes) {
            note_mismatch(out, run_begin, run_bytes, ".data (not restored)");
        }
    }
    verify_clear();
    while (g_records.size() > g_max_records) {
        drop_oldest();
    }
    refresh_stats();
    return true;
}

} // namespace heap_snapshot
