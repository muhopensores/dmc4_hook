#include "NetplayTestHarness.hpp"
#include "utility/CommandLineArgs.hpp"
#include "utility/Module.hpp"
#include "utility/Patch.hpp"
#include "dev/netplay/MemoryPageTracker.hpp"

#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <cstdlib>

namespace netplay_test {

// The recorder gate waits long enough to cover a cold TTD attach (inject + initialize)
// while the stage callback stays blocked; a shorter wait races the recorder.
static constexpr DWORD kRecorderWaitMs = 120000;

static std::mutex g_lock;
static HANDLE g_log_file = INVALID_HANDLE_VALUE;
static std::string g_run_id;
static bool g_requested = false;      // -netplay-run/-netplay-log seen
static bool g_configured = false;     // arguments valid and events.log open
static bool g_gate_attempted = false; // the one-shot gate already ran
static bool g_gate_passed = false;    // recorder readiness confirmed
static bool g_finished = false;       // DONE already emitted; complete() is idempotent

// ---- memory page observer (opt-in) -------------------------------------------------
// The game reaches the OS allocators through two IAT cells, not through a handful of
// call sites. IDA: 0x00B850CC (VirtualAlloc) is called from 0x008BB561, 0x008BB6F8
// (the pool commit inside sub_8BB540 / sub_8BB6A0) and from 0x00B5A9C4, 0x00B5AA4F
// (the CRT heap reserver, which commits 1 MiB with MEM_COMMIT|MEM_RESERVE at
// 0x00B5A9B7); every one of them pushes the four-argument form. 0x00B850D0 (VirtualFree)
// is called from 0x008BB5C8, 0x008BB7DE, 0x008BBBD1E and from 0x00B5A880 / 0x00B5A8DB,
// which load the cell first (`mov esi, ds:VirtualFree` at 0x00B5A868, then `call esi`)
// with the three-argument form. Overwriting the cells instruments every reservation
// the main module makes, including the `call esi` pair that no byte patch at a call
// site would reach, and it has to happen before sMain::initMemory reserves the pools -
// so this runs from Main.cpp startup_proc, not from the mod hooks.
struct allocator_import {
    uintptr_t rva;              // IDB-relative address of the IAT cell
    bool alloc;                 // VirtualAlloc(lpAddress, dwSize, flAllocationType, flProtect)
    const char* name;           // import the cell must currently resolve to
};
static constexpr allocator_import k_allocator_imports[] = {
    {0x00B850CC, true, "VirtualAlloc"},
    {0x00B850D0, false, "VirtualFree"},
};
static constexpr uintptr_t k_preferred_image_base = 0x00400000;
static constexpr size_t k_max_page_ranges = 16;

static bool pages_requested = false;   // -netplay-page-watch seen
static bool pages_armed = false;       // tracker prepared + sites patched
static bool pages_started = false;     // sidecar journal open (gate passed)
static bool pages_failed = false;      // a page adapter failed; complete() outranks it
static std::vector<Patch::Ptr> page_patches;   // process-lifetime: never destroyed
static netplay_pages::Range page_ranges_storage[k_max_page_ranges];
static netplay_pages::Config page_config{};
static char page_journal_path[1024] = {0};
static char page_run_id[1024] = {0};

static bool valid_run_id(const char* run_id) {
    size_t length = 0;
    for (const char* p = run_id; *p; ++p, ++length) {
        const char c = *p;
        const bool alnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (!alnum && c != '-') {
            return false;
        }
    }
    return length >= 1 && length <= 64;
}

static std::string sanitize(std::string_view message) {
    std::string out;
    out.reserve(message.size());
    for (const char c : message) {
        out += (c == '\r' || c == '\n') ? ' ' : c;
    }
    return out;
}


// One protocol line: file write + flush first so the runner can tail it, then the same
// complete line through the debugger channel. Caller holds g_lock.
static void emit_locked(std::string_view body) {
    std::string line = "DMC4NETPLAY ";
    line += g_run_id;
    line += ' ';
    line += body;
    line += '\n';
    if (g_log_file != INVALID_HANDLE_VALUE) {
        WriteFile(g_log_file, line.c_str(), (DWORD)line.size(), NULL, NULL);
        FlushFileBuffers(g_log_file);
    }
    OutputDebugStringA(line.c_str());
}

static void emit_error_locked(std::string_view message) {
    emit_locked("ERROR message=" + sanitize(message));
}

// ---- memory page observer: the redirected allocator imports ------------------------
static size_t page_ranges_used = 0;
static size_t page_copy_limit_bytes = 512ull << 20;

// The wrappers' signatures match the imports exactly (IDA: every VirtualAlloc site -
// 0x008BB561, 0x008BB6F8, 0x00B5A9C4, 0x00B5AA4F - pushes lpAddress, dwSize,
// flAllocationType, flProtect; every VirtualFree site - 0x008BB5C8, 0x008BB7DE,
// 0x008BBBD1E and the two `call esi` sites - pushes lpAddress, dwSize, dwFreeType), and
// both are cdecl with callee cleanup, so overwriting the cell leaves each site's pushed
// arguments, cleanup and eax result untouched. scripts/netplay_allocator_import_probe.cpp
// proves it against both cell-read shapes: the `FF 15 <cell>` site and the CRT's
// `mov esi, ds:cell` + `call esi` site each reach the wrapper with their own arguments
// after only the cell changes.
extern "C" LPVOID WINAPI page_site_allocate(LPVOID address, SIZE_T bytes, DWORD type, DWORD protect) {
    return netplay_pages::allocate(address, bytes, type, protect);
}

extern "C" BOOL WINAPI page_site_release(LPVOID address, SIZE_T bytes, DWORD type) {
    return netplay_pages::release(address, bytes, type);
}

// Redirect: the IAT cell is overwritten with the wrapper's address, so every site that
// reads the cell (direct `FF 15 <cell>` or `mov esi, ds:...` + `call esi`) now calls the
// tracker adapter. Validation first: the cell's current value must resolve to the real
// kernel32 import - either it already holds the loaded import address, or it points at
// the module's `jmp [cell]` import thunk (FF 25), whose own cell holds it.
static uintptr_t resolve_cell_target(uintptr_t value, uintptr_t image_begin, uintptr_t image_end) {
    if (value >= image_begin && value < image_end
        && *(const BYTE*)value == 0xFF && *(const BYTE*)(value + 1) == 0x25) {
        return *(const DWORD*)(value + 2);
    }
    return value;
}

static bool redirect_allocator_imports(uintptr_t image_base) {
    const HMODULE image = GetModuleHandleA(NULL);
    if (image == NULL) {
        return false;
    }
    const auto span = utility::get_module_size(image);
    if (!span.has_value()) {
        return false;
    }
    const auto image_begin = (uintptr_t)image;
    const auto image_end = image_begin + *span;
    const HMODULE system = GetModuleHandleA("kernel32.dll");
    if (system == NULL) {
        return false;
    }
    const auto delta = image_base - k_preferred_image_base;
    for (const auto& entry : k_allocator_imports) {
        const auto cell = (const DWORD*)(entry.rva + delta);
        const auto expected = (uintptr_t)GetProcAddress(system, entry.name);
        const auto current = *cell;
        if (expected == 0 || resolve_cell_target(current, image_begin, image_end) != expected) {
            return false;
        }
    }
    for (const auto& entry : k_allocator_imports) {
        const auto target = entry.alloc ? (uintptr_t)&page_site_allocate : (uintptr_t)&page_site_release;
        const auto cell_address = entry.rva + delta;
        const std::vector<int16_t> bytes = {
            (int16_t)(target & 0xFF), (int16_t)((target >> 8) & 0xFF),
            (int16_t)((target >> 16) & 0xFF), (int16_t)((target >> 24) & 0xFF),
        };
        auto holder = Patch::create(cell_address, bytes, true);
        if (!holder) {
            for (auto& enabled : page_patches) {
                enabled->disable();
            }
            page_patches.clear();
            return false;
        }
        page_patches.push_back(std::move(holder));
    }
    return true;
}

// The sidecar opens only for a recorded run: a refused run leaves no journal at all,
// which the runner reports as a missing opt-in output rather than a clean run.
static void open_page_journal() {
    const HMODULE image = GetModuleHandleA(NULL);
    if (image == NULL) {
        pages_failed = true;
        return;
    }
    const auto span = utility::get_module_size(image);
    page_config.path = page_journal_path;
    page_config.run_id = g_run_id.c_str();
    page_config.image_base = (uintptr_t)image;
    page_config.image_size = span.has_value() ? *span : 0;
    page_config.ranges = page_ranges_used != 0 ? page_ranges_storage : nullptr;
    page_config.range_count = page_ranges_used;
    page_config.copy_limit_bytes = page_copy_limit_bytes;
    pages_started = netplay_pages::start(page_config);
    if (!pages_started) {
        pages_failed = true;
    }
}

void initialize_memory_pages_early() {
    if (pages_requested || pages_armed) {
        return;
    }
    if (!utility::check_argument("-netplay-page-watch")) {
        return;
    }
    pages_requested = true;
    utility::get_argument("-netplay-page-log", page_journal_path, sizeof(page_journal_path));
    utility::get_argument("-netplay-run", page_run_id, sizeof(page_run_id));
    const int copy_limit_mib = utility::get_argument("-netplay-page-copy-limit-mib");
    if (copy_limit_mib > 0) {
        page_copy_limit_bytes = (size_t)copy_limit_mib << 20;
    }
    char ranges_argument[2048] = {0};
    utility::get_argument("-netplay-page-ranges", ranges_argument, sizeof(ranges_argument));
    // Ranges arrive as `BEGIN:SIZE` pairs, page-aligned, comma separated.
    for (const char* cursor = ranges_argument; *cursor != '\0' && page_ranges_used < k_max_page_ranges; ) {
        const char* colon = nullptr;
        for (const char* p = cursor; *p != '\0' && *p != ','; ++p) {
            if (*p == ':') {
                colon = p;
                break;
            }
        }
        const char* end_of_pair = cursor;
        while (*end_of_pair != '\0' && *end_of_pair != ',') {
            ++end_of_pair;
        }
        if (colon != nullptr) {
            const uint64_t begin = std::strtoull(cursor, nullptr, 16);
            const uint64_t size = std::strtoull(colon + 1, nullptr, 16);
            if (begin != 0 && size != 0 && (begin & 0xFFF) == 0 && (size & 0xFFF) == 0
                && begin + size > begin) {
                page_ranges_storage[page_ranges_used++] = {begin, begin + size};
            }
        }
        cursor = (*end_of_pair == ',') ? end_of_pair + 1 : end_of_pair;
    }
    if (page_run_id[0] == '\0' || page_journal_path[0] == '\0' || !valid_run_id(page_run_id)) {
        std::lock_guard<std::mutex> lock(g_lock);
        emit_error_locked("page-watch run id or journal path is missing or truncated; "
            "leaving the game's allocator uninstrumented");
        return;
    }
    if (!netplay_pages::prepare()) {
        std::lock_guard<std::mutex> lock(g_lock);
        emit_error_locked("memory page observer could not prepare; "
            "leaving the game's allocator uninstrumented");
        return;
    }
    const HMODULE image = GetModuleHandleA(NULL);
    if (image == NULL || !redirect_allocator_imports((uintptr_t)image)) {
        std::lock_guard<std::mutex> lock(g_lock);
        emit_error_locked("allocator import cells did not validate; "
            "leaving the game's allocator uninstrumented");
        return;
    }
    pages_armed = true;
}

void page_loop_begin() {
    if (!pages_started || pages_failed) {
        return;
    }
    pages_failed = !netplay_pages::begin_loop();
}

void page_session_started(int check_distance) {
    if (!pages_started || pages_failed) {
        return;
    }
    pages_failed = !netplay_pages::session_started((uint32_t)check_distance);
}

void page_advance(int frame, bool rolling_back, bool running_ahead) {
    if (!pages_started || pages_failed) {
        return;
    }
    pages_failed = !netplay_pages::advance(frame, rolling_back, running_ahead);
}

void page_saved(int frame, uint32_t checksum) {
    if (!pages_started || pages_failed) {
        return;
    }
    pages_failed = !netplay_pages::save(frame, checksum);
}

void page_loaded(int frame, bool restored) {
    if (!pages_started || pages_failed) {
        return;
    }
    pages_failed = !netplay_pages::load(frame, restored);
}

void page_failed() {
    pages_failed = true;
}

void configure_from_command_line() {
    char run_id[1024] = {0};
    char log_path[1024] = {0};
    const bool has_run = utility::get_argument("-netplay-run", run_id, sizeof(run_id));
    const bool has_log = utility::get_argument("-netplay-log", log_path, sizeof(log_path));
    if (!has_run && !has_log) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_lock);
    g_requested = true;
    if (!has_run || !has_log || !valid_run_id(run_id)) {
        return;
    }
    g_run_id = run_id;
    g_log_file = CreateFileA(log_path, GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | CREATE_ALWAYS, NULL,
        CREATE_ALWAYS, 0, NULL);
    if (g_log_file == INVALID_HANDLE_VALUE) {
        return;
    }
    g_configured = true;
}

