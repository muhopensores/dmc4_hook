#include "Schedulers.hpp"
#include "FrameHooks.hpp"
#include "../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <windows.h>
#include <algorithm>
#include <cstring>
#include <memory>
#include <mutex>
#include <type_traits>
#include <unordered_map>

namespace schedulers {
namespace {

constexpr uintptr_t kSchedulerMove = 0x90A850;   // uScheduler::move, thiscall
constexpr uintptr_t kTrackCount = 0x18;          // uScheduler: u32
constexpr uintptr_t kStates = 0x2C;              // uScheduler: track states, 0x34 bytes each
constexpr uintptr_t kResource = 0x38;            // uScheduler: rScheduler*
constexpr uintptr_t kStateSize = 0x34;
constexpr uintptr_t kPath = 0x04;                // cResource: char[64]
constexpr uint8_t   kTrackUnit = 2, kTrackComment = 4;

// reads that fail instead of crashing (freed units, a scheduler gone mid-frame)
bool copy(uintptr_t a, void* dst, size_t n) {
    if (!a) return false;
    __try {
        std::memcpy(dst, reinterpret_cast<const void*>(a), n);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}
template <typename T> bool rd(uintptr_t a, T& v) { return copy(a, &v, sizeof(T)); }
uintptr_t rd_ptr(uintptr_t a) {
    uintptr_t v = 0;
    return rd(a, v) ? v : 0;
}
std::string cstr(uintptr_t a, size_t max = 128) {
    char buf[129] = {};
    if (!a || max > 128) return {};
    for (size_t i = 0; i < max; ++i) {   // byte by byte: a short string may end near the end of a page
        if (!rd(a + i, buf[i])) return {};
        if (!buf[i]) return std::string(buf, i);
    }
    return std::string(buf, max);
}

std::mutex g_mutex;
std::unordered_map<uintptr_t, uint64_t> g_seen;   // uScheduler -> frame_hooks::frames() at its last move
std::unique_ptr<FunctionHook> g_hook;
bool g_tried = false, g_ok = false;
using Move = uintptr_t(__fastcall*)(void* self, void* edx);
Move g_original = nullptr;

uintptr_t __fastcall scheduler_move(void* self, void* edx) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_seen[reinterpret_cast<uintptr_t>(self)] = frame_hooks::frames();
    }
    return g_original(self, edx);
}

bool is_scheduler(uintptr_t s) {   // still a uScheduler: its vtable's move slot is uScheduler::move, and it has a file
    const uintptr_t vt = rd_ptr(s);
    return vt && rd_ptr(vt + 0x18) == kSchedulerMove && rd_ptr(s + kResource);
}

} // namespace

bool install() {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_tried) return g_ok;
    g_tried = true;
    frame_hooks::install();
    g_hook = std::make_unique<FunctionHook>(kSchedulerMove, &scheduler_move);
    g_ok = g_hook->create();
    if (g_ok) g_original = g_hook->get_original<std::remove_pointer_t<Move>>();
    else spdlog::error("Schedulers: can't hook uScheduler::move (0x90A850): live schedulers won't be listed");
    return g_ok;
}

std::vector<Live> live() {
    std::vector<std::pair<uintptr_t, uint64_t>> seen;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        seen.assign(g_seen.begin(), g_seen.end());
    }
    std::vector<Live> out;
    std::vector<uintptr_t> gone;
    for (const auto& [s, f] : seen) {
        if (is_scheduler(s)) out.push_back({ s, rd_ptr(s + kResource), f });
        else gone.push_back(s);
    }
    if (!gone.empty()) {   // freed (or reused as something else): no longer listed
        std::lock_guard<std::mutex> lock(g_mutex);
        for (uintptr_t s : gone) g_seen.erase(s);
    }
    std::sort(out.begin(), out.end(), [](const Live& a, const Live& b) { return a.id < b.id; });
    return out;
}

void forget() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_seen.clear();
}

std::string path_of(uintptr_t rsrc) { return rsrc ? cstr(rsrc + kPath, 64) : std::string(); }

bool Owner::keys(const char* property) const {
    for (const Track& t : keyed)
        if (t.name == property) return true;
    return false;
}

bool owner_of(uintptr_t unit, Owner& out) {
    out = Owner{};
    if (!unit) return false;
    for (const Live& l : live()) {
        uint32_t count = 0;
        const uintptr_t states = rd_ptr(l.id + kStates);
        if (!states || !rd(l.id + kTrackCount, count) || count > 4096) continue;
        int made = -1;
        for (uint32_t k = 0; k < count && made < 0; ++k) {
            uint8_t type = 0;
            if (rd(states + k * kStateSize + 4, type) && type == kTrackUnit && rd_ptr(states + k * kStateSize + 8) == unit)
                made = (int)k;
        }
        if (made < 0) continue;
        out.scheduler = l.id;
        out.rsrc = l.rsrc;
        out.path = path_of(l.rsrc);
        const uintptr_t mt = rd_ptr(states + made * kStateSize);
        out.unit = Track{ made, kTrackUnit, cstr(rd_ptr(mt + 8)) };
        // the unit's property tracks: bound to it, or children of its track in the file
        for (uint32_t k = 0; k < count; ++k) {
            const uintptr_t st = states + k * kStateSize;
            uint8_t type = 0;
            uint32_t parent = 0;
            const uintptr_t track = rd_ptr(st);
            if ((int)k == made || !track || !rd(st + 4, type) || type < 5 || type == kTrackComment || !rd(track + 4, parent)) continue;
            if (rd_ptr(st + 8) != unit && parent != (uint32_t)made) continue;
            if (type == 5) continue;   // an object member (mCnsChain...), not a value
            out.keyed.push_back(Track{ (int)k, type, cstr(rd_ptr(track + 8)) });
        }
        return true;
    }
    return false;
}

} // namespace schedulers
