#pragma once
// Opt-in, Windows-only memory-page observer for the netplay test harness.
//
// Sits between consecutive `devil4_main_move_loop` entries: it reports pages the OS
// saw written inside game allocator reservations the harness enabled
// MEM_WRITE_WATCH on (via `allocate`/`release`, which the game's own backing
// VirtualAlloc/VirtualFree callsites are patched onto), and falls back to a fresh
// VirtualQuery + ReadProcessMemory byte comparison for memory the OS cannot watch.
//
// Deliberately independent of the game SDK, GekkoNet, BattleSnapshot, the framework
// PCH and spdlog so a standalone native fixture can compile this implementation.
// No exceptions: every entry point reports failure through `false` plus a latched
// `error()` code, readable through `failed()`/`error()`.
#include <cstddef>
#include <cstdint>
#include <windows.h>

namespace netplay_pages {
    struct Range {
        uint64_t begin; // inclusive
        uint64_t end;   // exclusive; supports the 0x100000000 boundary
    };

    struct Config {
        const char* path;             // sidecar journal path (absolute)
        const char* run_id;           // validated harness run id
        uintptr_t image_base;         // main executable image base
        size_t image_size;            // main executable mapped size
        const Range* ranges;          // optional comparison filter, may be null
        size_t range_count;           // 0 means broad coverage
        size_t copy_limit_bytes;      // retained fallback before-image budget
    };

    bool prepare() noexcept;
    LPVOID WINAPI allocate(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) noexcept;
    BOOL WINAPI release(LPVOID address, SIZE_T bytes, DWORD type) noexcept;
    bool start(const Config& config) noexcept;
    bool begin_loop() noexcept;
    bool session_started(uint32_t check_distance) noexcept;
    bool advance(int frame, bool rolling_back, bool running_ahead) noexcept;
    bool save(int frame, uint32_t checksum) noexcept;
    bool load(int frame, bool restored) noexcept;
    bool desync(int frame, uint32_t local, uint32_t remote, uint32_t count) noexcept;
    bool finish() noexcept;
    bool failed() noexcept;
    const char* error() noexcept;
}
