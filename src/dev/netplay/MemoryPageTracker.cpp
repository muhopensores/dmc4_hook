// Windows page observer: OS write-watch harvest for the game allocator reservations
// the harness patches onto `netplay_pages::allocate`, plus a fresh VirtualQuery +
// ReadProcessMemory byte comparison for memory the OS cannot watch. The sidecar is
// ASCII JSON Lines (`<run_dir>/pages.jsonl`), opened in `start()` once the recorder
// has signalled readiness; the lifecycle registry and the two allocation wrappers
// stay valid until process exit even after `finish()` or a fatal latch.
//
// Deliberately free of the game SDK, GekkoNet, BattleSnapshot, the framework PCH and
// spdlog: scripts/netplay_memory_pages_smoke.cpp compiles this translation unit on
// its own against the real Win32 APIs.
#include "MemoryPageTracker.hpp"

#include <algorithm>
#include <cinttypes>
#include <cstdarg>
#include <cstdio>
#include <cstring>


namespace netplay_pages {
namespace {

constexpr size_t kMaxRanges = 32;
constexpr size_t kMaxStackExclusions = 64;
constexpr size_t kMaxTrackerReservations = 256;
constexpr size_t kWriterBytes = 64u << 10;
constexpr size_t kFormatBytes = 384;
constexpr size_t kScratchBytes = 64u << 10;
constexpr size_t kSlabBytes = 1u << 20;
constexpr size_t kChangedWindowBytes = 16;
constexpr size_t kSlabLimit = 512;       // 512 x 1 MiB slabs: one per MiB of copy budget
constexpr size_t kSlabBlockBytes = 16u << 20;  // pool blocks: 32-bit address space friendly
constexpr size_t kMinCopyRecords = 4096;
constexpr size_t kMaxCopyRecords = 1u << 18;
constexpr uint64_t kAddressCeiling = 0x100000000ull; // exclusive 4-GiB boundary

struct Reservation {
    uint64_t base;
    uint64_t bytes; // page-rounded requested length; 0 == recycled slot
    uint64_t id;
    DWORD alloc_type;
    bool live;    // mapped; false after MEM_DECOMMIT (identity retained)
    bool watched; // write-watch enabled: only these get queried/reset
};

struct PageCopy {
    uint64_t address;
    uint64_t alloc_base;
    DWORD protect;
    DWORD type;
    size_t length;
    size_t slab;
    size_t offset;
    bool valid;
};

struct Owned {
    uint64_t begin;
    uint64_t end; // exclusive
};

struct Counters {
    size_t written_rows;
    size_t changed_rows;
    size_t changed_bytes;
    size_t compared_bytes;
    size_t unreadable_pages;
};

// ------------------------------------------------------------------- state
SRWLOCK g_srw{};
bool g_prepared;
bool g_started;
bool g_finished;
bool g_fatal;
char g_error[32] = "";
DWORD g_error_win32;
bool g_error_row_emitted;

size_t g_page_size;
size_t g_granularity;
uint64_t g_min_address;
uint64_t g_user_end; // exclusive

uint64_t g_image_base;
size_t g_image_size;
Range g_ranges[kMaxRanges];
size_t g_range_count; // 0 == broad coverage
size_t g_copy_limit;

HANDLE g_journal_file = INVALID_HANDLE_VALUE;
char g_journal[kWriterBytes];
size_t g_journal_used;
bool g_journal_open;
char g_run_id_text[80];

int32_t* g_res_slots; // address / allocation granularity -> entry index or -1
size_t g_res_slot_count;
Reservation* g_res;
size_t g_res_capacity;
size_t g_res_used;

int32_t* g_copy_slots; // address / page size -> valid PageCopy index or -1
size_t g_copy_slot_count;
PageCopy* g_copies;
uint32_t* g_copy_pass;
size_t g_copy_capacity;
size_t g_copy_used;
char* g_copy_slabs[kSlabLimit];
char* g_slab_pool;        // current reservation block; slabs commit inside it
size_t g_slab_pool_used;  // bytes carved from the current block
size_t g_slab_count;
size_t g_slab_offset;
size_t g_copy_bytes;

PVOID* g_watch_addrs;
size_t g_watch_capacity;

char* g_scratch; // page-aligned read buffer, reused across batches
size_t g_scratch_capacity;

Owned g_owned[kMaxTrackerReservations];
size_t g_owned_count;

uint64_t g_stack_bases[kMaxStackExclusions];
size_t g_stack_count;

uint64_t g_reservation_id;
uint32_t g_session;
uint32_t g_save_id;
uint64_t g_loop; // 0 until the first baseline moves to loop 1

Counters g_counters{}; // observations inside the currently open interval
size_t g_watched_bytes;
uint32_t g_pass;

// Scratch for one optional unsigned number in a row: a row uses it at most once, so
// the pointer is only alive until the enclosing snprintf returns.
char g_opt_number[32];
const char* opt_u64_locked(unsigned long long value) noexcept {
    std::snprintf(g_opt_number, sizeof(g_opt_number), "%llu", value);
    return g_opt_number;
}

// --------------------------------------------------------------- latching
void latch_locked(const char* code, const DWORD win32_error) noexcept {
    if (g_fatal) {
        return;
    }
    g_fatal = true;
    size_t i = 0;
    for (; code[i] != '\0' && i + 1 < sizeof(g_error); ++i) {
        g_error[i] = code[i];
    }
    g_error[i] = '\0';
    g_error_win32 = win32_error;
}

// --------------------------------------------------------------- journal
void journal_flush_locked() noexcept {
    if (!g_journal_open || g_journal_used == 0) {
        return;
    }
    if (WriteFile(g_journal_file, g_journal, (DWORD)g_journal_used, NULL, NULL) == 0) {
        latch_locked("io_failed", GetLastError());
        g_journal_used = 0;
        return;
    }
    FlushFileBuffers(g_journal_file); // boundary/finalization only, never per page
    g_journal_used = 0;
}

void journal_append_locked(const char* text, size_t length) noexcept {
    if (!g_journal_open) {
        return;
    }
    if (g_journal_used + length > kWriterBytes) {
        journal_flush_locked();
    }
    if (g_journal_used + length > kWriterBytes) {
        latch_locked("io_failed", 0);
        return;
    }
    std::memcpy(g_journal + g_journal_used, text, length);
    g_journal_used += length;
}

void journal_row_locked(const char* fmt, ...) noexcept {
    if (!g_journal_open) {
        return;
    }
    char line[kFormatBytes];
    va_list args;
    va_start(args, fmt);
    const int length = std::vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);
    if (length <= 0 || (size_t)length >= sizeof(line)) {
        latch_locked("io_failed", 0);
        return;
    }
    journal_append_locked(line, (size_t)length);
}

void journal_error_locked() noexcept {
    if (!g_journal_open || g_error_row_emitted || !g_fatal) {
        return;
    }
    journal_row_locked("{\"kind\":\"error\",\"code\":\"%s\",\"win32_error\":%llu,\"loop\":%llu}\n",
                       g_error, (unsigned long long)g_error_win32, (unsigned long long)g_loop);
    g_error_row_emitted = true;
    journal_flush_locked();   // a latched error must survive even a hard kill
}

// --------------------------------------------------- owned tracker memory
LPVOID own_reserve_locked(size_t bytes) noexcept {
    if (g_page_size == 0 || g_page_size > kScratchBytes
        || g_owned_count >= kMaxTrackerReservations) {
        latch_locked("allocation_failed", 0);
        return nullptr;
    }
    const size_t rounded = (bytes + g_page_size - 1) & ~(g_page_size - 1);
    const LPVOID p = ::VirtualAlloc(NULL, rounded, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (p == nullptr) {
        latch_locked("allocation_failed", GetLastError());
        return nullptr;
    }
    g_owned[g_owned_count++] = {(uint64_t)(uintptr_t)p,
                                (uint64_t)(uintptr_t)p + rounded};
    return p;
}

bool owned_overlap_locked(uint64_t begin, uint64_t end) noexcept {
    for (size_t i = 0; i < g_owned_count; ++i) {
        if (g_owned[i].begin < end && g_owned[i].end > begin) {
            return true;
        }
    }
    return false;
}

// -------------------------------------------------------- fallback copies
void copy_invalidate_at_locked(size_t index) noexcept {
    if (index >= g_copy_used) {
        return;
    }
    PageCopy& c = g_copies[index];
    if (!c.valid) {
        return;
    }
    c.valid = false;
    if ((uint64_t)c.address < (uint64_t)g_copy_slot_count * (uint64_t)g_page_size) {
        g_copy_slots[c.address / g_page_size] = -1;
    }
    g_copy_bytes -= c.length;
}

void copy_invalidate_range_locked(uint64_t begin, uint64_t end) noexcept {
    if (g_page_size == 0 || begin >= end) {
        return;
    }
    const uint64_t aligned_begin = (begin + g_page_size - 1) & ~(uint64_t)(g_page_size - 1);
    if (aligned_begin >= end) {
        return;
    }
    for (uint64_t slot = aligned_begin / g_page_size; slot * g_page_size < end; ++slot) {
        if (slot >= g_copy_slot_count) {
            break;
        }
        if (g_copy_slots[slot] >= 0) {
            copy_invalidate_at_locked((size_t)g_copy_slots[slot]);
        }
    }
}

size_t copy_slot_index_locked(uint64_t address) noexcept {
    if (g_page_size == 0 || (uint64_t)g_copy_slot_count * (uint64_t)g_page_size <= address) {
        return SIZE_MAX;
    }
    const int32_t index = g_copy_slots[address / g_page_size];
    return index >= 0 ? (size_t)index : SIZE_MAX;
}

bool copy_store_locked(uint64_t address, uint64_t alloc_base, DWORD protect, DWORD type,
                       const unsigned char* bytes, size_t length) noexcept {
    if (g_copy_used >= g_copy_capacity) {
        latch_locked("copy_limit", 0);
        return false;
    }
    if (g_copy_bytes + length > g_copy_limit) {
        latch_locked("copy_limit", 0);
        return false;
    }
    if (g_slab_count == 0 || g_slab_offset + length > kSlabBytes) {
        if (g_slab_count >= kSlabLimit) {
            latch_locked("copy_limit", 0);
            return false;
        }
        // Slabs commit inside reservation blocks grown in modest chunks: the game is a
        // 32-bit process without IMAGE_FILE_LARGE_ADDRESS_AWARE (2 GiB of user space,
        // with ~600 MiB of arena reservations already placed), where one contiguous
        // multi-hundred-MiB reserve fails with ERROR_NOT_ENOUGH_MEMORY. Blocks are
        // tracked in g_owned so the observer never watches its own copy memory.
        if (g_slab_pool == nullptr || g_slab_pool_used + kSlabBytes > kSlabBlockBytes) {
            const size_t committed = g_slab_count * kSlabBytes;
            const size_t budget_left =
                g_copy_limit > committed ? g_copy_limit - committed : 0;
            const size_t wanted = budget_left < kSlabBlockBytes
                ? ((budget_left + kSlabBytes - 1) / kSlabBytes) * kSlabBytes
                : kSlabBlockBytes;
            const size_t block = wanted < kSlabBytes ? kSlabBytes : wanted;
            const LPVOID pool = ::VirtualAlloc(NULL, block, MEM_RESERVE, PAGE_READWRITE);
            if (pool == nullptr) {
                latch_locked("allocation_failed", GetLastError());
                return false;
            }
            g_slab_pool = (char*)pool;
            g_slab_pool_used = 0;
            g_owned[g_owned_count++] = {(uint64_t)(uintptr_t)pool,
                                        (uint64_t)(uintptr_t)pool + block};
        }
        const LPVOID slab = ::VirtualAlloc(g_slab_pool + g_slab_pool_used, kSlabBytes,
                                           MEM_COMMIT, PAGE_READWRITE);
        if (slab == nullptr) {
            latch_locked("allocation_failed", GetLastError());
            return false;
        }
        g_copy_slabs[g_slab_count++] = (char*)slab;
        g_slab_pool_used += kSlabBytes;
        g_slab_offset = 0;
    }
    PageCopy& c = g_copies[g_copy_used];
    c.address = address;
    c.alloc_base = alloc_base;
    c.protect = protect;
    c.type = type;
    c.length = length;
    c.slab = g_slab_count - 1;
    c.offset = g_slab_offset;
    c.valid = true;
    std::memcpy(g_copy_slabs[c.slab] + c.offset, bytes, length);
    g_slab_offset += length;
    g_copy_bytes += length;
    g_copy_slots[address / g_page_size] = (int32_t)g_copy_used;
    g_copy_pass[g_copy_used] = g_pass;
    ++g_copy_used;
    return true;
}

bool copy_refresh_locked(PageCopy& c, const unsigned char* bytes) noexcept {
    if (c.slab >= g_slab_count || c.offset + c.length > kSlabBytes) {
        return false;
    }
    std::memcpy(g_copy_slabs[c.slab] + c.offset, bytes, c.length);
    return true;
}

// ----------------------------------------------------------- eligibility
bool writable_protect_locked(const DWORD protect) noexcept {
    return protect == PAGE_READWRITE || protect == PAGE_WRITECOPY
        || protect == PAGE_EXECUTE_READWRITE || protect == PAGE_EXECUTE_WRITECOPY;
}

bool main_image_overlap_locked(uint64_t begin, uint64_t end) noexcept {
    return g_image_size != 0 && g_image_base < end && g_image_base + g_image_size > begin;
}

bool range_selected_locked(uint64_t page) noexcept {
    if (g_range_count == 0) {
        return true;
    }
    for (size_t i = 0; i < g_range_count; ++i) {
        if (g_ranges[i].begin <= page && page < g_ranges[i].end) {
            return true;
        }
    }
    return false;
}

bool stack_excluded_locked(uint64_t alloc_base) noexcept {
    for (size_t i = 0; i < g_stack_count; ++i) {
        if (g_stack_bases[i] == alloc_base) {
            return true;
        }
    }
    return false;
}

void note_stack_locked(uint64_t alloc_base) noexcept {
    if (alloc_base == 0 || stack_excluded_locked(alloc_base)) {
        return;
    }
    if (g_stack_count < kMaxStackExclusions) {
        g_stack_bases[g_stack_count++] = alloc_base;
    }
}

// --------------------------------------------------- registry bookkeeping
size_t res_slot_for_locked(uint64_t address) noexcept {
    if (g_granularity == 0
        || (uint64_t)g_res_slot_count * (uint64_t)g_granularity <= address) {
        return SIZE_MAX;
    }
    return address / g_granularity;
}

// Slots are allocation-granular, so two small reservations can share one slot; a
// slot hit is only a fast path and every miss falls back to the entry scan.
bool reservation_covers_locked(uint64_t page) noexcept {
    const size_t slot = res_slot_for_locked(page);
    if (slot != SIZE_MAX) {
        const int32_t index = g_res_slots[slot];
        if (index >= 0) {
            const Reservation& r = g_res[(size_t)index];
            if (r.bytes != 0 && r.base <= page && page < r.base + r.bytes) {
                return true;
            }
        }
    }
    for (size_t i = 0; i < g_res_used; ++i) {
        const Reservation& r = g_res[i];
        if (r.bytes != 0 && r.base <= page && page < r.base + r.bytes) {
            return true;
        }
    }
    return false;
}

Reservation* reservation_at_locked(uint64_t address) noexcept {
    const size_t slot = res_slot_for_locked(address);
    if (slot != SIZE_MAX) {
        const int32_t index = g_res_slots[slot];
        if (index >= 0 && g_res[(size_t)index].base == address) {
            return &g_res[(size_t)index];
        }
    }
    for (size_t i = 0; i < g_res_used; ++i) {
        if (g_res[i].bytes != 0 && g_res[i].base == address) {
            return &g_res[i];
        }
    }
    return nullptr;
}

void res_mark_locked(size_t begin_slot, size_t end_slot, int32_t index) noexcept {
    for (size_t s = begin_slot; s <= end_slot && s < g_res_slot_count; ++s) {
        g_res_slots[s] = index;
    }
}

void res_note_locked(const Reservation& r, const char* state, const bool allocation) noexcept {
    if (!g_journal_open || g_fatal || g_finished) {
        return;
    }
    if (allocation) {
        journal_row_locked("{\"kind\":\"arena\",\"reservation_id\":%llu,\"base\":\"%08llX\",\"bytes\":%llu,"
                           "\"state\":\"%s\",\"loop\":%llu,\"allocation_type\":\"%08llX\"}\n",
                           (unsigned long long)r.id, (unsigned long long)r.base,
                           (unsigned long long)r.bytes, state, (unsigned long long)g_loop,
                           (unsigned long long)r.alloc_type);
    } else {
        journal_row_locked("{\"kind\":\"arena\",\"reservation_id\":%llu,\"base\":\"%08llX\",\"bytes\":%llu,"
                           "\"state\":\"%s\",\"loop\":%llu}\n",
                           (unsigned long long)r.id, (unsigned long long)r.base,
                           (unsigned long long)r.bytes, state, (unsigned long long)g_loop);
    }
}

size_t res_take_slot_locked() noexcept {
    for (size_t i = 0; i < g_res_used; ++i) {
        if (g_res[i].bytes == 0) {
            return i;
        }
    }
    if (g_res_used >= g_res_capacity) {
        latch_locked("allocation_failed", 0);
        return SIZE_MAX;
    }
    return g_res_used++;
}

void res_register_locked(uint64_t base, size_t bytes, DWORD alloc_type) noexcept {
    const size_t index = res_take_slot_locked();
    if (index == SIZE_MAX) {
        return;
    }
    Reservation& r = g_res[index];
    r.base = base;
    r.bytes = bytes;
    r.id = ++g_reservation_id;
    r.alloc_type = alloc_type;
    r.live = true;
    r.watched = true;
    const size_t first = res_slot_for_locked(base);
    const size_t last = res_slot_for_locked(base + bytes - 1);
    if (first != SIZE_MAX && last != SIZE_MAX && last >= first) {
        res_mark_locked(first, last, (int32_t)index);
    }
    // A watched arena claiming an address invalidates any fallback copy there.
    copy_invalidate_range_locked(base, base + bytes);
    g_watched_bytes += bytes;
    res_note_locked(r, "allocated", true);
}

void res_retire_locked(Reservation& r, const DWORD action) noexcept {
    if (action == MEM_RELEASE) {
        const size_t first = res_slot_for_locked(r.base);
        const size_t last = res_slot_for_locked(r.base + r.bytes - 1);
        if (first != SIZE_MAX && last != SIZE_MAX) {
            res_mark_locked(first, last, -1);
        }
        if (r.watched) {
            g_watched_bytes -= r.bytes;
        }
        r.base = 0;
        r.bytes = 0;
        r.live = false;
        r.watched = false;
    } else {
        r.live = false;
        copy_invalidate_range_locked(r.base, r.base + r.bytes);
    }
}

// -------------------------------------------------------- write watch
bool ensure_watch_capacity_locked(size_t entries) noexcept {
    if (entries <= g_watch_capacity) {
        return true;
    }
    const size_t want = entries + 16;
    const LPVOID buffer = own_reserve_locked(want * sizeof(PVOID));
    if (buffer == nullptr) {
        return false;
    }
    g_watch_addrs = (PVOID*)buffer;
    g_watch_capacity = want;
    return true;
}

bool harvest_reservation_locked(const Reservation& r, const char* harvest,
                                const bool emit_rows) noexcept {
    if (!r.watched || !r.live) {
        return true;
    }
    if (!ensure_watch_capacity_locked(r.bytes / g_page_size + 1)) {
        return false;
    }
    SIZE_T count = g_watch_capacity; // input capacity on every call
    SIZE_T granule = 0;
    SetLastError(0);
    GetWriteWatch(WRITE_WATCH_FLAG_RESET, (PVOID)(uintptr_t)r.base, (SIZE_T)r.bytes,
                  g_watch_addrs, &count, &granule);
    const DWORD query_error = GetLastError();
    // GetWriteWatch reports entries through `count`; success is decided by the error
    // code plus the reported granule, never the return value.
    if (query_error != 0 || granule == 0 || granule > g_granularity
        || g_granularity % granule != 0) {
        latch_locked("write_watch_failed", query_error);
        return false;
    }
    if (count > g_watch_capacity) {
        latch_locked("write_watch_failed", 0);
        return false;
    }
    std::sort(g_watch_addrs, g_watch_addrs + (size_t)count);
    if (!emit_rows) {
        return true;
    }
    for (SIZE_T i = 0; i < count; ++i) {
        const uint64_t address = (uint64_t)(uintptr_t)g_watch_addrs[i];
        if (address < r.base || address >= r.base + r.bytes) {
            latch_locked("write_watch_failed", 0);
            return false;
        }
        ++g_counters.written_rows;
        journal_row_locked("{\"kind\":\"written\",\"from_loop\":%llu,\"reservation_id\":%llu,"
                           "\"address\":\"%08llX\",\"harvest\":\"%s\"}\n",
                           (unsigned long long)g_loop, (unsigned long long)r.id,
                           (unsigned long long)address, harvest);
    }
    return true;
}

// ------------------------------------------------------- byte comparison
void compare_page_locked(uint64_t page, const MEMORY_BASIC_INFORMATION& region,
                         const bool establishing, const unsigned char* bytes,
                         size_t length) noexcept {
    if (g_fatal) {
        return;
    }
    const uint64_t alloc_base = (uint64_t)(uintptr_t)region.AllocationBase;
    const size_t index = copy_slot_index_locked(page);

    if (bytes == nullptr) { // read failure: coverage record, stale copy retired
        if (index != SIZE_MAX) {
            copy_invalidate_at_locked(index);
        }
        ++g_counters.unreadable_pages;
        if (!establishing) {
            journal_row_locked("{\"kind\":\"coverage\",\"from_loop\":%llu,\"address\":\"%08llX\","
                               "\"state\":\"unreadable\"}\n",
                               (unsigned long long)g_loop, (unsigned long long)page);
        }
        return;
    }

    bool rebaeline = false;
    if (index != SIZE_MAX) {
        PageCopy& existing = g_copies[index];
        const bool matched_identity = existing.address == page
            && existing.alloc_base == alloc_base
            && existing.protect == region.Protect
            && existing.type == region.Type;
        if (matched_identity) {
            g_copy_pass[index] = g_pass;
            const unsigned char* before = (const unsigned char*)g_copy_slabs[existing.slab]
                + existing.offset;
            const size_t keep = existing.length < length ? existing.length : length;
            size_t diffs = 0;
            size_t first_diff = keep;
            for (size_t i = 0; i < keep; ++i) {
                if (before[i] != bytes[i]) {
                    if (diffs == 0) {
                        first_diff = i;
                    }
                    ++diffs;
                }
            }
            g_counters.compared_bytes += keep;
            if (diffs == 0) {
                return;
            }
            const size_t window = (keep - first_diff < kChangedWindowBytes)
                                      ? (keep - first_diff) : kChangedWindowBytes;
            char before_window[kChangedWindowBytes * 2 + 1] = "";
            char after_window[kChangedWindowBytes * 2 + 1] = "";
            for (size_t i = 0; i < window; ++i) {
                std::snprintf(before_window + i * 2, 3, "%02X", before[first_diff + i]);
                std::snprintf(after_window + i * 2, 3, "%02X", bytes[first_diff + i]);
            }
            before_window[window * 2] = '\0';
            after_window[window * 2] = '\0';
            const bool in_image = page >= g_image_base && page < g_image_base + g_image_size;
            ++g_counters.changed_rows;
            g_counters.changed_bytes += diffs;
            journal_row_locked("{\"kind\":\"changed\",\"from_loop\":%llu,\"address\":\"%08llX\","
                               "\"allocation_base\":\"%08llX\",\"image_rva\":%s,\"protection\":\"%08llX\","
                               "\"changed_bytes\":%llu,\"first_offset\":%llu,\"before\":\"%s\","
                               "\"after\":\"%s\"}\n",
                               (unsigned long long)g_loop, (unsigned long long)page,
                               (unsigned long long)alloc_base,
                               in_image ? opt_u64_locked((unsigned long long)(page - g_image_base))
                                        : "null",
                               (unsigned long long)region.Protect,
                               (unsigned long long)diffs, (unsigned long long)first_diff,
                               before_window, after_window);
            if (!copy_refresh_locked(existing, bytes)) {
                latch_locked("allocation_failed", 0);
            }
            return;
        }
        // Same address, different mapping identity: retire, then re-baseline.
        if (!establishing) {
            journal_row_locked("{\"kind\":\"coverage\",\"from_loop\":%llu,\"address\":\"%08llX\","
                               "\"state\":\"remapped\"}\n",
                               (unsigned long long)g_loop, (unsigned long long)page);
        }
        copy_invalidate_at_locked(index);
        rebaeline = true;
    }

    if (!establishing && !rebaeline) {
        journal_row_locked("{\"kind\":\"coverage\",\"from_loop\":%llu,\"address\":\"%08llX\","
                           "\"state\":\"added\"}\n",
                           (unsigned long long)g_loop, (unsigned long long)page);
    }
    if (!copy_store_locked(page, alloc_base, region.Protect, region.Type, bytes, length)) {
        return;
    }
}

// --------------------------------------------------------------- sampling
bool sample_locked(const bool establishing) noexcept {
    if (g_scratch == nullptr || g_page_size == 0) {
        latch_locked("allocation_failed", 0);
        return false;
    }
    // Never compare (or read) the sampling thread's own stack reservation.
    char probe{};
    MEMORY_BASIC_INFORMATION self{};
    if (VirtualQuery(&probe, &self, sizeof(self)) == sizeof(self)) {
        note_stack_locked((uint64_t)(uintptr_t)self.AllocationBase);
    }

    MEMORY_BASIC_INFORMATION region{};
    uint64_t cursor = g_min_address;
    ++g_pass;

    while (cursor < g_user_end) {
        if (VirtualQuery((LPCVOID)(uintptr_t)cursor, &region, sizeof(region))
            != sizeof(region)) {
            latch_locked("query_failed", GetLastError());
            return false;
        }
        const uint64_t begin = (uint64_t)(uintptr_t)region.BaseAddress;
        const uint64_t raw_end = begin + (uint64_t)region.RegionSize;
        if (raw_end <= cursor || raw_end < begin) {
            cursor += g_granularity; // non-advancing/overflowing region end
            continue;
        }
        const bool eligible = region.State == MEM_COMMIT
            && (region.Type == MEM_PRIVATE
                || (region.Type == MEM_IMAGE && main_image_overlap_locked(begin, raw_end)))
            && writable_protect_locked(region.Protect)
            && !owned_overlap_locked(begin, raw_end)
            && !stack_excluded_locked((uint64_t)(uintptr_t)region.AllocationBase);

        if (eligible) {
            const uint64_t page_begin = (begin + g_page_size - 1) & ~(uint64_t)(g_page_size - 1);
            const uint64_t page_end = raw_end & ~(uint64_t)(g_page_size - 1);
            uint64_t page = page_begin;
            while (page < page_end) {
                if (reservation_covers_locked(page) || !range_selected_locked(page)) {
                    page += g_page_size;
                    continue;
                }
                size_t batch = 0;
                uint64_t scan = page;
                while (scan < page_end && batch + g_page_size <= g_scratch_capacity
                    && !reservation_covers_locked(scan) && range_selected_locked(scan)) {
                    scan += g_page_size;
                    batch += g_page_size;
                }
                if (batch == 0) {
                    page += g_page_size;
                    continue;
                }
                SIZE_T got = 0;
                const BOOL whole = ReadProcessMemory((HANDLE)GetCurrentProcess(),
                                                     (LPCVOID)(uintptr_t)page, g_scratch,
                                                     (SIZE_T)batch, &got);
                if (whole == FALSE || got != (SIZE_T)batch) {
                    // Localize the disappearing/inaccessible pages, never consume a
                    // partial buffer as valid.
                    for (uint64_t one = page; one < page + batch; one += g_page_size) {
                        SIZE_T one_got = 0;
                        const BOOL one_ok = ReadProcessMemory((HANDLE)GetCurrentProcess(),
                                                              (LPCVOID)(uintptr_t)one, g_scratch,
                                                              (SIZE_T)g_page_size, &one_got);
                        const bool readable = (one_ok != FALSE) && one_got == (SIZE_T)g_page_size;
                        compare_page_locked(one, region, establishing,
                                            readable ? (const unsigned char*)g_scratch : nullptr,
                                            readable ? g_page_size : 0);
                        if (g_fatal) {
                            return false;
                        }
                    }
                } else {
                    size_t offset = 0;
                    for (uint64_t one = page; one < page + batch; one += g_page_size) {
                        compare_page_locked(one, region, establishing,
                                            (const unsigned char*)g_scratch + offset, g_page_size);
                        offset += g_page_size;
                        if (g_fatal) {
                            return false;
                        }
                    }
                }
                page += batch;
            }
        }
        cursor = raw_end;
    }

    if (!establishing) {
        // Fallback pages that went unvisited: they left the eligible set.
        for (size_t i = 0; i < g_copy_used; ++i) {
            if (g_copies[i].valid && g_copy_pass[i] != g_pass) {
                journal_row_locked("{\"kind\":\"coverage\",\"from_loop\":%llu,\"address\":\"%08llX\","
                                   "\"state\":\"removed\"}\n",
                                   (unsigned long long)g_loop, (unsigned long long)g_copies[i].address);
                copy_invalidate_at_locked(i);
            }
        }
    }
    return !g_fatal;
}

// ------------------------------------------------------- loop boundaries
void emit_interval_locked(const uint64_t to_loop, const char* end_kind,
                          const bool partial) noexcept {
    journal_row_locked("{\"kind\":\"interval\",\"from_loop\":%llu,\"to_loop\":%s,\"end\":\"%s\","
                       "\"partial\":%s,\"written_page_observations\":%llu,\"changed_page_observations\":%llu,"
                       "\"changed_bytes\":%llu,\"watched_bytes\":%llu,\"compared_bytes\":%llu,"
                       "\"unreadable_pages\":%llu,\"copy_bytes\":%llu}\n",
                       (unsigned long long)g_loop,
                       to_loop == 0 ? "null" : opt_u64_locked((unsigned long long)to_loop),
                       end_kind, partial ? "true" : "false",
                       (unsigned long long)g_counters.written_rows,
                       (unsigned long long)g_counters.changed_rows,
                       (unsigned long long)g_counters.changed_bytes,
                       (unsigned long long)g_watched_bytes,
                       (unsigned long long)g_counters.compared_bytes,
                       (unsigned long long)g_counters.unreadable_pages,
                       (unsigned long long)g_copy_bytes);
    g_counters = {};
    // Boundary: every closed interval persists its rows, so a run killed before a
    // clean finish() still leaves its completed intervals on disk.
    journal_flush_locked();
}

bool begin_loop_locked() noexcept {
    if (g_loop == 0) {
        bool watched_live = false;
        for (size_t i = 0; i < g_res_used; ++i) {
            if (g_res[i].watched && g_res[i].live) {
                watched_live = true;
                if (!harvest_reservation_locked(g_res[i], "boundary", false)) {
                    return false;
                }
            }
        }
        if (!watched_live) {
            latch_locked("no_watch_arenas", 0);
            return false;
        }
        if (!sample_locked(true)) {
            return false;
        }
        journal_row_locked("{\"kind\":\"baseline\",\"loop\":1,\"watched_bytes\":%llu,"
                           "\"compared_bytes\":0,\"unreadable_pages\":%llu,\"copy_bytes\":%llu}\n",
                           (unsigned long long)g_watched_bytes,
                           (unsigned long long)g_counters.unreadable_pages,
                           (unsigned long long)g_copy_bytes);
        g_counters = {};
        g_loop = 1;
        journal_flush_locked();
        return true;
    }

    for (size_t i = 0; i < g_res_used; ++i) {
        if (g_res[i].watched && g_res[i].live
            && !harvest_reservation_locked(g_res[i], "boundary", true)) {
            return false;
        }
    }
    if (!sample_locked(false)) {
        return false;
    }
    emit_interval_locked(g_loop + 1, "loop_entry", false);
    ++g_loop;
    return true;
}

bool finish_locked() noexcept {
    if (g_started && !g_finished && !g_fatal) {
        for (size_t i = 0; i < g_res_used; ++i) {
            if (g_res[i].watched && g_res[i].live
                && !harvest_reservation_locked(g_res[i], "boundary", true)) {
                break;
            }
        }
        if (g_loop >= 1) {
            emit_interval_locked(0, "done", true);
        }
    }
    journal_error_locked();
    if (g_journal_open) {
        journal_row_locked("{\"kind\":\"end\",\"status\":\"%s\"}\n", g_fatal ? "error" : "ok");
        journal_flush_locked();
        if (g_journal_file != INVALID_HANDLE_VALUE) {
            CloseHandle(g_journal_file);
            g_journal_file = INVALID_HANDLE_VALUE;
        }
    }
    g_journal_open = false;
    g_finished = true;
    return !g_fatal;
}

// ------------------------------------------------------------ event rows
bool session_started_locked(uint32_t check_distance) noexcept {
    ++g_session;
    journal_row_locked("{\"kind\":\"event\",\"event\":\"session\",\"loop\":%llu,\"session\":%u,"
                       "\"check_distance\":%u}\n",
                       (unsigned long long)g_loop, g_session, check_distance);
    return true;
}

bool advance_locked(int frame, bool rolling_back, bool running_ahead) noexcept {
    journal_row_locked("{\"kind\":\"event\",\"event\":\"advance\",\"loop\":%llu,\"session\":%u,"
                       "\"frame\":%d,\"rolling_back\":%s,\"running_ahead\":%s}\n",
                       (unsigned long long)g_loop, g_session, frame,
                       rolling_back ? "true" : "false", running_ahead ? "true" : "false");
    return true;
}

bool save_locked(int frame, uint32_t checksum) noexcept {
    ++g_save_id;
    journal_row_locked("{\"kind\":\"event\",\"event\":\"save\",\"loop\":%llu,\"session\":%u,"
                       "\"frame\":%d,\"checksum\":\"%08X\",\"save_id\":%u}\n",
                       (unsigned long long)g_loop, g_session, frame, checksum, g_save_id);
    return true;
}

bool load_locked(int frame, bool restored) noexcept {
    journal_row_locked("{\"kind\":\"event\",\"event\":\"load\",\"loop\":%llu,\"session\":%u,"
                       "\"frame\":%d,\"restored\":%s}\n",
                       (unsigned long long)g_loop, g_session, frame,
                       restored ? "true" : "false");
    return true;
}

bool desync_locked(int frame, uint32_t local, uint32_t remote, uint32_t count) noexcept {
    journal_row_locked("{\"kind\":\"event\",\"event\":\"desync\",\"loop\":%llu,\"session\":%u,"
                       "\"frame\":%d,\"local\":\"%08X\",\"remote\":\"%08X\",\"count\":%u}\n",
                       (unsigned long long)g_loop, g_session, frame, local, remote, count);
    return true;
}

// ----------------------------------------------------------- validation
bool config_valid_locked(const Config& config) noexcept {
    if (config.path == nullptr || config.run_id == nullptr) {
        return false;
    }
    size_t path_len = 0;
    while (config.path[path_len] != '\0' && path_len < MAX_PATH * 2) {
        ++path_len;
    }
    if (path_len == 0 || config.path[path_len] != '\0') {
        return false; // empty, or longer than the observer's own limit
    }
    bool absolute = false;
    for (size_t i = 0; i + 1 < path_len; ++i) {
        if (config.path[i] == ':' && (config.path[i + 1] == '\\' || config.path[i + 1] == '/')) {
            absolute = true;
            break;
        }
    }
    if (!absolute) {
        return false;
    }
    size_t run_len = 0;
    for (const char* p = config.run_id; *p; ++p) {
        ++run_len;
        if (run_len >= sizeof(g_run_id_text)) {
            return false;
        }
    }
    if (run_len == 0) {
        return false;
    }
    if (config.image_base == 0 || config.image_size == 0) {
        return false;
    }
    if (config.copy_limit_bytes == 0) {
        return false;
    }
    if (config.range_count > kMaxRanges) {
        return false;
    }
    if (config.range_count > 0 && config.ranges == nullptr) {
        return false;
    }
    for (size_t i = 0; i < config.range_count; ++i) {
        const Range& r = config.ranges[i];
        if (r.begin >= r.end || r.end > kAddressCeiling) {
            return false;
        }
    }
    return true;
}

void normalize_ranges_locked(const Range* input, size_t count) noexcept {
    g_range_count = 0;
    if (count == 0) {
        return;
    }
    Range expanded[kMaxRanges];
    size_t made = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint64_t begin = input[i].begin & ~(uint64_t)(g_page_size - 1);
        uint64_t end = (input[i].end + g_page_size - 1) & ~(uint64_t)(g_page_size - 1);
        if (end > kAddressCeiling) {
            end = kAddressCeiling;
        }
        if (end > g_user_end) {
            end = g_user_end;
        }
        if (begin < g_user_end && end > begin) {
            expanded[made++] = {begin, end};
        }
    }
    if (made == 0) {
        return;
    }
    std::sort(expanded, expanded + made,
              [](const Range& a, const Range& b) { return a.begin < b.begin; });
    g_ranges[0] = expanded[0];
    g_range_count = 1;
    for (size_t i = 1; i < made; ++i) {
        Range& last = g_ranges[g_range_count - 1];
        if (expanded[i].begin <= last.end) {
            if (expanded[i].end > last.end) {
                last.end = expanded[i].end;
            }
            continue;
        }
        g_ranges[g_range_count++] = expanded[i];
    }
}

// The fallback-copy array is sized from the configured byte budget: one record per
// page of budget, clamped. prepare() pre-allocates nothing here, so until start()
// runs there are no fallback copies and the slot table stays empty.
bool grow_copy_records_locked(size_t capacity) noexcept {
    if (g_page_size == 0 || capacity <= g_copy_capacity) {
        return g_copy_capacity != 0;
    }
    const LPVOID records = own_reserve_locked(capacity * sizeof(PageCopy));
    const LPVOID passes = own_reserve_locked(capacity * sizeof(uint32_t));
    if (records == nullptr || passes == nullptr) {
        return false;
    }
    g_copies = (PageCopy*)records;
    g_copy_pass = (uint32_t*)passes;
    g_copy_capacity = capacity;
    std::memset(g_copies, 0, capacity * sizeof(PageCopy));
    std::memset(g_copy_pass, 0, capacity * sizeof(uint32_t));
    std::memset(g_copy_slots, 0xFF, g_copy_slot_count * sizeof(int32_t));
    g_copy_used = 0;
    g_copy_bytes = 0;
    return true;
}

bool start_locked(const Config& config) noexcept {
    if (g_fatal) {
        return false;
    }
    if (g_started) {
        return true;
    }
    if (!config_valid_locked(config)) {
        latch_locked("invalid_config", 0);
        return false;
    }
    size_t i = 0;
    for (; config.run_id[i] != '\0' && i + 1 < sizeof(g_run_id_text); ++i) {
        g_run_id_text[i] = config.run_id[i];
    }
    g_run_id_text[i] = '\0';

    g_image_base = (uint64_t)config.image_base;
    g_image_size = config.image_size;
    g_copy_limit = config.copy_limit_bytes;
    normalize_ranges_locked(config.ranges, config.range_count);

    g_journal_file = CreateFileA(config.path, GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | CREATE_ALWAYS, NULL,
                                 CREATE_ALWAYS, 0, NULL);
    if (g_journal_file == INVALID_HANDLE_VALUE) {
        latch_locked("io_failed", GetLastError());
        return false;
    }
    g_journal_open = true;

    journal_row_locked("{\"kind\":\"header\",\"version\":1,\"run_id\":\"%s\",\"mode\":\"hybrid\","
                       "\"scope\":\"%s\",\"page_size\":%u,\"image_base\":\"%08llX\",\"image_size\":%llu,"
                       "\"ranges\":[",
                       g_run_id_text, g_range_count == 0 ? "broad" : "ranges",
                       (unsigned)g_page_size, (unsigned long long)g_image_base,
                       (unsigned long long)g_image_size);
    for (size_t r = 0; r < g_range_count; ++r) {
        journal_row_locked("%s\"%08llX:%08llX\"", r == 0 ? "" : ",",
                           (unsigned long long)g_ranges[r].begin,
                           (unsigned long long)g_ranges[r].end);
    }
    journal_row_locked("],\"copy_limit_bytes\":%llu}\n", (unsigned long long)g_copy_limit);

    size_t records = kMinCopyRecords;
    if (g_page_size != 0) {
        const size_t by_budget = (size_t)((g_copy_limit + g_page_size - 1) / g_page_size);
        if (by_budget > records) {
            records = by_budget;
        }
    }
    if (records > kMaxCopyRecords) {
        records = kMaxCopyRecords;
    }
    if (!grow_copy_records_locked(records)) {
        journal_error_locked();
        return false;
    }

    for (size_t e = 0; e < g_res_used; ++e) {
        if (g_res[e].bytes != 0 && g_res[e].live) {
            res_note_locked(g_res[e], "allocated", true);
        }
    }
    // The header and arena rows must be visible even if the run dies before any
    // interval closes (the runner kills the game at its timeout without a clean
    // finish(), and a sidecar with zero bytes is reported as a contract error).
    journal_flush_locked();
    g_started = true;
    return !g_fatal;
}

} // namespace