void log(std::string_view message) {
    if (!g_requested) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_lock);
    emit_locked("LOG " + sanitize(message));
}

void desync(int frame, uint32_t local, uint32_t remote, uint32_t count) {
    if (!g_requested) {
        return;
    }
    if (pages_started && !pages_failed) {
        pages_failed = !netplay_pages::desync(frame, local, remote, count);
    }
    char body[128];
    std::snprintf(body, sizeof(body), "DESYNC frame=%d local=%08X remote=%08X count=%u",
        frame, local, remote, count);
    std::lock_guard<std::mutex> lock(g_lock);
    emit_locked(body);
}

void complete(std::string_view reason, uint32_t desyncs) {
    if (!g_requested) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_lock);
    if (g_finished) {
        return;
    }
    g_finished = true;
    std::string reason_text(reason);
    // A page observer that was asked for but refused (unvalidated call sites) or that
    // reported a failure mid-run outranks the caller's reason: the runner must never
    // read a desync/stage_end DONE line from a run whose page tracking broke.
    if (pages_failed || (pages_requested && !pages_armed)) {
        reason_text = "harness_error";
    }
    if (pages_started && !netplay_pages::finish()) {
        pages_failed = true;
        reason_text = "harness_error";
    }
    char body[128];
    std::snprintf(body, sizeof(body), "DONE reason=%s desyncs=%u", reason_text.c_str(), desyncs);
    emit_locked(body);
}

