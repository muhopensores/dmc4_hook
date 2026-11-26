// Standalone native proof for the memory-page observer: it drives the real core
// (MemoryPageTracker.cpp) and the real Win32 APIs in a fresh process, writes the
// sidecar `pages.jsonl` plus a small `manifest.json` of the fixture's own
// addresses/reservation ids, and exits nonzero only when a fixture setup or API
// operation fails unexpectedly.
//
//   netplay_memory_pages_smoke <output-directory> <normal|copy-limit>
//
// Free of the game SDK/PCH/spdlog: it compiles beside MemoryPageTracker.cpp alone.
// Stores that must survive optimization go through `volatile` pointers, so same-value
// and write-then-restore stores stay observable to the OS write watch.
#include "dev/netplay/MemoryPageTracker.hpp"

#include <windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

static size_t g_page_size;
static size_t g_granularity;

struct FixtureBlock {
    const char* name;
    bool through_core; // allocated/released through netplay_pages
    uint64_t base;
    size_t bytes;
};
static std::vector<FixtureBlock> g_blocks;

static void store_byte(void* address, size_t offset, unsigned char value) {
    *(volatile unsigned char*)((char*)address + offset) = value;
}

static void note(const char* name, bool through_core, void* base, size_t bytes) {
    g_blocks.push_back({name, through_core, (uint64_t)(uintptr_t)base, bytes});
}

static bool set_protection(void* address, size_t bytes, DWORD protect) {
    DWORD old = 0;
    return VirtualProtect(address, bytes, protect, &old) != FALSE;
}

// Private RW committed ranges the process sees; the difference across prepare() is
// exactly what the core reserved for itself, and broad sampling must never report
// any of it as payload.
static bool enumerate_private(std::vector<uint64_t>& out) {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    MEMORY_BASIC_INFORMATION region{};
    uint64_t cursor = (uint64_t)(uintptr_t)info.lpMinimumApplicationAddress;
    const uint64_t stop = (uint64_t)(uintptr_t)info.lpMaximumApplicationAddress + 1;
    while (cursor < stop) {
        if (VirtualQuery((LPCVOID)(uintptr_t)cursor, &region, sizeof(region))
            != sizeof(region)) {
            return false;
        }
        const uint64_t begin = (uint64_t)(uintptr_t)region.BaseAddress;
        const uint64_t end = begin + (uint64_t)region.RegionSize;
        if (region.Type == MEM_PRIVATE && region.State == MEM_COMMIT) {
            out.push_back(begin);
            out.push_back(end);
        }
        cursor = (end <= cursor) ? (cursor + info.dwAllocationGranularity) : end;
    }
    return true;
}

