#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/FrameHooks.hpp"
#include "../../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <cctype>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

// S5, Albam link v1 (tools.md "Albam link, v1"; agreed with albam-6b 2026-11-18): live .sdl keys and the scheduler
// clock. RE: Vibed/RE/sdl_scheduler_runtime.md "Live editing".
//
// - Every live scheduler (uScheduler, 0x3C) passes uScheduler::move (0x90A850) once a tick, whoever owns it (room,
//   moving floor, enemy, title...), sometimes on a sMain job thread: an entry hook records it (under a mutex).
// - A loaded .sdl is one file image at rScheduler+0x60, fixed up in place: header +0x10 name area and per track
//   name / timing / data offsets become pointers (types 2 / 3 dti -> MtDTI*); key timing words and int / vector /
//   float / bool values stay raw and are read at every re-key, so same-size writes there work. prev = -1 re-keys.
// - Patches and clock writes run as game-thread jobs at the frame hook, between ticks.

namespace debug_server {
namespace {

using namespace mem;

constexpr uintptr_t kSchedulerMove = 0x90A850;   // uScheduler::move, thiscall
constexpr uintptr_t kFrame = 0x1C;               // uScheduler: float current frame
constexpr uintptr_t kPrev = 0x20;                // float previous frame (-1 = re-key next tick)
constexpr uintptr_t kSpeed = 0x24;               // float
constexpr uintptr_t kPause = 0x28;               // u8, default 1
constexpr uintptr_t kLoop = 0x29;                // u8, default 1
constexpr uintptr_t kResource = 0x38;            // rScheduler*
constexpr uintptr_t kImage = 0x60;               // rScheduler: the file image
constexpr uintptr_t kPath = 0x04;                // cResource: char[64]
constexpr uintptr_t kResourceSize = 0x50;        // cResource: m_size = the file size (blade01: 2078 = on disk)

// ---- load generation ----

uint32_t g_load_gen = 1;
uintptr_t g_last_layers[3] = {};

// ---- live schedulers (the move hook) ----

std::mutex g_mutex;
std::unordered_map<uintptr_t, uint64_t> g_seen;   // uScheduler -> frame_hooks::frames() at its last move
std::unique_ptr<FunctionHook> g_hook;
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

struct Live {
    uintptr_t id, rsrc;
    uint64_t seen;
};

// the schedulers seen since the last load (paused ones stop moving but stay listed while their object is a scheduler)
std::vector<Live> live() {
    std::vector<std::pair<uintptr_t, uint64_t>> seen;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        seen.assign(g_seen.begin(), g_seen.end());
    }
    std::vector<Live> out;
    for (const auto& [s, f] : seen)
        if (is_scheduler(s)) out.push_back({s, rd_ptr(s + kResource), f});
    std::sort(out.begin(), out.end(), [](const Live& a, const Live& b) { return a.id < b.id; });
    return out;
}

std::string path_of(uintptr_t rsrc) {
    const json p = safe_cstr(rsrc + kPath, 64);
    return p.is_string() ? p.get<std::string>() : std::string();
}

bool same_path(std::string a, std::string b) {
    for (auto* s : {&a, &b})
        for (char& c : *s) c = c == '/' ? '\\' : (char)std::tolower((unsigned char)c);
    return a == b;
}

// ---- file image ----

struct Track {
    int index;
    uint8_t type, prop_type;
    uint16_t keys;
    uint32_t parent;
    uintptr_t name, dti, timing, data;   // as in the image (pointers after the load's fix-ups)
};

struct Image {
    uintptr_t rsrc = 0, base = 0;
    uint16_t version = 0, count = 0;
    uint32_t frames_word = 0, marker = 0;
    uintptr_t name_area = 0;
    std::vector<Track> tracks;
};

Image read_image(uintptr_t rsrc) {
    Image im;
    im.rsrc = rsrc;
    im.base = rd_ptr(rsrc + kImage);
    uint8_t h[0x14];
    if (!im.base || !safe_read(im.base, h, sizeof(h)) || std::memcmp(h, "SDL\0", 4) != 0)
        throw ToolError("failed", "the file image of " + hex(rsrc) + " isn't readable / an SDL");
    std::memcpy(&im.version, h + 4, 2);
    std::memcpy(&im.count, h + 6, 2);
    std::memcpy(&im.frames_word, h + 8, 4);
    std::memcpy(&im.marker, h + 0xC, 4);
    std::memcpy(&im.name_area, h + 0x10, 4);
    for (int i = 0; i < im.count; ++i) {
        uint8_t t[0x18];
        if (!safe_read(im.base + 0x14 + 0x18 * i, t, sizeof(t))) throw ToolError("failed", "track table unreadable");
        Track k{};
        k.index = i;
        k.type = t[0];
        k.prop_type = t[1];
        std::memcpy(&k.keys, t + 2, 2);
        std::memcpy(&k.parent, t + 4, 4);
        uint32_t w[4];
        std::memcpy(w, t + 8, 16);
        k.name = w[0], k.dti = w[1], k.timing = w[2], k.data = w[3];
        im.tracks.push_back(k);
    }
    return im;
}

bool has_arrays(uint8_t type) { return type >= 6 && type <= 14; }
bool patchable(uint8_t type) { return type >= 6 && type <= 9; }

size_t value_size(uint8_t type) {   // per key, in the file (tools.md / dmc4xml): int 4, vector 16, float 4, bool 1
    switch (type) {
    case 6: case 8: return 4;
    case 7: return 16;
    case 9: return 1;
    default: return 0;
    }
}

json track_json(const Image& im, const Track& k) {
    json j{{"index", k.index}, {"type", k.type}, {"prop_type", k.prop_type}, {"keys", k.keys}, {"parent", k.parent}};
    // type 4 isn't fixed up at load (its name is still a name-area offset)
    const uintptr_t name = k.type == 4 ? im.name_area + k.name : k.name;
    j["name"] = safe_cstr(name, 128);
    if (k.type == 2 || k.type == 3) j["dti"] = k.dti ? json(dti_name(k.dti)) : json();
    else j["dti"] = hex(k.dti);
    j["timing_offset"] = has_arrays(k.type) ? json(k.timing - im.base) : json();
    j["data_offset"] = has_arrays(k.type) ? json(k.data - im.base) : json();
    return j;
}

uintptr_t find_file(const std::string& path) {
    for (const Live& l : live())
        if (same_path(path_of(l.rsrc), path)) return l.rsrc;
    throw ToolError("not_loaded", "no live scheduler plays '" + path + "' (sdl_list shows the loaded files)");
}

void check_gen(const json& a) {
    if (a.contains("load_gen") && !a["load_gen"].is_null() && a["load_gen"].get<long long>() != (long long)g_load_gen)
        throw ToolError("stale", "load_gen " + a["load_gen"].dump() + " isn't the current " + std::to_string(g_load_gen),
                        json{{"load_gen", g_load_gen}});
}

std::string arg_path(const json& a) {
    if (!a.contains("path") || !a["path"].is_string()) throw ToolError("bad_args", "path is required");
    return a["path"].get<std::string>();
}

// ---- tools ----

json scheduler_json(uintptr_t s, const Image* im) {
    float frame = 0, prev = 0, speed = 0;
    uint8_t pause = 0, loop = 0;
    rd(s + kFrame, frame);
    rd(s + kPrev, prev);
    rd(s + kSpeed, speed);
    rd(s + kPause, pause);
    rd(s + kLoop, loop);
    json j{{"id", hex(s)}, {"frame", frame}, {"prev", prev}, {"speed", speed}, {"pause", pause != 0}, {"loop", loop != 0}};
    if (im) j["length"] = im->frames_word & 0xFFFFFF;
    return j;
}

json sdl_list(const json&) {
    std::map<uintptr_t, json> files;
    const uint64_t now = frame_hooks::frames();
    for (const Live& l : live()) {
        json& f = files[l.rsrc];
        if (f.is_null()) {
            uint32_t size = 0;
            rd(l.rsrc + kResourceSize, size);
            f = json{{"path", path_of(l.rsrc)}, {"rsrc", hex(l.rsrc)}, {"size", size},
                     {"schedulers", json::array()}};
        }
        Image im;
        try { im = read_image(l.rsrc); } catch (const std::exception&) {}
        json s = scheduler_json(l.id, im.base ? &im : nullptr);
        s["frames_since_move"] = now - l.seen;
        f["schedulers"].push_back(s);
    }
    json out = json::array();
    for (auto& [r, f] : files) out.push_back(f);
    return json{{"load_gen", g_load_gen}, {"files", out}};
}

json sdl_layout(const json& a) {
    const uintptr_t rsrc = find_file(arg_path(a));
    const Image im = read_image(rsrc);
    uint32_t size = 0;
    rd(rsrc + kResourceSize, size);
    json tracks = json::array();
    for (const Track& k : im.tracks) tracks.push_back(track_json(im, k));
    return json{{"path", path_of(rsrc)}, {"load_gen", g_load_gen}, {"version", im.version}, {"track_count", im.count},
                {"frames_word", im.frames_word}, {"marker_track", im.marker},
                {"name_area_offset", im.name_area - im.base}, {"size", size}, {"tracks", tracks}};
}

struct Run {
    uint32_t offset;
    std::vector<uint8_t> bytes;
};

std::vector<uint8_t> parse_hex(const std::string& s, int run) {
    std::vector<uint8_t> out;
    int nib = -1;
    for (char c : s) {
        const int v = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                    : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
        if (v < 0) {
            if (c == ' ' || c == ',' || c == '\n' || c == '\t') continue;
            throw ToolError("bad_args", std::string("run ") + std::to_string(run) + ": not a hex digit '" + c + "'",
                            json{{"run", run}});
        }
        if (nib < 0) nib = v;
        else { out.push_back((uint8_t)(nib * 16 + v)); nib = -1; }
    }
    if (nib >= 0 || out.empty())
        throw ToolError("bad_args", "run " + std::to_string(run) + ": bytes = an even, non-zero number of hex digits",
                        json{{"run", run}});
    return out;
}

// where [off, off + n) lies: "" = inside a patchable array, else the refusal code
std::string classify(const Image& im, uint32_t off, size_t n) {
    const uint64_t end = (uint64_t)off + n;
    std::string best = "outside_value_array";
    for (const Track& k : im.tracks) {
        if (!has_arrays(k.type) || !k.keys) continue;
        const uint64_t t0 = k.timing - im.base, t1 = t0 + 4ull * k.keys;
        const size_t es = value_size(k.type);
        const uint64_t d0 = k.data - im.base;
        const uint64_t d1 = es ? d0 + es * k.keys : d0;   // other types: only "inside or not" is checked below
        auto starts_in = [&](uint64_t a0, uint64_t a1) { return off >= a0 && off < a1; };
        if (starts_in(t0, t1) || (es && starts_in(d0, d1))) {
            if (!patchable(k.type)) return "track_type";
            if ((starts_in(t0, t1) && end <= t1) || (es && starts_in(d0, d1) && end <= d1)) return "";
            best = "size";
        } else if (!patchable(k.type) && off == d0) {
            return "track_type";
        }
    }
    return best;
}

json sdl_patch(const json& a) {
    check_gen(a);
    const uintptr_t rsrc = find_file(arg_path(a));
    if (!a.contains("runs") || !a["runs"].is_array() || a["runs"].empty())
        throw ToolError("bad_args", "runs: [{offset, bytes}]");
    const Image im = read_image(rsrc);
    std::vector<Run> runs;
    for (size_t i = 0; i < a["runs"].size(); ++i) {
        const json& r = a["runs"][i];
        long long off = -1;
        if (!r.contains("offset") || !parse_number(r["offset"], off) || off < 0x14)
            throw ToolError("bad_args", "run " + std::to_string(i) + ": offset", json{{"run", i}});
        if (!r.contains("bytes") || !r["bytes"].is_string())
            throw ToolError("bad_args", "run " + std::to_string(i) + ": bytes (hex)", json{{"run", i}});
        Run run{(uint32_t)off, parse_hex(r["bytes"].get<std::string>(), (int)i)};
        const std::string why = classify(im, run.offset, run.bytes.size());
        if (!why.empty())
            throw ToolError(why, "run " + std::to_string(i) + " (offset " + std::to_string(run.offset) + ", " +
                                     std::to_string(run.bytes.size()) + " bytes): " + why,
                            json{{"run", i}});
        runs.push_back(std::move(run));
    }
    for (const Run& r : runs)   // all checked: apply all
        if (!safe_write(im.base + r.offset, r.bytes.data(), r.bytes.size()))
            throw ToolError("failed", "can't write at offset " + std::to_string(r.offset) + " (earlier runs applied)");
    int rekeyed = 0;
    const float minus_one = -1.0f;
    for (const Live& l : live())
        if (l.rsrc == rsrc && safe_write(l.id + kPrev, &minus_one, 4)) ++rekeyed;
    return json{{"applied", runs.size()}, {"schedulers_rekeyed", rekeyed}, {"load_gen", g_load_gen}};
}

json sdl_clock(const json& a) {
    check_gen(a);
    uintptr_t s = 0;
    if (a.contains("id")) {
        long long n = 0;
        if (!parse_number(a["id"], n)) throw ToolError("bad_args", "id: a scheduler id from sdl_list");
        s = (uintptr_t)n;
        bool found = false;
        for (const Live& l : live()) found |= l.id == s;
        if (!found) throw ToolError("not_found", "no live scheduler " + hex(s));
    } else if (a.contains("path")) {
        const uintptr_t rsrc = find_file(arg_path(a));
        for (const Live& l : live())
            if (l.rsrc == rsrc) { s = l.id; break; }
    } else {
        throw ToolError("bad_args", "id (or path) is required");
    }
    const bool set = a.contains("frame") || a.contains("pause") || a.contains("speed");
    if (set) {
        if (!dev_tools()) throw ToolError("dev_tools_off", "setting the clock changes the game: switch on Dev tools");
        if (a.contains("frame")) {
            const float f = a["frame"].get<float>(), minus_one = -1.0f;
            safe_write(s + kFrame, &f, 4);
            safe_write(s + kPrev, &minus_one, 4);   // re-key at that frame next tick, paused or not
        }
        if (a.contains("pause")) {
            const uint8_t p = a["pause"].get<bool>() ? 1 : 0;
            safe_write(s + kPause, &p, 1);
        }
        if (a.contains("speed")) {
            const float v = a["speed"].get<float>();
            safe_write(s + kSpeed, &v, 4);
        }
    }
    Image im;
    try { im = read_image(rd_ptr(s + kResource)); } catch (const std::exception&) {}
    json out = scheduler_json(s, im.base ? &im : nullptr);
    out["path"] = path_of(rd_ptr(s + kResource));
    out["load_gen"] = g_load_gen;
    return out;
}

json obj(std::initializer_list<std::pair<const char*, json>> props, std::vector<std::string> required = {}) {
    json p = json::object();
    for (const auto& [k, v] : props) p[k] = v;
    json s{{"type", "object"}, {"properties", p}};
    if (!required.empty()) s["required"] = required;
    return s;
}
json prop(const char* type, const char* description) { return json{{"type", type}, {"description", description}}; }

} // namespace

