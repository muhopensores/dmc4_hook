#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../../mods/AreaJump.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/Menus.hpp"
#include "../../sdk/RoomClasses.hpp"
#include "../../sdk/MissionMenu.hpp"
#include <windows.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

// S2 tools (plan Vibed/GameMCP/README.md): write, call, enter_room, print_text, spawn, kill_unit. All change the game,
// so all are `dev` (the panel's Dev tools switch). Game thread.

namespace debug_server {
namespace {

using namespace mem;

json obj(std::initializer_list<std::pair<const char*, json>> props, std::vector<std::string> required = {}) {
    json p = json::object();
    for (const auto& [k, v] : props) p[k] = v;
    json s{{"type", "object"}, {"properties", p}};
    if (!required.empty()) s["required"] = required;
    return s;
}
json prop(const char* type, const char* description) { return json{{"type", type}, {"description", description}}; }
json any_of(const char* description) {
    return json{{"anyOf", json::array({json{{"type", "integer"}}, json{{"type", "string"}}})}, {"description", description}};
}
json address_props() {   // as `read`
    return json{{"address", any_of("number, hex string or root (sArea, sUnit, sMediator, sRender, room, player1-4)")},
                {"path", json{{"type", "array"}, {"items", any_of("offset")}, {"description", "pointers to follow"}}},
                {"offset", any_of("added after the path")}};
}

std::string bytes_hex(const uint8_t* b, size_t n) {
    static const char* d = "0123456789ABCDEF";
    std::string h;
    for (size_t i = 0; i < n; ++i) {
        if (i) h += ' ';
        h += d[b[i] >> 4];
        h += d[b[i] & 15];
    }
    return h;
}

std::vector<uint8_t> parse_hex(const std::string& s) {
    std::vector<uint8_t> out;
    int nib = -1;
    for (char c : s) {
        int v = (c >= '0' && c <= '9') ? c - '0' : (c >= 'a' && c <= 'f') ? c - 'a' + 10 : (c >= 'A' && c <= 'F') ? c - 'A' + 10 : -1;
        if (v < 0) {
            if (c == ' ' || c == ',' || c == '\n' || c == '\t') continue;
            throw std::runtime_error(std::string("not a hex digit: '") + c + "'");
        }
        if (nib < 0) nib = v;
        else { out.push_back((uint8_t)(nib * 16 + v)); nib = -1; }
    }
    if (nib >= 0) throw std::runtime_error("odd number of hex digits");
    return out;
}

// ---- write ----

json write_tool(const json& a) {
    json chain = json::array();
    const uintptr_t addr = resolve(a, &chain);
    if (!a.contains("value")) throw std::runtime_error("value is required");
    const std::string type = a.value("type", std::string("bytes"));
    std::vector<uint8_t> data;
    if (type == "bytes") {
        if (!a["value"].is_string()) throw std::runtime_error("bytes: value = a hex string (\"90 90\")");
        data = parse_hex(a["value"].get<std::string>());
        if (data.empty() || data.size() > 4096) throw std::runtime_error("1 to 4096 bytes");
    } else {
        const TypeInfo* t = type_named(type);
        if (!t) throw std::runtime_error("no type '" + type + "' (types: " + scalar_type_names() + ", bytes)");
        // an array of values for a scalar type writes consecutive elements; a vector type takes one value
        const bool many = a["value"].is_array() && t->kind != Kind::Vec3 && t->kind != Kind::Vec4 && t->kind != Kind::Mat4;
        const size_t count = many ? a["value"].size() : 1;
        if (count == 0 || count * t->size > 4096) throw std::runtime_error("1 to 4096 bytes");
        data.resize(count * t->size);
        if (!safe_read(addr, data.data(), data.size()))   // keeps the vec3 padding
            throw std::runtime_error("can't read " + std::to_string(data.size()) + " bytes at " + hex(addr));
        for (size_t i = 0; i < count; ++i) encode(many ? a["value"][i] : a["value"], *t, data.data() + i * t->size);
    }
    std::vector<uint8_t> before(data.size());
    if (!safe_read(addr, before.data(), before.size()))
        throw std::runtime_error("can't read " + std::to_string(data.size()) + " bytes at " + hex(addr));
    if (!safe_write(addr, data.data(), data.size())) throw std::runtime_error("can't write at " + hex(addr));
    json out{{"address", hex(addr)}, {"before", bytes_hex(before.data(), before.size())},
             {"after", bytes_hex(data.data(), data.size())}};
    if (chain.size() > 1) out["chain"] = chain;
    return out;
}

// ---- call ----

json call_tool(const json& a) {
    const uintptr_t fn = resolve(a);
    if (!executable(fn)) throw std::runtime_error(hex(fn) + " isn't code");
    const std::string conv_name = lower(a.value("convention", std::string("cdecl")));
    Conv conv;
    if (conv_name == "cdecl") conv = Conv::Cdecl;
    else if (conv_name == "stdcall") conv = Conv::Stdcall;
    else if (conv_name == "thiscall") conv = Conv::Thiscall;
    else if (conv_name == "fastcall") conv = Conv::Fastcall;
    else throw std::runtime_error("convention: cdecl, stdcall, thiscall or fastcall");
    const std::string returns = lower(a.value("returns", std::string("int")));
    if (returns != "int" && returns != "float" && returns != "void" && returns != "cstr")
        throw std::runtime_error("returns: int, float, cstr or void");

    std::vector<uint32_t> args;
    std::deque<std::string> strings;              // stay put while the call runs
    std::deque<std::vector<uint8_t>> buffers;
    std::vector<std::pair<size_t, size_t>> buffer_args;   // (arg index, buffer index)
    if (a.contains("args")) {
        if (!a["args"].is_array() || a["args"].size() > 16) throw std::runtime_error("args: an array of up to 16");
        for (const json& v : a["args"]) {
            long long n = 0;
            if (v.is_number_float()) {
                const float f = v.get<float>();
                uint32_t bits;
                std::memcpy(&bits, &f, 4);
                args.push_back(bits);
            } else if (parse_number(v, n)) {
                args.push_back((uint32_t)n);
            } else if (v.is_boolean()) {
                args.push_back(v.get<bool>() ? 1 : 0);
            } else if (v.is_object() && v.contains("str")) {
                strings.push_back(v["str"].get<std::string>());
                args.push_back((uint32_t) reinterpret_cast<uintptr_t>(strings.back().c_str()));
            } else if (v.is_object() && v.contains("buf")) {
                const int size = v["buf"].get<int>();
                if (size < 1 || size > 65536) throw std::runtime_error("buf: 1 to 65536 bytes");
                buffers.emplace_back(size, 0);
                buffer_args.push_back({args.size(), buffers.size() - 1});
                args.push_back((uint32_t) reinterpret_cast<uintptr_t>(buffers.back().data()));
            } else if (v.is_object() && v.contains("f32")) {
                const float f = v["f32"].get<float>();
                uint32_t bits;
                std::memcpy(&bits, &f, 4);
                args.push_back(bits);
            } else if (v.is_object() && v.contains("address")) {
                args.push_back((uint32_t)resolve(v));
            } else {
                throw std::runtime_error("arg " + v.dump() + ": a number, hex string, bool, {\"f32\": x}, "
                                         "{\"str\": text}, {\"buf\": bytes} or {\"address\": ...}");
            }
        }
    }
    if ((conv == Conv::Thiscall && args.empty()) || (conv == Conv::Fastcall && args.empty()))
        throw std::runtime_error("thiscall / fastcall: args[0] is ecx");
    uint32_t regs[6] = {};
    unsigned regs_set = 0;
    if (a.contains("registers")) {   // the game's __usercall functions: {"eax": ..., "ecx": ...}
        static const char* names[6] = {"eax", "ebx", "ecx", "edx", "esi", "edi"};
        if (!a["registers"].is_object()) throw ToolError("bad_args", "registers: {\"eax\": value, ...}");
        for (const auto& [name, v] : a["registers"].items()) {
            const std::string n = lower(name);
            int k = 0;
            while (k < 6 && n != names[k]) ++k;
            if (k == 6) throw ToolError("bad_args", "registers: eax, ebx, ecx, edx, esi or edi, not " + name);
            long long x = 0;
            if (v.is_object() && v.contains("address")) x = (long long)resolve(v);
            else if (v.is_boolean()) x = v.get<bool>() ? 1 : 0;
            else if (!parse_number(v, x))
                throw ToolError("bad_args", "registers." + n + ": a number, hex string or {\"address\": ...}");
            regs[k] = (uint32_t)x;
            regs_set |= 1u << k;
        }
    }
    const CallResult r = mem::call(fn, conv, args.data(), (int)args.size(), returns == "float", regs, regs_set);
    if (!r.ok) {
        char b[96];
        std::snprintf(b, sizeof(b), "the call raised exception 0x%08X at 0x%08X", r.code, (unsigned)r.at);
        throw std::runtime_error(b);
    }
    json out{{"function", hex(fn)}};
    if (returns == "int") out["eax"] = json{{"hex", hex(r.eax)}, {"int", (int32_t)r.eax}, {"edx", hex(r.edx)}};
    else if (returns == "float") out["st0"] = r.st0;
    else if (returns == "cstr") out["string"] = safe_cstr(r.eax, 4096), out["eax"] = hex(r.eax);
    json bufs = json::array();
    for (const auto& [arg, b] : buffer_args) {
        const auto& buf = buffers[b];
        bufs.push_back({{"arg", arg}, {"hex", bytes_hex(buf.data(), std::min<size_t>(buf.size(), 4096))}});
    }
    if (!bufs.empty()) out["buffers"] = bufs;
    return out;
}

// ---- enter_room ----

json enter_room(const json& a) {
    if (!a.contains("room") || !a["room"].is_number_integer()) throw std::runtime_error("room: a room number");
    const int room = a["room"].get<int>();
    if (!AreaJump::is_valid_room_id(room))
        throw std::runtime_error("no room " + std::to_string(room) + " (not a game room, nor one a mod / plugin added)");
    aRoom* r = room_classes::current();
    // aGame takes the jump only in a room or as a mission starts; from the Mission Menu, sdk/MissionMenu starts the menu's
    // difficulty's mission into the room (2026-11-18). The title and other screens can't.
    if (!r && !mission_menu::in_mission_menu())
        throw std::runtime_error("only from a room or the Mission Menu (the title and other screens don't take jumps)");
    if (!mission_menu::enter_room(room)) throw std::runtime_error("can't jump now (no aGame)");
    return json{{"jumping_to", room}, {"label", room_classes::label(room) ? json(room_classes::label(room)) : json()},
                {"from_room", r ? json(r->room_no) : json()},
                {"note", r ? "aGame room_id / init_jump are set; the game changes room over the next frames: poll status"
                           : "from the Mission Menu: the menu starts its difficulty's mission, which loads this room; poll status"}};
}

// ---- print_text ----

struct Print {
    int id;
    std::string text;
    int slot, x, y;
    unsigned w, h;
    MtColor color;
    int prio;
    int frames_left;
};
std::mutex g_print_mutex;
std::vector<Print> g_prints;
int g_next_print = 1;

// sMessage::printf takes the colour as bytes B, G, R, A (the SDK's MtColor names them r, g, b, a): seen in a
// screenshot 2026-11-18, FFD040 came out light blue. So the fields are filled in that order.
MtColor parse_color(const json& c) {
    if (c.is_array() && (c.size() == 3 || c.size() == 4)) {
        auto ch = [&](size_t i) { return (uint8_t)std::clamp(c[i].get<int>(), 0, 255); };
        return MtColor{ch(2), ch(1), ch(0), c.size() == 4 ? ch(3) : (uint8_t)255};   // see bgra below
    }
    if (c.is_string()) {   // "RRGGBB" or "RRGGBBAA"
        const auto b = parse_hex(c.get<std::string>());
        if (b.size() == 3 || b.size() == 4) return MtColor{b[2], b[1], b[0], b.size() == 4 ? b[3] : (uint8_t)255};
    }
    throw std::runtime_error("color: [r, g, b(, a)] 0-255 or \"RRGGBB(AA)\"");
}

json print_text(const json& a) {
    std::lock_guard<std::mutex> lock(g_print_mutex);
    if (a.value("clear", false)) {
        const size_t n = g_prints.size();
        g_prints.clear();
        if (!a.contains("text")) return json{{"cleared", n}};
    }
    if (a.contains("remove")) {
        const int id = a["remove"].get<int>();
        const auto it = std::remove_if(g_prints.begin(), g_prints.end(), [id](const Print& p) { return p.id == id; });
        const bool found = it != g_prints.end();
        g_prints.erase(it, g_prints.end());
        return json{{"removed", found}};
    }
    if (!a.contains("text") || !a["text"].is_string()) throw std::runtime_error("text is required");
    Print p;
    p.text = a["text"].get<std::string>();
    for (char c : p.text)
        if ((unsigned char)c >= 0x80) throw std::runtime_error("text: ASCII only (the debug font's table)");
    p.slot = a.value("slot", menus::kDebugFontSlot);
    p.x = a.value("x", 64);
    p.y = a.value("y", 64);
    const int size = a.value("size", 24);
    p.w = (unsigned)a.value("w", size / 2);
    p.h = (unsigned)a.value("h", size);
    p.color = a.contains("color") ? parse_color(a["color"]) : MtColor{255, 255, 255, 255};
    p.prio = a.value("prio", 0);
    p.frames_left = std::clamp(a.value("frames", 300), 1, 60 * 60 * 10);
    if (p.slot < 0 || p.slot >= menus::kMessageSlots) throw std::runtime_error("slot: 0-11");
    if (!menus::slot_loaded(p.slot)) throw std::runtime_error("message slot " + std::to_string(p.slot) + " has nothing loaded");
    if (g_prints.size() >= 64) throw std::runtime_error("64 texts on screen already (clear or remove some)");
    p.id = g_next_print++;
    g_prints.push_back(p);
    return json{{"id", p.id}, {"frames", p.frames_left}};
}

// ---- spawn / kill_unit ----

bool add_unit_seh(void* obj, int line) {
    __try {
        devil4_sdk::sUnit_spawn(obj, line);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

json spawn(const json& a) {
    const std::string cls = a.value("class", std::string());
    if (cls.empty()) throw std::runtime_error("class is required");
    const uintptr_t dti = find_dti(cls);
    if (!dti) throw std::runtime_error("no class '" + cls + "'");
    if (!dti_is_a(dti, "cunit")) throw std::runtime_error(cls + " isn't a unit (cUnit)");
    if (!a.contains("line") || !a["line"].is_number_integer()) throw std::runtime_error("line: the move line 0-31");
    const int line = a["line"].get<int>();
    if (line < 0 || line > 31) throw std::runtime_error("line: 0-31");
    if (!devil4_sdk::get_sUnit()) throw std::runtime_error("no sUnit yet");
    const uintptr_t new_instance = rd_ptr(rd_ptr(dti) + 4);   // the DTI node's vtable slot 1, thiscall on the node
    if (!executable(new_instance)) throw std::runtime_error(cls + " has no newInstance");
    const bool coord = dti_is_a(dti, "ucoord");
    if (a.contains("pos") && !coord) throw std::runtime_error(cls + " has no position (not a uCoord)");
    const uint32_t this_arg = (uint32_t)dti;
    const CallResult r = mem::call(new_instance, Conv::Thiscall, &this_arg, 1, false);
    if (!r.ok) throw std::runtime_error("newInstance raised an exception");
    const uintptr_t unit = r.eax;
    if (!unit) throw std::runtime_error("newInstance gave null (an abstract class?)");
    if (a.contains("pos")) {
        uint8_t v[16];
        safe_read(unit + 0x30, v, 16);
        encode(a["pos"], *type_named("vec3"), v);
        safe_write(unit + 0x30, v, 16);
    }
    if (!add_unit_seh(reinterpret_cast<void*>(unit), line))
        throw std::runtime_error("sUnit::addBottom raised an exception (the unit at " + hex(unit) + " is leaked)");
    return json{{"address", hex(unit)}, {"class", dti_name(dti)}, {"line", line},
                {"note", "the unit is set up by its own init on its first move; classes that need data from their "
                         "spawner (enemies' set data, effects' resources) may misbehave"}};
}

constexpr uintptr_t kMoveLines = 0x20;   // sUnit::mMoveLine (Queries.cpp)

// calls f(unit) for every unit on the move lines; f returns false to stop
template <typename F> void each_unit(F&& f) {
    const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
    if (!su) return;
    for (int i = 0; i < 32; ++i) {
        MoveLine ml;
        if (!rd(su + kMoveLines + i * sizeof(MoveLine), ml)) continue;
        int guard = 0;
        for (uintptr_t u = reinterpret_cast<uintptr_t>(ml.mTop); u && guard < 20000; ++guard) {
            cUnit cu;
            if (!rd(u, cu)) break;
            if (!f(u, i)) return;
            u = reinterpret_cast<uintptr_t>(cu.mp_next_unit);
        }
    }
}

bool mark_deleted(uintptr_t unit) {
    uint32_t flags = 0;
    if (!rd(unit + 4, flags)) return false;
    if ((flags & 7) >= 3) return true;   // already on its way out
    // the unit's own kill (vtable +0x30): enemies take their lock-on targets out of sLockOnTargetMgr before they mark
    // themselves for deletion; the flag alone left the targets on freed units (crash in cLockOnTarget::sort 0x48BB43,
    // 2026-11-19). The flag stays the fallback when the call fails.
    const uintptr_t kill = rd_ptr(rd_ptr(unit) + 0x30);
    const uint32_t this_arg = (uint32_t)unit;
    if (executable(kill) && mem::call(kill, Conv::Thiscall, &this_arg, 1, false).ok && rd(unit + 4, flags) && (flags & 7) >= 3)
        return true;
    flags = (flags & ~7u) | 3u;          // BEFLAG_PRE_DELETE: sUnit deletes it (as PhotoMode's Remove)
    return safe_write(unit + 4, &flags, 4);
}

json kill_unit(const json& a) {
    json killed = json::array();
    if (a.contains("address")) {
        long long n = 0;
        if (!parse_number(a["address"], n)) throw std::runtime_error("address: a number or hex string");
        const uintptr_t target = (uintptr_t)n;
        bool found = false;
        each_unit([&](uintptr_t u, int) { if (u == target) { found = true; return false; } return true; });
        if (!found) throw std::runtime_error(hex(target) + " isn't a unit on sUnit's move lines");
        if (!mark_deleted(target)) throw std::runtime_error("can't write its flags");
        killed.push_back({{"address", hex(target)}, {"class", dti_name(dti_of(target))}});
        return json{{"killed", killed}};
    }
    const std::string cls = lower(a.value("class", std::string()));
    if (cls.empty()) throw std::runtime_error("address or class is required");
    if (cls == "cunit" || cls == "mtobject") throw std::runtime_error("refusing to delete every unit");
    const int max = std::clamp(a.value("max", 50), 1, 1000);
    std::vector<uintptr_t> units;
    each_unit([&](uintptr_t u, int) {
        const uintptr_t d = dti_of(u);
        if (d && dti_is_a(d, cls)) units.push_back(u);
        return (int)units.size() < max;
    });
    for (uintptr_t u : units)
        if (mark_deleted(u)) killed.push_back({{"address", hex(u)}, {"class", dti_name(dti_of(u))}});
    return json{{"killed", killed}};
}

} // namespace

void actions_frame() {
    std::lock_guard<std::mutex> lock(g_print_mutex);
    for (Print& p : g_prints) menus::print(p.slot, p.x, p.y, p.w, p.h, p.color, p.prio, p.text.c_str());
    for (Print& p : g_prints) --p.frames_left;
    g_prints.erase(std::remove_if(g_prints.begin(), g_prints.end(), [](const Print& p) { return p.frames_left <= 0; }),
                   g_prints.end());
}

void register_actions() {
    json write_props = address_props();
    write_props["type"] = prop("string", "bytes (default; value = hex string) or u8 ... f64, ptr, vec3, vec4, mat4");
    write_props["value"] = json{{"description", "the value: as `read` gives it (an array writes consecutive scalars)"}};
    add_tool({"write",
              "Write game memory: address / path / offset as `read`, type, value. Code and read-only pages are opened "
              "for the write. Returns the bytes before and after (write `before` back to undo). Game thread.",
              json{{"type", "object"}, {"properties", write_props}, {"required", json::array({"address", "value"})}},
              true, write_tool, true});

    json call_props = address_props();
    call_props["convention"] = prop("string", "cdecl (default), stdcall, thiscall (args[0] = ecx), fastcall (ecx, edx)");
    call_props["args"] = json{{"type", "array"},
                              {"description", "up to 16: integers / hex strings, floats (sent as f32 bits), bools, "
                                              "{\"f32\": x}, {\"str\": text} (a char* to it), {\"buf\": bytes} (a zeroed "
                                              "buffer, returned as hex after the call), {\"address\": ...} (resolved as "
                                              "read's)"}};
    call_props["returns"] = prop("string", "int (eax, edx; default), float (st0), cstr (eax as char*), void");
    call_props["registers"] = json{{"type", "object"},
                                   {"description", "register arguments of a __usercall function: {\"eax\": value, ...} "
                                                   "(eax, ebx, ecx, edx, esi, edi; a number, hex string or "
                                                   "{\"address\": ...}); they override the convention's ecx / edx, args "
                                                   "go on the stack"}};
    add_tool({"call",
              "Call a game function on the game thread: address (or a root + path to a pointer), convention, args, "
              "registers (for __usercall). "
              "An exception inside it comes back as an error (the game may still be left in a bad state).",
              json{{"type", "object"}, {"properties", call_props}, {"required", json::array({"address"})}},
              true, call_tool, true});

    add_tool({"enter_room",
              "Jump to a room by number (AreaJump: aGame room_id + init_jump), vanilla or one a mod / plugin added "
              "(room_classes), from a room or from the Mission Menu (there the menu starts its difficulty's mission into the "
              "room). The change happens over the next frames: poll status for the new room.",
              obj({{"room", prop("integer", "room number, e.g. 200 or 900")}}, {"room"}), true, enter_room, true});

    add_tool({"print_text",
              "Show ASCII text on screen for some frames (menus::print, sMessage::printf in the debug font, slot 11). "
              "x / y in 1280 x 720 (default 64, 64), size = character height (default 24; w / h override), color "
              "[r, g, b, a] or \"RRGGBBAA\", frames (default 300). Returns an id; remove = id takes one off, clear = "
              "true all.",
              obj({{"text", prop("string", "ASCII; \\n starts a line")}, {"x", prop("integer", "")},
                   {"y", prop("integer", "")}, {"size", prop("integer", "character height")},
                   {"w", prop("integer", "character width")}, {"h", prop("integer", "character height")},
                   {"color", json{{"description", "[r, g, b, a] or \"RRGGBBAA\""}}},
                   {"frames", prop("integer", "frames shown (default 300)")},
                   {"slot", prop("integer", "message slot (default 11, the debug font)")},
                   {"prio", prop("integer", "draw priority")}, {"remove", prop("integer", "an id to take off")},
                   {"clear", prop("boolean", "take all off")}}),
              true, print_text, true});

    add_tool({"spawn",
              "Make a unit of a class (its DTI newInstance) and add it to a move line (sUnit::addBottom), with pos for "
              "uCoord classes (game cm). Classes that expect a spawner's setup may misbehave; this is for testing.",
              obj({{"class", prop("string", "DTI class, a cUnit")}, {"line", prop("integer", "move line 0-31")},
                   {"pos", json{{"type", "array"}, {"description", "[x, y, z]"}}}},
                  {"class", "line"}),
              true, spawn, true});

    add_tool({"kill_unit",
              "Delete units the way the game does (be flag PRE_DELETE, sUnit frees them): one by address (it must be on "
              "a move line), or every unit of a class (that class or derived; max default 50).",
              obj({{"address", any_of("a unit's address (find_units)")}, {"class", prop("string", "DTI class")},
                   {"max", prop("integer", "units at most")}}),
              true, kill_unit, true});
}

} // namespace debug_server
