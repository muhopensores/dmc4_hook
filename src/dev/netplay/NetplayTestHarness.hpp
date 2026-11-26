#pragma once
// Opt-in, Windows-only netplay test harness: a named-event recorder handshake plus a
// share-readable desync event stream. See TTD_DESYNC_HARNESS.md for the protocol.
//
// Without -netplay-run/-netplay-log on the game command line every entry point is a
// no-op and wait_for_recorder() returns true, so normal launches are unaffected.
#include <cstdint>
#include <string_view>

namespace netplay_test {
    void configure_from_command_line();
    bool wait_for_recorder();
    void log(std::string_view message);
    void desync(int frame, uint32_t local, uint32_t remote, uint32_t count);
    void complete(std::string_view reason, uint32_t desyncs);
    // Memory page observer (`dev/netplay/MemoryPageTracker.hpp`) adapters. `initialize_memory_pages_early()`
    // belongs to the DLL-load bootstrap: the patched allocator sites must exist before
    // sMain::initMemory reserves the pools, and they only exist for an opted-in run.
    void initialize_memory_pages_early();
    void page_loop_begin();
    void page_session_started(int check_distance);
    void page_advance(int frame, bool rolling_back, bool running_ahead);
    void page_saved(int frame, uint32_t checksum);
    void page_loaded(int frame, bool restored);
    void page_failed();
}