uint32_t load_gen() { return g_load_gen; }

void sdl_frame() {
    sArea* area = devil4_sdk::get_sArea();
    const uintptr_t now[3] = {area ? reinterpret_cast<uintptr_t>(area->aGamePtr) : 0,
                              area ? reinterpret_cast<uintptr_t>(area->currentMissionPtr) : 0,
                              area ? reinterpret_cast<uintptr_t>(area->aRoomPtr) : 0};
    if (std::memcmp(now, g_last_layers, sizeof(now)) != 0) {
        std::memcpy(g_last_layers, now, sizeof(now));
        ++g_load_gen;
        std::lock_guard<std::mutex> lock(g_mutex);
        g_seen.clear();   // ids are valid until load_gen changes
    }
}

void register_sdl() {
    if (!g_hook) {
        g_hook = std::make_unique<FunctionHook>(kSchedulerMove, &scheduler_move);
        if (g_hook->create()) g_original = g_hook->get_original<std::remove_pointer_t<Move>>();
        else spdlog::error("DebugServer: can't hook uScheduler::move (0x90A850): the sdl tools won't see schedulers");
    }
    add_tool({"sdl_list",
              "The loaded .sdl files (rScheduler) with the schedulers playing them: path, rsrc, and per scheduler id, "
              "frame, prev, length, loop, pause, speed. ids are valid until load_gen (also in status) changes. tools.md.",
              obj({}), true, sdl_list});
    add_tool({"sdl_layout",
              "A loaded .sdl's header and track table as the game holds it: version, track_count, frames_word, "
              "marker_track, name_area_offset, tracks [{index, type, prop_type, keys, parent, name, dti, timing_offset, "
              "data_offset}] (file-relative offsets). Errors: not_loaded. tools.md.",
              obj({{"path", prop("string", "e.g. scr\\st405\\sdl\\blade01")}}, {"path"}), true, sdl_layout});
    add_tool({"sdl_patch",
              "Same-size byte patches into a loaded .sdl: runs [{offset (file-relative), bytes (hex)}], each inside "
              "the timing or value array of an int / vector / float / bool track; all or none; then its schedulers "
              "re-key. Errors (data.code, data.run): not_loaded, stale, size, outside_value_array, track_type. tools.md.",
              obj({{"path", prop("string", "the file's path (sdl_list)")},
                   {"load_gen", prop("integer", "refuse with stale if it isn't current")},
                   {"runs", json{{"type", "array"}, {"description", "[{offset, bytes}]"}}}},
                  {"path", "runs"}),
              true, sdl_patch, true});
    add_tool({"sdl_clock",
              "A scheduler's clock: id (or path), and to set (needs Dev tools): frame (re-keys there next tick, even "
              "paused), pause, speed. Returns id, frame, prev, length, loop, pause, speed. Errors: not_found, stale.",
              obj({{"id", prop("string", "a scheduler id (sdl_list)")}, {"path", prop("string", "or a file: its first scheduler")},
                   {"frame", prop("number", "set the frame")}, {"pause", prop("boolean", "set the pause")},
                   {"speed", prop("number", "set the speed")}, {"load_gen", prop("integer", "refuse with stale if not current")}}),
              true, sdl_clock});
}

} // namespace debug_server