// ------------------------------------------------------------- entry points
bool prepare() noexcept {
    if (g_prepared) {
        return true;
    }
    InitializeSRWLock(&g_srw);
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    g_page_size = info.dwPageSize;
    g_granularity = info.dwAllocationGranularity;
    if (g_page_size == 0 || g_granularity == 0 || g_page_size > kScratchBytes) {
        latch_locked("invalid_config", 0);
        return false;
    }
    g_min_address = ((uint64_t)(uintptr_t)info.lpMinimumApplicationAddress
                     + g_page_size - 1) & ~(uint64_t)(g_page_size - 1);
    const uint64_t max_address = (uint64_t)(uintptr_t)info.lpMaximumApplicationAddress;
    g_user_end = (max_address + 1 < kAddressCeiling) ? (max_address + 1) : kAddressCeiling;

    AcquireSRWLockExclusive(&g_srw);
    const size_t res_slots = (g_user_end + g_granularity - 1) / g_granularity;
    const size_t copy_slots_count = (g_user_end + g_page_size - 1) / g_page_size;
    const LPVOID slot_memory = own_reserve_locked(res_slots * sizeof(int32_t));
    const LPVOID entry_memory = own_reserve_locked(res_slots * sizeof(Reservation));
    const LPVOID copy_slot_memory = own_reserve_locked(copy_slots_count * sizeof(int32_t));
    const LPVOID scratch_memory = own_reserve_locked(kScratchBytes);
    if (slot_memory == nullptr || entry_memory == nullptr || copy_slot_memory == nullptr
        || scratch_memory == nullptr) {
        ReleaseSRWLockExclusive(&g_srw);
        return false;
    }
    g_res_slots = (int32_t*)slot_memory;
    std::memset(g_res_slots, 0xFF, res_slots * sizeof(int32_t));
    g_res_slot_count = res_slots;
    g_res = (Reservation*)entry_memory;
    g_res_capacity = res_slots;
    std::memset(g_res, 0, res_slots * sizeof(Reservation));
    g_copy_slots = (int32_t*)copy_slot_memory;
    std::memset(g_copy_slots, 0xFF, copy_slots_count * sizeof(int32_t));
    g_copy_slot_count = copy_slots_count;
    g_scratch = (char*)scratch_memory;
    g_scratch_capacity = kScratchBytes;
    g_prepared = true;
    ReleaseSRWLockExclusive(&g_srw);
    return true;
}