static bool write_manifest(const char* path, const char* case_name, const char* expected_error,
                           const char* stack_base, const std::vector<uint64_t>& core_ranges) {
    FILE* f = nullptr;
    if (fopen_s(&f, path, "wb") != 0 || f == nullptr) {
        std::fprintf(stderr, "fixture: could not write manifest %s\n", path);
        return false;
    }
    std::string expected_token = (expected_error[0] != '\0')
                                     ? ("\"" + std::string(expected_error) + "\"")
                                     : std::string("null");
    std::fprintf(f, "{\"case\":\"%s\",\"expected_error\":%s,\"page_size\":%zu,"
                   "\"granularity\":%zu,\"stack_base\":\"%s\",\"blocks\":[",
                   case_name, expected_token.c_str(),
                   g_page_size, g_granularity, stack_base);
    bool first = true;
    for (const FixtureBlock& b : g_blocks) {
        std::fprintf(f, "%s{\"name\":\"%s\",\"through_core\":%s,\"base\":\"%08llX\",\"bytes\":%llu}",
                     first ? "" : ",", b.name, b.through_core ? "true" : "false",
                     (unsigned long long)b.base, (unsigned long long)b.bytes);
        first = false;
    }
    std::fprintf(f, "],\"core_ranges\":[");
    first = true;
    for (size_t i = 0; i + 1 < core_ranges.size(); i += 2) {
        std::fprintf(f, "%s\"%08llX:%08llX\"", first ? "" : ",",
                     (unsigned long long)core_ranges[i], (unsigned long long)core_ranges[i + 1]);
        first = false;
    }
    std::fprintf(f, "]}\n");
    std::fclose(f);
    return true;
}

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "usage: netplay_memory_pages_smoke <output-dir> <normal|copy-limit>\n");
        return 2;
    }
    const std::string out_dir = argv[1];
    const std::string mode = argv[2];
    const bool normal = mode == "normal";
    if (!normal && mode != "copy-limit") {
        std::fprintf(stderr, "fixture: unknown mode %s\n", mode.c_str());
        return 2;
    }

    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    g_page_size = info.dwPageSize;
    g_granularity = info.dwAllocationGranularity;
    if (g_page_size == 0 || g_granularity == 0) {
        return 2;
    }

    std::vector<uint64_t> before;
    if (!enumerate_private(before)) {
        std::fprintf(stderr, "fixture: pre-prepare enumeration failed\n");
        return 2;
    }
    if (!netplay_pages::prepare()) {
        std::fprintf(stderr, "fixture: prepare failed: %s\n", netplay_pages::error());
        return 2;
    }
    std::vector<uint64_t> after;
    if (!enumerate_private(after)) {
        std::fprintf(stderr, "fixture: post-prepare enumeration failed\n");
        return 2;
    }
    std::vector<uint64_t> core_ranges;
    for (size_t i = 0; i + 1 < after.size(); i += 2) {
        bool seen = false;
        for (size_t j = 0; j + 1 < before.size(); j += 2) {
            if (after[i] == before[j] && after[i + 1] == before[j + 1]) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            core_ranges.push_back(after[i]);
            core_ranges.push_back(after[i + 1]);
        }
    }

    char journal_path[MAX_PATH];
    if (std::snprintf(journal_path, sizeof(journal_path), "%s\\pages.jsonl", out_dir.c_str())
        <= 0) {
        return 2;
    }
    char manifest_path[MAX_PATH];
    if (std::snprintf(manifest_path, sizeof(manifest_path), "%s\\manifest.json", out_dir.c_str())
        <= 0) {
        return 2;
    }

    HMODULE exe = GetModuleHandle(NULL);
    MEMORY_BASIC_INFORMATION head{};
    if (VirtualQuery((LPCVOID)exe, &head, sizeof(head)) != sizeof(head)) {
        std::fprintf(stderr, "fixture: could not query the main image\n");
        return 2;
    }
    // VirtualQuery reports one section; the manifest needs the whole loaded image so
    // the consumer can check the `image_rva` column against the module's real span.
    auto* dos = (PIMAGE_DOS_HEADER)(uintptr_t)exe;
    auto* nt = (PIMAGE_NT_HEADERS)((BYTE*)exe + dos->e_lfanew);
    const DWORD image_size = nt->OptionalHeader.SizeOfImage;

    char stack_probe{};
    MEMORY_BASIC_INFORMATION stack_region{};
    if (VirtualQuery(&stack_probe, &stack_region, sizeof(stack_region))
        != sizeof(stack_region)) {
        std::fprintf(stderr, "fixture: could not query the sampling stack\n");
        return 2;
    }
    char stack_base[9];
    std::snprintf(stack_base, sizeof(stack_base), "%08llX",
                  (unsigned long long)(uint64_t)(uintptr_t)stack_region.AllocationBase);

    netplay_pages::Range ranges[32];
    size_t range_count = 0;
    netplay_pages::Config config{};
    config.path = journal_path;
    config.run_id = normal ? "fixture-normal" : "fixture-copy-limit";
    config.image_base = (uintptr_t)exe;
    config.image_size = image_size;
    config.ranges = ranges;
    config.range_count = range_count;
    config.copy_limit_bytes = normal ? (size_t)512u << 20 : g_page_size;

    if (normal) {
        // Watched block A: four pages through the core wrapper.
        void* watched_a = netplay_pages::allocate(NULL, 4 * g_page_size, MEM_COMMIT | MEM_RESERVE,
                                                  PAGE_READWRITE);
        if (watched_a == nullptr) {
            std::fprintf(stderr, "fixture: watched_a allocation failed\n");
            return 2;
        }
        note("watched_a", true, watched_a, 4 * g_page_size);

        // Split block, protected before the baseline:
        //   p0 RW, p1 read-only, p2 RW, p3 no-access, p4 RW, p5..p6 PAGE_GUARD.
        // A PAGE_GUARD range guards the page that precedes it, so p4 faults when read
        // and p5/p6 carry the guard flag; p0/p2 are the RW pages on both sides of the
        // read-only split and p2/p4 straddle the no-access split.
        void* split = ::VirtualAlloc(NULL, 7 * g_page_size, MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
        if (split == nullptr) {
            std::fprintf(stderr, "fixture: split allocation failed\n");
            return 2;
        }
        note("split", false, split, 7 * g_page_size);
        if (!set_protection((LPVOID)((char*)split + 1 * g_page_size), g_page_size, PAGE_READONLY)
            || !set_protection((LPVOID)((char*)split + 3 * g_page_size), g_page_size, PAGE_NOACCESS)
            || !set_protection((LPVOID)((char*)split + 5 * g_page_size), 2 * g_page_size,
                               PAGE_READWRITE | PAGE_GUARD)) {
            std::fprintf(stderr, "fixture: VirtualProtect splits failed err=%lu\n", GetLastError());
            return 2;
        }

        // Ordinary block B: two pages for the byte-difference cases.
        void* ordinary_b = ::VirtualAlloc(NULL, 2 * g_page_size, MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE);
        if (ordinary_b == nullptr) {
            std::fprintf(stderr, "fixture: ordinary_b allocation failed\n");
            return 2;
        }
        note("ordinary_b", false, ordinary_b, 2 * g_page_size);

        // Ordinary block T: two pages; the tail page is decommitted and recommitted
        // across a boundary to prove coverage removal instead of a stale diff.
        void* tail = ::VirtualAlloc(NULL, 2 * g_page_size, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
        if (tail == nullptr) {
            std::fprintf(stderr, "fixture: tail allocation failed\n");
            return 2;
        }
        note("tail", false, tail, 2 * g_page_size);

        if (!netplay_pages::start(config)) {
            std::fprintf(stderr, "fixture: start failed: %s\n", netplay_pages::error());
            return 2;
        }
        if (!netplay_pages::begin_loop()) { // baseline: initialization writes discarded
            std::fprintf(stderr, "fixture: baseline failed: %s\n", netplay_pages::error());
            return 2;
        }

        // Interval [1,2): stores in every observed class.
        store_byte(watched_a, 17, 0x7B);                    // watched page 0, byte 17
        store_byte(watched_a, 1 * g_page_size, 0x55);       // watched page 1 ...
        store_byte(watched_a, 1 * g_page_size, 0x00);       // ... written then restored
        store_byte(ordinary_b, 17, 0x31);                   // ordinary page 0, offset 17
        store_byte(ordinary_b, 1 * g_page_size, 0x99);      // ordinary page 1 ...
        store_byte(ordinary_b, 1 * g_page_size, 0x00);      // ... written then restored
        store_byte(split, 0 * g_page_size, 0x11);           // RW left of read-only split
        store_byte(split, 2 * g_page_size, 0x12);           // RW right of both splits
        store_byte(tail, 0, 0x01);
        if (!netplay_pages::begin_loop()) { // closes interval [1,2)
            std::fprintf(stderr, "fixture: interval 2 failed: %s\n", netplay_pages::error());
            return 2;
        }

        // Interval [2,3): a watched block written and released inside the interval, a
        // plain re-reservation of the freed address, a decommitted tail page, a same-value
        // store, and a protection change on an already-copied page.
        void* watched_c = netplay_pages::allocate(NULL, 2 * g_page_size, MEM_COMMIT | MEM_RESERVE,
                                                  PAGE_READWRITE);
        if (watched_c == nullptr) {
            std::fprintf(stderr, "fixture: watched_c allocation failed\n");
            return 2;
        }
        note("watched_c", true, watched_c, 2 * g_page_size);
        store_byte(watched_c, 0, 0x7E);
        if (netplay_pages::release(watched_c, 2 * g_page_size, MEM_DECOMMIT) == FALSE) {
            std::fprintf(stderr, "fixture: decommit of the watched block failed\n");
            return 2;
        }
        if (netplay_pages::release(watched_c, 0, MEM_RELEASE) == FALSE) {
            std::fprintf(stderr, "fixture: release of the watched block failed\n");
            return 2;
        }
        if (::VirtualAlloc(watched_c, 2 * g_page_size, MEM_RESERVE | MEM_COMMIT,
                           PAGE_READWRITE) == nullptr) {
            std::fprintf(stderr, "fixture: re-reserving the released address failed\n");
            return 2;
        }
        note("rereserved", false, watched_c, 2 * g_page_size);
        if (::VirtualFree((LPVOID)((char*)tail + 1 * g_page_size), g_page_size,
                          MEM_DECOMMIT) == FALSE) {
            std::fprintf(stderr, "fixture: decommit of the tail page failed\n");
            return 2;
        }
        store_byte(watched_a, 17, 0x7B); // same value: a write observation, not a change
        // Execute-read-write is a comparison-eligible protection: changing the
        // protection of an already-copied page must retire the stale copy as
        // `remapped`, not diff it against bytes captured under the old mapping.
        if (!set_protection((LPVOID)((char*)ordinary_b + 1 * g_page_size), g_page_size,
                            PAGE_EXECUTE_READWRITE)) {
            std::fprintf(stderr, "fixture: execute remap failed err=%lu\n", GetLastError());
            return 2;
        }
        if (!netplay_pages::begin_loop()) { // closes interval [2,3)
            std::fprintf(stderr, "fixture: interval 3 failed: %s\n", netplay_pages::error());
            return 2;
        }

        // Interval [3,4): the re-reserved address byte-compares; the recommitted tail
        // page takes a fresh baseline instead of a diff against stale bytes; the
        // protection change is undone, which remaps the page again.
        store_byte(watched_c, 0, 0x81);
        if (::VirtualAlloc((LPVOID)((char*)tail + 1 * g_page_size), g_page_size,
                           MEM_COMMIT, PAGE_READWRITE) == nullptr) {
            std::fprintf(stderr, "fixture: recommmit of the tail page failed\n");
            return 2;
        }
        if (!set_protection((LPVOID)((char*)ordinary_b + 1 * g_page_size), g_page_size,
                            PAGE_READWRITE)) {
            std::fprintf(stderr, "fixture: restoring the copy protection failed err=%lu\n",
                         GetLastError());
            return 2;
        }
        if (!netplay_pages::begin_loop()) { // closes interval [3,4)
            std::fprintf(stderr, "fixture: interval 4 failed: %s\n", netplay_pages::error());
            return 2;
        }

        // Interval [4,5): one more single-byte change plus the Gekko-style context.
        store_byte(ordinary_b, 17, 0x32);
        store_byte(tail, 1 * g_page_size, 0x44);
        if (!netplay_pages::session_started(8u) || !netplay_pages::advance(11, true, false)
            || !netplay_pages::advance(12, false, true) || !netplay_pages::save(-1, 0xDEADBEEFu)
            || !netplay_pages::save(7, 0xDEADBEEFu) || !netplay_pages::load(4, true)
            || !netplay_pages::load(5, false)
            || !netplay_pages::desync(12, 0x11223344u, 0x55667788u, 1u)) {
            std::fprintf(stderr, "fixture: an event adapter failed\n");
            return 2;
        }
        if (!netplay_pages::begin_loop()) { // closes interval [4,5)
            std::fprintf(stderr, "fixture: interval 5 failed: %s\n", netplay_pages::error());
            return 2;
        }
        if (netplay_pages::failed()) {
            std::fprintf(stderr, "fixture: unexpected fatal latch %s\n", netplay_pages::error());
            return 2;
        }
        // Flush-at-boundary contract: everything written so far must already be on disk,
        // because a live run gets no clean finish() when the runner kills the game at its
        // timeout; an empty sidecar there is reported as a contract error. At this point
        // start_locked flushed the header/arena rows and begin_loop closed interval 5.
        if (GetFileAttributesA(journal_path) == INVALID_FILE_ATTRIBUTES) {
            std::fprintf(stderr, "fixture: journal missing before finish\n");
            return 2;
        }
        HANDLE opened = CreateFileA(journal_path, GENERIC_READ,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                    OPEN_EXISTING, 0, NULL);
        if (opened == INVALID_HANDLE_VALUE) {
            std::fprintf(stderr, "fixture: journal could not be opened before finish (win32 %lu)\n",
                         GetLastError());
            return 2;
        }
        LARGE_INTEGER bytes{};
        if (!GetFileSizeEx(opened, &bytes) || bytes.QuadPart < 128) {
            std::fprintf(stderr, "fixture: journal flush at boundary missing (%lld bytes)\n",
                         bytes.QuadPart);
            CloseHandle(opened);
            return 2;
        }
        CloseHandle(opened);
        if (!netplay_pages::finish()) { // partial done endpoint
            std::fprintf(stderr, "fixture: finish failed: %s\n", netplay_pages::error());
            return 2;
        }

        MEMORY_BASIC_INFORMATION guard_region{};
        if (VirtualQuery((LPCVOID)(uintptr_t)((char*)split + 5 * g_page_size), &guard_region,
                         sizeof(guard_region)) != sizeof(guard_region)
            || (guard_region.Protect & PAGE_GUARD) == 0) {
            std::fprintf(stderr, "fixture: the guard range lost its protection\n");
            return 2;
        }

        if (!write_manifest(manifest_path, "normal", "", stack_base, core_ranges)) {
            return 2;
        }
        // Post-finish releases: still valid, and silent.
        if (netplay_pages::release(watched_a, 4 * g_page_size, MEM_DECOMMIT) == FALSE
            || netplay_pages::release(watched_a, 0, MEM_RELEASE) == FALSE) {
            std::fprintf(stderr, "fixture: post-finish release of watched_a failed\n");
            return 2;
        }
        if (::VirtualFree(split, 0, MEM_RELEASE) == FALSE
            || ::VirtualFree(ordinary_b, 0, MEM_RELEASE) == FALSE
            || ::VirtualFree(tail, 0, MEM_RELEASE) == FALSE) {
            std::fprintf(stderr, "fixture: post-finish VirtualFree of ordinary blocks failed\n");
            return 2;
        }
        return 0;
    }

    // copy-limit: one watched page keeps the arena check alive, the configured range
    // restricts comparison to one ordinary two-page RW allocation, and the budget is
    // a single page: the initial capture must latch copy_limit explicitly.
    void* watched = netplay_pages::allocate(NULL, g_page_size, MEM_COMMIT | MEM_RESERVE,
                                            PAGE_READWRITE);
    if (watched == nullptr) {
        std::fprintf(stderr, "fixture: watched allocation failed\n");
        return 2;
    }
    note("watched", true, watched, g_page_size);
    void* ordinary = ::VirtualAlloc(NULL, 2 * g_page_size, MEM_RESERVE | MEM_COMMIT,
                                    PAGE_READWRITE);
    if (ordinary == nullptr) {
        std::fprintf(stderr, "fixture: ordinary allocation failed\n");
        return 2;
    }
    note("ordinary", false, ordinary, 2 * g_page_size);
    ranges[0] = {(uint64_t)(uintptr_t)ordinary,
                 (uint64_t)(uintptr_t)ordinary + 2 * g_page_size};
    range_count = 1;
    config.ranges = ranges;
    config.range_count = range_count;
    config.copy_limit_bytes = g_page_size;

    if (!netplay_pages::start(config)) {
        std::fprintf(stderr, "fixture: start failed: %s\n", netplay_pages::error());
        return 2;
    }
    if (netplay_pages::begin_loop()) {
        std::fprintf(stderr, "fixture: the copy limit did not stop diagnostics\n");
        return 2;
    }
    if (std::strcmp(netplay_pages::error(), "copy_limit") != 0) {
        std::fprintf(stderr, "fixture: unexpected latch %s\n", netplay_pages::error());
        return 2;
    }
    netplay_pages::finish(); // keeps the error record and the footer
    if (!write_manifest(manifest_path, "copy-limit", "copy_limit", stack_base, core_ranges)) {
        return 2;
    }
    if (::VirtualFree(ordinary, 0, MEM_RELEASE) == FALSE) {
        std::fprintf(stderr, "fixture: releasing the ordinary block failed\n");
        return 2;
    }
    netplay_pages::release(watched, g_page_size, MEM_DECOMMIT);
    netplay_pages::release(watched, 0, MEM_RELEASE);
    return 0;
}