bool wait_for_recorder() {
    if (!g_requested) {
        return true;
    }
    HANDLE stage = NULL;
    HANDLE record = NULL;
    {
        std::lock_guard<std::mutex> lock(g_lock);
        if (g_gate_attempted) {
            return g_gate_passed;
        }
        g_gate_attempted = true;
        if (!g_configured) {
            emit_error_locked("harness arguments were invalid or events.log could not be created; "
                "refusing to run an unrecorded test");
            return false;
        }
        // The runner pre-created both manual-reset events; the game only opens them.
        const std::string stage_name = "dmc4-netplay-stage-" + g_run_id;
        const std::string record_name = "dmc4-netplay-record-" + g_run_id;
        stage = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, stage_name.c_str());
        record = OpenEventA(SYNCHRONIZE | EVENT_MODIFY_STATE, FALSE, record_name.c_str());
        if (!stage || !record) {
            // kernel32 exports no CloseEvent on this OS build: the opened handles
            // are process-local and release when the game exits.
            emit_error_locked("could not open the recorder handshake events; refusing to run an unrecorded test");
            return false;
        }
        char body[64];
        std::snprintf(body, sizeof(body), "STAGE_WAIT pid=%u",
            (unsigned)GetProcessId(GetCurrentProcess()));
        emit_locked(body);
        if (SetEvent(stage) == FALSE) {
            emit_error_locked("SetEvent on the stage event failed");
            return false;
        }
    }
    // Event waiting, not a busy loop; the stage callback stays blocked here.
    const DWORD waited = WaitForSingleObjectEx(record, kRecorderWaitMs, FALSE);
    {
        std::lock_guard<std::mutex> lock(g_lock);
        if (waited != WAIT_OBJECT_0) {
            emit_error_locked(waited == WAIT_TIMEOUT
                ? "recorder handshake timed out after 120000 ms"
                : "WaitForSingleObjectEx on the record event failed");
            return false;
        }
        g_gate_passed = true;
        if (pages_armed && !pages_started) {
            open_page_journal();
        }
        emit_locked("RECORDING");
        return true;
    }
}

} // namespace netplay_test