LPVOID WINAPI allocate(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) noexcept {
    const DWORD incoming_error = GetLastError();
    const bool watchable = g_prepared && !g_fatal && !g_finished
        && address == nullptr && bytes != 0
        && (type & (MEM_COMMIT | MEM_RESERVE)) != 0
        && (type & (MEM_PHYSICAL | MEM_LARGE_PAGES)) == 0;
    if (!watchable) {
        SetLastError(incoming_error);
        // The game's sites push flProtect as a fourth argument and the OS export this
        // build links against takes (lpAddress, dwSize, flAllocationType, flProtect),
        // so flProtect is forwarded verbatim.
        return ::VirtualAlloc(address, bytes, type, protect);
    }

    AcquireSRWLockExclusive(&g_srw);
    SetLastError(incoming_error);
    const DWORD watch_type = type | MEM_RESERVE | MEM_WRITE_WATCH;
    const LPVOID watched = ::VirtualAlloc(NULL, bytes, watch_type, protect);
    if (watched == nullptr) {
        const DWORD watch_error = GetLastError();
        // The game dereferences the returned backing pointer without a null check,
        // so one retry with the original arguments keeps startup alive; that result
        // is never registered as watched, and the latched error still fails the run.
        SetLastError(incoming_error);
        const LPVOID plain = ::VirtualAlloc(address, bytes, type, protect);
        const DWORD plain_error = GetLastError();
        latch_locked("watch_reservation_failed", watch_error);
        ReleaseSRWLockExclusive(&g_srw);
        SetLastError(plain_error);
        return plain;
    }
    const DWORD watched_error = GetLastError();
    const size_t rounded = (bytes + g_page_size - 1) & ~(g_page_size - 1);
    res_register_locked((uint64_t)(uintptr_t)watched, rounded, watch_type);
    ReleaseSRWLockExclusive(&g_srw);
    SetLastError(watched_error);
    return watched;
}

