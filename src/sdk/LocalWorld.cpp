#include "LocalWorld.hpp"
#include "FrameHooks.hpp"
#include "TestSwitch.hpp"

#include <windows.h>
#include <intrin.h>

#include <algorithm>
#include <atomic>
#include <cstring>
#include <memory>

#include "Devil4.hpp"
#include "sUnit.hpp"
#include "MtRandom.hpp"
#include "../utility/FunctionHook.hpp"

#pragma intrinsic(_ReturnAddress)

namespace local_world {
namespace {

constexpr uintptr_t kHeapCtor = 0x008BB6A0;        // MtHeapAllocator ctor: eax type, ecx name, push size, push object; retn 8
constexpr uintptr_t kHeapAlloc = 0x008BB910;       // vtable +0x14: thiscall(this, size, align)
constexpr uintptr_t kHeapFree = 0x008BBAF0;        // vtable +0x18: thiscall(this, pointer)
constexpr uintptr_t kHeapUsed = 0x04, kHeapSize = 0x0C, kHeapRegion = 0x34;
constexpr size_t kLocalBytes = 0x4000000;           // 64 MiB
constexpr size_t kHeapObjectBytes = 0x280;

// sUnit's per-line passes (its vtable 0xBFF890 +0x20, +0x34, +0x24): move (init, delete, move), the second move pass, the
// after pass. Each is thiscall(sUnit, line) and skips a paused line.
constexpr uintptr_t kLineMove = 0x008DC290, kLineMove2 = 0x008DC3D0, kLineAfter = 0x008DC480;
constexpr uint32_t kLinePaused = 2;                 // MoveLine +0x08 bit 1

// The allocator globals a Scope swaps (InitAllocators): Global, Temp, Material, System, Unit, Area, Trans, Array, String.
// Resource and Sound stay: loaded files and sounds are shared with the synced world.
constexpr uintptr_t kSwapped[] = {0x00C307A8, 0x00C307AC, 0x00E143E4, 0x00E14378, 0x00E1434C, 0x00E14344, 0x00E143E0,
    0x00E142FC, 0x00E142F8};
constexpr int kSwappedCount = sizeof(kSwapped) / sizeof(kSwapped[0]);
// Every heap's global, for routing frees by address.
constexpr uintptr_t kAllHeaps[] = {0x00C307A8, 0x00C307AC, 0x00E14348, 0x00E143E4, 0x00E14378, 0x00E1434C, 0x00E14344,
    0x00E143E0, 0x00E142FC, 0x00E143DC};

struct Span {
    uintptr_t begin, end, heap;
};
Span g_spans[16];
int g_span_count = 0;

bool g_enabled = false;
uintptr_t g_local = 0;          // the local heap object
uintptr_t g_base = 0, g_end = 0;
bool g_failed = false;

int g_depth = 0;
void (*g_watch_enter)() = nullptr;
void (*g_watch_exit)() = nullptr;
// The game's one random stream, as the outermost Scope found it: local code draws from it too (effects, model setup), and
// it runs on some machines' ticks and not others' (forward ticks only, or per-machine mod state), so it leaves it as it was.
MtRandom g_rng_saved;
DWORD g_scope_thread = 0;
uintptr_t g_saved[kSwappedCount];

std::unique_ptr<FunctionHook> g_free_hook, g_alloc_hook;
using FreeFn = void(__thiscall*)(uintptr_t, void*);
using AllocFn = void*(__thiscall*)(uintptr_t, size_t, int);
FreeFn g_free_original = nullptr;
AllocFn g_alloc_original = nullptr;
std::atomic<uint32_t> g_rerouted{0}, g_outside{0}, g_ticks{0};
std::atomic<uintptr_t> g_outside_from{0};

uintptr_t owner_of(uintptr_t p)
{
    for (int i = 0; i < g_span_count; ++i) {
        if (p >= g_spans[i].begin && p < g_spans[i].end) {
            return g_spans[i].heap;
        }
    }
    return 0;
}

// DMC4_TRACE_UNIT_ALLOCS: rings of the Unit heap's allocations and frees (any thread: a slot is claimed by an atomic index)
struct UnitAlloc {
    uintptr_t p;
    uint32_t size, seq;
    int frame;
    void* stack[8];
};
struct UnitFree {
    uintptr_t p, from;
    uint32_t seq;
};
constexpr int kUnitAllocs = 1 << 16, kUnitFrees = 1 << 16;
UnitAlloc* g_unit_allocs = nullptr;
UnitFree* g_unit_frees = nullptr;
volatile LONG g_unit_alloc_next = 0, g_unit_free_next = 0, g_unit_seq = 0;
bool trace_unit_allocs()
{
    static const bool on = [] {
        if (!test_switch("DMC4_TRACE_UNIT_ALLOCS")) {
            return false;
        }
        g_unit_allocs = (UnitAlloc*)VirtualAlloc(nullptr, sizeof(UnitAlloc) * kUnitAllocs, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        g_unit_frees = (UnitFree*)VirtualAlloc(nullptr, sizeof(UnitFree) * kUnitFrees, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        return g_unit_allocs && g_unit_frees;
    }();
    return on;
}
uintptr_t unit_heap() { return *(const uintptr_t*)0x00E1434C; }

void __fastcall free_detour(uintptr_t self, void*, void* p)
{
    if (p && trace_unit_allocs() && (self == unit_heap() || owner_of((uintptr_t)p) == unit_heap())) {
        UnitFree& f = g_unit_frees[(InterlockedIncrement(&g_unit_free_next) - 1) & (kUnitFrees - 1)];
        f.p = (uintptr_t)p;
        f.from = (uintptr_t)_ReturnAddress();
        f.seq = (uint32_t)InterlockedIncrement(&g_unit_seq);
    }
    const uintptr_t owner = p && g_local ? owner_of((uintptr_t)p) : 0;
    if (owner && owner != self) {
        g_rerouted.fetch_add(1, std::memory_order_relaxed);
        self = owner;
    }
    g_free_original(self, p);
}

constexpr int kMaxSites = 64;
Site g_sites[kMaxSites];
int g_site_count = 0;
volatile LONG g_site_lock = 0;
std::atomic<uintptr_t> g_count_heap{0};

void note_site(uintptr_t from, size_t size)
{
    while (InterlockedCompareExchange(&g_site_lock, 1, 0) != 0) {
        YieldProcessor();
    }
    int i = 0;
    while (i < g_site_count && g_sites[i].from != from) {
        ++i;
    }
    if (i == g_site_count && g_site_count < kMaxSites) {
        g_sites[g_site_count++] = {from, 0, 0};
    }
    if (i < g_site_count) {
        ++g_sites[i].count;
        g_sites[i].bytes += (uint32_t)size;
    }
    InterlockedExchange(&g_site_lock, 0);
}

struct CodeRange {
    uintptr_t begin, end;
};
CodeRange g_routes[8];
int g_route_count = 0;
std::atomic<uint32_t> g_routed{0};

bool routed_heap(uintptr_t heap)
{
    // the synced heaps only: Resource and Sound are shared, the local one is already right
    const uintptr_t resource = *(const uintptr_t*)0x00E14348, sound = *(const uintptr_t*)0x00E143DC;
    return heap != g_local && heap != resource && heap != sound;
}

// Diagnostics: the call stacks of the latest allocations made in the local heap.
struct Origin {
    uintptr_t p;
    void* stack[12];
    USHORT frames;
};
constexpr int kOrigins = 2048;
Origin g_origins[kOrigins];
volatile LONG g_origin_next = 0;

// Netplay: every block of the game's own heaps starts as zeros. The allocator hands out memory as the last user left it,
// and constructors leave fields unset (a shake's +0x48, a damage vector until the first hit): what a machine read there
// was its own heap's history, which machines with different pasts (a player who joined later) don't share.
std::atomic<bool> g_zero_fill{false};

bool zero_filled_heap(uintptr_t heap)
{
    const uintptr_t temp = *(const uintptr_t*)0x00C307AC;   // scratch the sound thread streams through
    return routed_heap(heap) && heap != temp;
}

void* __fastcall alloc_detour(uintptr_t self, void*, size_t size, int align)
{
    if (g_enabled && g_local && g_route_count) {
        const uintptr_t from = (uintptr_t)_ReturnAddress();
        for (int i = 0; i < g_route_count; ++i) {
            if (from >= g_routes[i].begin && from < g_routes[i].end && routed_heap(self)) {
                g_routed.fetch_add(1, std::memory_order_relaxed);
                self = g_local;
                break;
            }
        }
    }
    void* p = g_alloc_original(self, size, align);
    if (p && trace_unit_allocs() && self == unit_heap()) {
        UnitAlloc& a = g_unit_allocs[(InterlockedIncrement(&g_unit_alloc_next) - 1) & (kUnitAllocs - 1)];
        a.p = (uintptr_t)p;
        a.size = (uint32_t)size;
        a.seq = (uint32_t)InterlockedIncrement(&g_unit_seq);
        a.frame = frame_hooks::tick_frame();
        const USHORT n = CaptureStackBackTrace(1, 8, a.stack, nullptr);
        for (USHORT k = n; k < 8; ++k) {
            a.stack[k] = nullptr;
        }
    }
    if (p && size && g_zero_fill.load(std::memory_order_relaxed) && zero_filled_heap(self)) {
        std::memset(p, 0, size);
    }
    if (p && self == g_local && g_local) {
        Origin& o = g_origins[(InterlockedIncrement(&g_origin_next) - 1) & (kOrigins - 1)];
        o.p = (uintptr_t)p;
        o.frames = CaptureStackBackTrace(1, 12, o.stack, nullptr);
    }
    const uintptr_t counted = g_count_heap.load(std::memory_order_relaxed);
    if (counted && self == counted) {
        note_site((uintptr_t)_ReturnAddress(), size);
    }
    if (g_depth > 0 && self != g_local && GetCurrentThreadId() == g_scope_thread) {
        for (int i = 0; i < kSwappedCount; ++i) {
            if (g_saved[i] == self) {   // a swapped heap reached some other way (a cached allocator pointer)
                if (g_outside.fetch_add(1, std::memory_order_relaxed) == 0) {
                    g_outside_from.store((uintptr_t)_ReturnAddress());
                }
                break;
            }
        }
    }
    return p;
}

// Made on first use, once the game's heaps exist.
// The allocator hooks. Made at process start (install_early) when possible: hooks are written without freezing the
// other threads (FunctionHook skips MinHook's locks), and a job thread running through the allocator while its prologue
// is patched corrupted a heap. They pass everything through until the local heap exists.
bool install_hooks()
{
    if (g_free_hook && g_alloc_hook) {
        return true;
    }
    g_free_hook = std::make_unique<FunctionHook>(kHeapFree, (uintptr_t)&free_detour);
    g_alloc_hook = std::make_unique<FunctionHook>(kHeapAlloc, (uintptr_t)&alloc_detour);
    if (!g_free_hook->create() || !g_alloc_hook->create()) {
        g_free_hook.reset();
        g_alloc_hook.reset();
        return false;
    }
    g_free_original = (FreeFn)(uintptr_t)g_free_hook->get_original();
    g_alloc_original = (AllocFn)(uintptr_t)g_alloc_hook->get_original();
    return true;
}

bool make_heap()
{
    if (g_local) {
        return true;
    }
    if (g_failed || !*(const uintptr_t*)0x00E1434C) {
        return false;
    }
    // The allocator object lives in this DLL: taken from the game's root allocator (as InitAllocators takes its heaps'),
    // it was overwritten when made during startup (a session armed from the command line), and the heap's lists broke.
    alignas(16) static uint8_t storage[kHeapObjectBytes];
    void* object = storage;
    static const char kName[] = "Local";
    uintptr_t heap = 0;
    const uintptr_t ctor = kHeapCtor;
    const size_t bytes = kLocalBytes;
    __asm {
        push bytes
        push object
        mov eax, 3
        lea ecx, kName
        call ctor
        mov heap, eax
    }
    if (!heap || !*(const uintptr_t*)(heap + kHeapRegion)) {
        g_failed = true;
        return false;
    }
    // always take its lock (flag 2; the constructor's 4 locks only while MtAllocator::mJobSafe is on): the routed GPU
    // particle code allocates and frees here from the main thread and from job threads, during startup too
    *(uint16_t*)(heap + 0x16) |= 2;
    g_base = *(const uintptr_t*)(heap + kHeapRegion);
    g_end = g_base + *(const uint32_t*)(heap + kHeapSize);
    g_span_count = 0;
    for (uintptr_t global : kAllHeaps) {
        const uintptr_t h = *(const uintptr_t*)global;
        if (h) {
            const uintptr_t b = *(const uintptr_t*)(h + kHeapRegion);
            g_spans[g_span_count++] = {b, b + *(const uint32_t*)(h + kHeapSize), h};
        }
    }
    g_spans[g_span_count++] = {g_base, g_end, heap};
    if (!install_hooks()) {
        g_failed = true;
        return false;
    }
    MemoryBarrier();
    g_local = heap;   // last: the hooks only reroute once the spans are complete
    return true;
}

MoveLine* local_line()
{
    sUnit* su = devil4_sdk::get_sUnit();
    return su ? &su->mMoveLine[kLine] : nullptr;
}

void set_paused(bool paused)
{
    if (MoveLine* ml = local_line()) {
        uint32_t& flags = *(uint32_t*)((uintptr_t)ml + 8);
        flags = paused ? (flags | kLinePaused) : (flags & ~kLinePaused);
    }
}

void run_pass(uintptr_t fn, sUnit* su)
{
    const int line = kLine;
    __asm {
        push line
        mov ecx, su
        call fn
    }
}

} // namespace

void route_code(uintptr_t begin, uintptr_t end)
{
    if (g_route_count < 8) {
        g_routes[g_route_count++] = {begin, end};
    }
}

void set_zero_fill(bool on)
{
    g_zero_fill.store(on, std::memory_order_relaxed);
}

void set_enabled(bool on)
{
    g_enabled = on;
    if (on) {
        make_heap();   // the hooks route from the first frame (no-op until the game's heaps exist; tick() retries)
    }
    if (on) {
        set_paused(true);
    } else {
        set_paused(false);   // what is on the line moves with the game's passes again
    }
}

bool enabled()
{
    return g_enabled;
}

Scope::Scope() : ok(false)
{
    if (!g_enabled || !make_heap()) {
        return;
    }
    if (g_depth++ == 0) {
        if (g_watch_enter) {
            g_watch_enter();
        }
        std::memcpy(&g_rng_saved, (const void*)kMtRandom, sizeof g_rng_saved);
        g_scope_thread = GetCurrentThreadId();
        for (int i = 0; i < kSwappedCount; ++i) {
            g_saved[i] = *(uintptr_t*)kSwapped[i];
            *(uintptr_t*)kSwapped[i] = g_local;
        }
    }
    ok = true;
}

Scope::~Scope()
{
    if (!ok) {
        return;
    }
    if (--g_depth == 0) {
        for (int i = 0; i < kSwappedCount; ++i) {
            *(uintptr_t*)kSwapped[i] = g_saved[i];
        }
        std::memcpy((void*)kMtRandom, &g_rng_saved, sizeof g_rng_saved);
        if (g_watch_exit) {
            g_watch_exit();
        }
    }
}

bool find_unit_alloc(uintptr_t address, AllocTrace& out)
{
    if (!trace_unit_allocs()) {
        return false;
    }
    const UnitAlloc* best = nullptr;
    for (int i = 0; i < kUnitAllocs; ++i) {
        const UnitAlloc& a = g_unit_allocs[i];
        if (a.p && address >= a.p && address < a.p + std::max<uint32_t>(a.size, 1) && (!best || a.seq > best->seq)) {
            best = &a;
        }
    }
    if (!best) {
        return false;
    }
    out = AllocTrace{};
    out.p = best->p;
    out.size = best->size;
    out.seq = best->seq;
    out.frame = best->frame;
    for (int k = 0; k < 8; ++k) {
        out.stack[k] = (uintptr_t)best->stack[k];
    }
    for (int i = 0; i < kUnitFrees; ++i) {
        const UnitFree& f = g_unit_frees[i];
        if (f.p == best->p && f.seq > best->seq && (!out.freed_seq || f.seq < out.freed_seq)) {
            out.freed_seq = f.seq;
            out.freed_from = f.from;
        }
    }
    return true;
}

int unit_alloc_history(uintptr_t address, AllocTrace* out, int max)
{
    if (!trace_unit_allocs() || max <= 0) {
        return 0;
    }
    const UnitAlloc* found[64];
    int n = 0;
    for (int i = 0; i < kUnitAllocs; ++i) {
        const UnitAlloc& a = g_unit_allocs[i];
        if (!a.p || address < a.p || address >= a.p + std::max<uint32_t>(a.size, 1)) {
            continue;
        }
        if (n < 64) {
            found[n++] = &a;
        } else {   // keep the latest 64
            int oldest = 0;
            for (int k = 1; k < 64; ++k) {
                if (found[k]->seq < found[oldest]->seq) oldest = k;
            }
            if (a.seq > found[oldest]->seq) found[oldest] = &a;
        }
    }
    std::sort(found, found + n, [](const UnitAlloc* x, const UnitAlloc* y) { return x->seq < y->seq; });
    const int first = std::max(0, n - max);
    for (int k = first; k < n; ++k) {
        const UnitAlloc& a = *found[k];
        AllocTrace& t = out[k - first];
        t = AllocTrace{};
        t.p = a.p;
        t.size = a.size;
        t.seq = a.seq;
        t.frame = a.frame;
        for (int s = 0; s < 8; ++s) {
            t.stack[s] = (uintptr_t)a.stack[s];
        }
        for (int i = 0; i < kUnitFrees; ++i) {
            const UnitFree& f = g_unit_frees[i];
            if (f.p == a.p && f.seq > a.seq && (!t.freed_seq || f.seq < t.freed_seq)) {
                t.freed_seq = f.seq;
                t.freed_from = f.from;
            }
        }
    }
    return n - first;
}

void set_scope_watch(void (*enter)(), void (*exit)())
{
    g_watch_enter = enter;
    g_watch_exit = exit;
}

void tick()
{
    if (!g_enabled) {
        return;
    }
    make_heap();
    sUnit* su = devil4_sdk::get_sUnit();
    MoveLine* ml = local_line();
    if (!su || !ml || !ml->mTop) {
        set_paused(true);
        return;
    }
    Scope scope;
    if (!scope.ok) {
        return;
    }
    set_paused(false);
    run_pass(kLineMove, su);
    run_pass(kLineMove2, su);
    run_pass(kLineAfter, su);
    set_paused(true);
    g_ticks.fetch_add(1, std::memory_order_relaxed);
}

void install_early()
{
    install_hooks();
}

bool contains(uintptr_t p)
{
    return g_local && p >= g_base && p < g_end;
}

uintptr_t allocator()
{
    return g_local;
}

int origin(uintptr_t p, void** stack, int max)
{
    for (int i = 0; i < kOrigins; ++i) {
        const Origin& o = g_origins[i];
        if (o.p == p) {
            const int n = std::min<int>(o.frames, max);
            for (int k = 0; k < n; ++k) {
                stack[k] = o.stack[k];
            }
            return n;
        }
    }
    return 0;
}

Stats stats()
{
    Stats s;
    s.ready = g_local != 0;
    s.base = g_base;
    s.size = (uint32_t)(g_end - g_base);
    s.used = g_local ? *(const uint32_t*)(g_local + kHeapUsed) : 0;
    if (MoveLine* ml = local_line()) {
        for (uintptr_t u = (uintptr_t)ml->mTop; u && s.units < 100000; u = *(const uintptr_t*)(u + 8)) {
            ++s.units;
        }
    }
    s.ticks = g_ticks.load();
    s.frees_rerouted = g_rerouted.load();
    s.routed_allocs = g_routed.load();
    s.outside_allocs = g_outside.load();
    s.outside_from = g_outside_from.load();
    return s;
}

void count_sites(uintptr_t heap_global, bool on)
{
    if (on && !make_heap()) {
        return;
    }
    if (on) {
        while (InterlockedCompareExchange(&g_site_lock, 1, 0) != 0) {
            YieldProcessor();
        }
        g_site_count = 0;
        InterlockedExchange(&g_site_lock, 0);
        g_count_heap.store(*(const uintptr_t*)heap_global);
    } else {
        g_count_heap.store(0);
    }
}

int sites(Site* out, int max)
{
    while (InterlockedCompareExchange(&g_site_lock, 1, 0) != 0) {
        YieldProcessor();
    }
    Site copy[kMaxSites];
    const int n = g_site_count;
    std::memcpy(copy, g_sites, sizeof(Site) * n);
    InterlockedExchange(&g_site_lock, 0);
    std::sort(copy, copy + n, [](const Site& a, const Site& b) { return a.count > b.count; });
    const int k = n < max ? n : max;
    std::memcpy(out, copy, sizeof(Site) * k);
    return k;
}

} // namespace local_world
