#include "FrameHooks.hpp"
#include "../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <atomic>
#include <cstring>
#include <memory>
#include <mutex>
#include <vector>

namespace frame_hooks {
namespace {

constexpr uintptr_t kFrameEndSite = 0x8AF03F;   // sDevil4Main::move, after call sCollisionGame::checkAttackMain
constexpr uint8_t kFrameEndBytes[6] = {0x8B, 0x8D, 0x64, 0x03, 0x01, 0x00};   // mov ecx, [ebp+0x10364]

struct Entry {
    int id;
    Point point;
    Fn fn;
    void* user;
    const char* owner;
};

std::mutex g_mutex;
std::vector<Entry> g_entries;
int g_next_id = 1;
std::atomic<uint64_t> g_frames{0};
std::unique_ptr<FunctionHook> g_hook_frame_end;
bool g_installed = false;
std::atomic<bool> g_ticks_driven{false};
int g_tick_frame = -1;
uintptr_t g_ret_frame_end = 0;

void run(Point point) {
    Entry list[64];
    int n = 0;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const Entry& e : g_entries)
            if (e.point == point && n < 64) list[n++] = e;
    }
    for (int i = 0; i < n; ++i) list[i].fn(list[i].user);   // each owner guards its own callbacks (plugins: SEH)
}

void __stdcall on_frame_end() {
    ++g_frames;
    run(FRAME_END);
}

__declspec(naked) void detour_frame_end() {
    __asm {
        pushad
        call on_frame_end
        popad
        mov ecx, [ebp+0x10364]
        jmp dword ptr [g_ret_frame_end]
    }
}

} // namespace

bool install() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_installed) return true;
    if (std::memcmp(reinterpret_cast<const void*>(kFrameEndSite), kFrameEndBytes, sizeof kFrameEndBytes) != 0) {
        spdlog::error("FrameHooks: unexpected code at 0x{:X}; frame callbacks won't run", kFrameEndSite);
        return false;
    }
    g_hook_frame_end = std::make_unique<FunctionHook>(kFrameEndSite, reinterpret_cast<uintptr_t>(&detour_frame_end));
    if (!g_hook_frame_end->create()) {
        spdlog::error("FrameHooks: failed to hook 0x{:X}", kFrameEndSite);
        g_hook_frame_end.reset();
        return false;
    }
    g_ret_frame_end = kFrameEndSite + sizeof kFrameEndBytes;
    g_installed = true;
    return true;
}

int add(Point point, Fn fn, void* user, const char* owner) {
    if (!fn || point < 0 || point >= POINT_COUNT) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    const int id = g_next_id++;
    g_entries.push_back({id, point, fn, user, owner ? owner : "?"});
    return id;
}

void frame_end() {
    on_frame_end();
}

void tick_start(int frame) {
    g_tick_frame = frame;
    run(TICK_START);
}

int tick_frame() {
    return g_ticks_driven.load() ? g_tick_frame : -1;
}

void set_ticks_driven(bool driven) {
    g_ticks_driven = driven;
}

bool ticks_driven() {
    return g_ticks_driven.load();
}

void remove(int id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    for (size_t i = 0; i < g_entries.size(); ++i)
        if (g_entries[i].id == id) { g_entries.erase(g_entries.begin() + i); return; }
}

uint64_t frames() { return g_frames.load(); }

} // namespace frame_hooks