BOOL WINAPI release(LPVOID address, SIZE_T bytes, DWORD type) noexcept {
    const DWORD incoming_error = GetLastError();
    AcquireSRWLockExclusive(&g_srw);
    Reservation* entry = (address != nullptr)
                             ? reservation_at_locked((uint64_t)(uintptr_t)address) : nullptr;
    if (entry != nullptr && entry->watched && entry->live && !g_fatal && !g_finished && g_started) {
        // Harvest before the unmap: the write-watch dirty bits are lost when the
        // pages disappear, so work written and freed entirely between two loop
        // entries still reports.
        if (!harvest_reservation_locked(*entry, "release", g_journal_open)) {
            journal_error_locked();
        }
    }
    SetLastError(incoming_error);
    const BOOL freed = ::VirtualFree(address, bytes, type);
    const DWORD freed_error = GetLastError();
    if (address != nullptr && freed != FALSE && entry != nullptr) {
        res_note_locked(*entry, type == MEM_RELEASE ? "released" : "decommitted", false);
        res_retire_locked(*entry, type);
    }
    ReleaseSRWLockExclusive(&g_srw);
    SetLastError(freed_error);
    return freed;
}

bool start(const Config& config) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = start_locked(config);
    if (!ok) {
        journal_error_locked();
    }
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool begin_loop() noexcept {
    AcquireSRWLockExclusive(&g_srw);
    if (!g_prepared || !g_started || g_finished) {
        ReleaseSRWLockExclusive(&g_srw);
        return true;
    }
    const bool ok = begin_loop_locked();
    if (!ok) {
        journal_error_locked();
    }
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool session_started(uint32_t check_distance) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = (g_started && !g_finished && !g_fatal)
                        ? session_started_locked(check_distance) : !g_fatal;
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool advance(int frame, bool rolling_back, bool running_ahead) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = (g_started && !g_finished && !g_fatal)
                        ? advance_locked(frame, rolling_back, running_ahead) : !g_fatal;
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool save(int frame, uint32_t checksum) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = (g_started && !g_finished && !g_fatal)
                        ? save_locked(frame, checksum) : !g_fatal;
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool load(int frame, bool restored) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = (g_started && !g_finished && !g_fatal)
                        ? load_locked(frame, restored) : !g_fatal;
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool desync(int frame, uint32_t local, uint32_t remote, uint32_t count) noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = (g_started && !g_finished && !g_fatal)
                        ? desync_locked(frame, local, remote, count) : !g_fatal;
    if (!ok) {
        journal_error_locked();
    }
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool finish() noexcept {
    AcquireSRWLockExclusive(&g_srw);
    const bool ok = finish_locked();
    if (!ok) {
        journal_error_locked();
    }
    ReleaseSRWLockExclusive(&g_srw);
    return ok;
}

bool failed() noexcept {
    return g_fatal;
}

const char* error() noexcept {
    return g_error[0] != '\0' ? g_error : "none";
}

} // namespace netplay_pages
