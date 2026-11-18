#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../../ModFramework.hpp"
#include "../../Mods.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/Players.hpp"
#include "../../sdk/RoomClasses.hpp"
#include "../plugins/PluginHost.hpp"
#include <windows.h>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// S1 tools (plan Vibed/GameMCP/README.md): read, find_units, class_tree, room_classes, plugins, mods.
// Every read of game memory goes through mem::safe_read (Memory.cpp), so a bad address is an error, not a crash.

namespace debug_server {
namespace {

using namespace mem;

json dti_json(uintptr_t a, int depth) {
    Dti d;
    if (!read_dti(a, d)) return json();
    json j{{"name", a == kDtiRoot ? std::string("(root)") : dti_name(a)}, {"address", hex(a)}, {"size", d.size_flags & 0xFFFFFF},
           {"flags", hex(d.size_flags >> 24)}, {"id", hex(d.id)}};
    int count = 0;
    for (uintptr_t c = d.child; c && count < 4096; ++count) {
        Dti cd;
        if (!read_dti(c, cd)) break;
        c = cd.next;
    }
    j["child_count"] = count;
    if (depth > 0 && count) {
        json kids = json::array();
        for (uintptr_t c = d.child; c && kids.size() < 4096; ) {
            Dti cd;
            if (!read_dti(c, cd)) break;
            kids.push_back(dti_json(c, depth - 1));
            c = cd.next;
        }
        std::sort(kids.begin(), kids.end(), [](const json& x, const json& y) {
            return x["name"].get<std::string>() < y["name"].get<std::string>();
        });
        j["children"] = kids;
    }
    return j;
}

json dti_parents(uintptr_t a) {
    json p = json::array();
    Dti d;
    for (int i = 0; read_dti(a, d) && d.parent && d.parent != kDtiRoot && i < 64; ++i) {   // the root has no name
        p.push_back(dti_name(d.parent));
        a = d.parent;
    }
    return p;
}

json class_tree(const json& a) {
    const int max = std::clamp(a.value("max", 200), 1, 5000);
    if (a.contains("search")) {
        const std::string q = lower(a["search"].get<std::string>());
        json hits = json::array();
        int budget = 50000;
        walk_dti(kDtiRoot, [&](uintptr_t n, int) {
            const std::string nm = dti_name(n);
            if (lower(nm).find(q) != std::string::npos) {
                Dti d;
                read_dti(n, d);
                json parents = dti_parents(n);
                hits.push_back({{"name", nm}, {"size", d.size_flags & 0xFFFFFF},
                                {"parent", parents.empty() ? json() : parents[0]}});
            }
            return (int)hits.size() < max;
        }, budget);
        std::sort(hits.begin(), hits.end(), [](const json& x, const json& y) {
            return x["name"].get<std::string>() < y["name"].get<std::string>();
        });
        return json{{"search", a["search"]}, {"matches", hits}, {"truncated", (int)hits.size() >= max}};
    }
    const std::string name = a.value("name", std::string());
    const uintptr_t n = name.empty() ? kDtiRoot : find_dti(name);
    if (!n) throw std::runtime_error("no class '" + name + "' (try search)");
    json j = dti_json(n, std::clamp(a.value("depth", 1), 0, 6));
    j["parents"] = dti_parents(n);
    return j;
}

// SDK structs `read` knows by name (offsets from the sdk headers' static_asserts)
struct Field {
    const char* name;
    uint32_t offset;
    const char* type;
};
struct Layout {
    const char* name;
    uint32_t size;
    std::vector<Field> fields;
};

const std::vector<Layout>& layouts() {
    static const std::vector<Layout> l = {
        {"cUnit", 0x18, {{"vtable", 0x0, "ptr"}, {"flags", 0x4, "u32"}, {"next", 0x8, "ptr"}, {"prev", 0xC, "ptr"},
                         {"delta_time", 0x10, "f32"}}},
        {"uCoord", 0xE0, {{"vtable", 0x0, "ptr"}, {"flags", 0x4, "u32"}, {"next", 0x8, "ptr"}, {"prev", 0xC, "ptr"},
                          {"delta_time", 0x10, "f32"}, {"parent", 0x18, "ptr"}, {"parent_joint", 0x1C, "i32"},
                          {"order", 0x20, "i32"}, {"pos", 0x30, "vec3"}, {"quat", 0x40, "vec4"}, {"scale", 0x50, "vec3"},
                          {"lmat", 0x60, "mat4"}, {"wmat", 0xA0, "mat4"}}},
        {"MoveLine", 0x18, {{"vtable", 0x0, "ptr"}, {"name", 0x4, "cstr"}, {"flags", 0x8, "u32"}, {"top", 0xC, "ptr"},
                            {"bottom", 0x10, "ptr"}, {"delta_time", 0x14, "f32"}}},
        {"MtDTI", 0x20, {{"vtable", 0x0, "ptr"}, {"name", 0x4, "cstr"}, {"next", 0x8, "ptr"}, {"child", 0xC, "ptr"},
                         {"parent", 0x10, "ptr"}, {"link", 0x14, "ptr"}, {"size_flags", 0x18, "u32"}, {"id", 0x1C, "u32"}}},
        {"aRoom", 0x1C0, {{"vtable", 0x0, "ptr"}, {"mode", 0x4, "u8"}, {"sub_state", 0x5, "u8"}, {"room_no", 0x28, "i32"},
                          {"stage_no", 0x2C, "i32"}}},
    };
    return l;
}

const Layout* layout_named(const std::string& n) {
    for (const Layout& l : layouts())
        if (lower(n) == lower(l.name)) return &l;
    return nullptr;
}

json read_struct(uintptr_t a, const Layout& l) {
    json j = json::object();
    for (const Field& f : l.fields) {
        try {
            j[f.name] = value_at(a + f.offset, *type_named(f.type));
        } catch (const std::exception&) {
            j[f.name] = json();
        }
    }
    return j;
}

std::string types_list() {
    std::string s = scalar_type_names();
    for (const Layout& l : layouts()) s += std::string(", ") + l.name;
    return s + ", object, bytes";
}

json read_tool(const json& a) {
    json chain = json::array();
    const uintptr_t addr = resolve(a, &chain);
    const std::string type = a.value("type", std::string("bytes"));
    json out{{"address", hex(addr)}, {"type", type}};
    if (chain.size() > 1) out["chain"] = chain;

    if (type == "bytes") {
        const int count = std::clamp(a.value("count", 64), 1, 4096);
        std::vector<uint8_t> buf(count);
        if (!safe_read(addr, buf.data(), buf.size()))
            throw std::runtime_error("can't read " + std::to_string(count) + " bytes at " + hex(addr));
        std::string h;
        static const char* d = "0123456789ABCDEF";
        for (size_t i = 0; i < buf.size(); ++i) {
            if (i) h += (i % 16 == 0) ? '\n' : ' ';
            h += d[buf[i] >> 4];
            h += d[buf[i] & 15];
        }
        out["hex"] = h;
        return out;
    }
    if (type == "object") {   // the class through getDTI, and as a struct when it is one `read` knows
        const uintptr_t dti = dti_of(addr);
        if (!dti) throw std::runtime_error("no class at " + hex(addr) + " (not an MtObject?)");
        out["class"] = dti_name(dti);
        out["parents"] = dti_parents(dti);
        if (dti_is_a(dti, "ucoord")) out["fields"] = read_struct(addr, *layout_named("uCoord"));
        else if (dti_is_a(dti, "aroom")) out["fields"] = read_struct(addr, *layout_named("aRoom"));
        else if (dti_is_a(dti, "cunit")) out["fields"] = read_struct(addr, *layout_named("cUnit"));
        return out;
    }
    const int count = std::clamp(a.value("count", 1), 1, 1024);
    if (const Layout* l = layout_named(type)) {
        if (count == 1) out["value"] = read_struct(addr, *l);
        else {
            json v = json::array();
            for (int i = 0; i < count; ++i) v.push_back(read_struct(addr + i * l->size, *l));
            out["value"] = v;
        }
        return out;
    }
    const TypeInfo* t = type_named(type);
    if (!t) throw std::runtime_error("no type '" + type + "' (types: " + types_list() + ")");
    if (count == 1) out["value"] = value_at(addr, *t);
    else {
        json v = json::array();
        for (int i = 0; i < count; ++i) v.push_back(value_at(addr + i * t->size, *t));
        out["value"] = v;
    }
    return out;
}

// ---- units ----

constexpr uintptr_t kMoveLines = 0x20;   // sUnit::mMoveLine (sizeof(sUnit) 0x320 = cSystem 0x20 + 32 lines)
static_assert(sizeof(sUnit) == kMoveLines + 32 * sizeof(MoveLine));

json find_units(const json& a) {
    const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
    if (!su) throw std::runtime_error("no sUnit yet");
    int only_line = -1;
    std::string line_name;
    if (a.contains("line")) {
        if (a["line"].is_number_integer()) only_line = a["line"].get<int>();
        else if (a["line"].is_string()) line_name = lower(a["line"].get<std::string>());
    }
    const std::string cls = a.contains("class") ? lower(a["class"].get<std::string>()) : std::string();
    const int max = std::clamp(a.value("max", 300), 1, 5000);
    int total = 0;
    bool truncated = false;
    json lines = json::array();
    for (int i = 0; i < 32; ++i) {
        MoveLine ml;
        if (!rd(su + kMoveLines + i * sizeof(MoveLine), ml)) continue;
        const json nm = safe_cstr(reinterpret_cast<uintptr_t>(ml.mName), 64);
        const std::string name = nm.is_string() ? nm.get<std::string>() : std::string();
        if (only_line >= 0 && i != only_line) continue;
        if (!line_name.empty() && lower(name) != line_name) continue;
        json units = json::array();
        int in_line = 0;
        for (uintptr_t u = reinterpret_cast<uintptr_t>(ml.mTop); u && in_line < 20000; ++in_line) {
            cUnit cu;
            if (!rd(u, cu)) break;
            const uintptr_t dti = dti_of(u);
            if (cls.empty() || (dti && dti_is_a(dti, cls))) {
                if (total >= max) { truncated = true; }
                else {
                    json j{{"address", hex(u)}, {"class", dti ? json(dti_name(dti)) : json()},
                           {"be_flag", (unsigned)cu.flags.bits.mBeFlag}, {"attr", (unsigned)cu.flags.bits.mUnitAttr}};
                    if (dti && dti_is_a(dti, "ucoord")) {
                        float p[3];
                        if (safe_read(u + 0x30, p, sizeof(p))) j["pos"] = json::array({p[0], p[1], p[2]});
                    }
                    units.push_back(j);
                }
                ++total;
            }
            u = reinterpret_cast<uintptr_t>(cu.mp_next_unit);
        }
        if (!units.empty() || (only_line >= 0 || !line_name.empty()))
            lines.push_back({{"line", i}, {"name", name}, {"units_in_line", in_line}, {"units", units}});
    }
    return json{{"matches", total}, {"truncated", truncated}, {"lines", lines}};
}

// ---- mods ----

const std::vector<std::unique_ptr<Mod>>& all_mods() {
    if (!g_framework || !g_framework->get_mods()) throw std::runtime_error("the mods aren't set up yet");
    return g_framework->get_mods()->get_mods();
}

Mod* mod_named(const std::string& name) {
    for (const auto& m : all_mods())
        if (lower(m->get_mod_name()) == lower(name)) return m.get();
    return nullptr;
}

json settings_of(Mod* m) {
    utility::Config cfg{};
    m->on_config_save(cfg);
    json s = json::object();
    for (const auto& [k, v] : cfg.get_key_values()) s[k] = v;
    return s;
}

json mods_tool(const json& a) {
    if (!a.contains("name")) {
        json list = json::array();
        const std::string q = a.contains("search") ? lower(a["search"].get<std::string>()) : std::string();
        for (const auto& m : all_mods()) {
            const std::string n = m->get_mod_name();
            if (!q.empty() && lower(n).find(q) == std::string::npos) continue;
            list.push_back(n);
        }
        return json{{"mods", list}};
    }
    const std::string name = a["name"].get<std::string>();
    Mod* m = mod_named(name);
    if (!m) throw std::runtime_error("no mod '" + name + "' (mods without a name lists them)");
    json out{{"name", m->get_mod_name()}};
    if (a.contains("set")) {
        if (!dev_tools())
            throw std::runtime_error("changing settings needs the panel's Dev tools switch");
        if (!a["set"].is_object()) throw std::runtime_error("set: an object of key: value");
        utility::Config cfg{};
        m->on_config_save(cfg);
        for (const auto& [k, v] : a["set"].items()) {
            if (!cfg.get(k)) throw std::runtime_error("'" + m->get_mod_name() + "' has no setting '" + k + "'");
            cfg.set(k, v.is_string() ? v.get<std::string>() : v.is_boolean() ? (v.get<bool>() ? "true" : "false") : v.dump());
        }
        m->on_config_load(cfg);   // as at startup: the mod applies its settings (it's saved with the rest on exit)
        out["changed"] = a["set"];
    }
    out["settings"] = settings_of(m);
    return out;
}

// ---- room classes, plugins ----

json room_classes_tool() {
    json list = json::array();
    for (const auto& i : room_classes::list())
        list.push_back({{"name", i.name}, {"base", i.base}, {"owner", i.owner}, {"room", i.room}, {"stage", i.stage},
                        {"size", i.size}, {"created", i.created}, {"in_area_table", i.in_area_table}, {"live", i.live},
                        {"error", i.error}});
    aRoom* r = room_classes::current();
    return json{{"created", room_classes::created()}, {"classes", list},
                {"current_room", r ? json(r->room_no) : json()}};
}

json plugins_tool() {
    json list = json::array();
    for (const auto& p : plugins::list())
        list.push_back({{"file", p.file}, {"name", p.name}, {"version", p.version}, {"author", p.author},
                        {"description", p.description}, {"state", p.state}, {"error", p.error}, {"enabled", p.enabled},
                        {"hooks", p.hooks}, {"log", p.log}});
    return json{{"plugins", list}};
}

json obj(std::initializer_list<std::pair<const char*, json>> props) {
    json p = json::object();
    for (const auto& [k, v] : props) p[k] = v;
    return json{{"type", "object"}, {"properties", p}};
}

json prop(const char* type, const char* description) { return json{{"type", type}, {"description", description}}; }

json any_of(const char* description) {
    return json{{"anyOf", json::array({json{{"type", "integer"}}, json{{"type", "string"}}})}, {"description", description}};
}

} // namespace

void register_queries() {
    add_tool({"read",
              "Read game memory (safely: a bad address is an error). address = a number, a hex string or a root "
              "(sArea, sUnit, sMediator, sRender, room, player1-4); path = offsets of pointers to follow from it "
              "([0x24, 0x10]: the pointer at address+0x24, then the one at that+0x10); offset = added at the end. "
              "type = bytes (hex dump, count = bytes), u8 ... f64, ptr, cstr (a char* there), vec3 / vec4 (16-byte "
              "MtVector), mat4, an SDK struct (cUnit, uCoord, MoveLine, MtDTI, aRoom) or object (its class through "
              "getDTI, with the struct's fields when it is a uCoord / aRoom / cUnit); count = elements. Game thread.",
              obj({{"address", any_of("number, hex string or root name")},
                   {"path", json{{"type", "array"}, {"items", any_of("offset")}, {"description", "pointers to follow"}}},
                   {"offset", any_of("added after the path")},
                   {"type", prop("string", "bytes (default), a scalar / vector type, an SDK struct or object")},
                   {"count", prop("integer", "elements (bytes: byte count, default 64, max 4096)")}}),
              true, read_tool});
    add_tool({"find_units",
              "The units on sUnit's 32 move lines (the game's update lists), top to bottom: address, class, be_flag "
              "(2 = moving), attr, and pos (game cm) for uCoord units. Filter by line (index 0-31, or its name "
              "Line0-Line31: DX9 names them no better) and class (that class or derived, e.g. uEnemy); max = units listed (default 300). Game "
              "thread.",
              obj({{"line", any_of("move line index 0-31 or name")},
                   {"class", prop("string", "DTI class name; derived classes match too")},
                   {"max", prop("integer", "units at most (default 300)")}}),
              true, find_units});
    add_tool({"class_tree",
              "The game's DTI class tree (MtDTI). name = a class (default the root): its size, id, parents and "
              "children to depth (default 1, max 6); or search = part of a name, matches with their size and parent "
              "(max default 200).",
              obj({{"name", prop("string", "class name, any case")},
                   {"depth", prop("integer", "levels of children (default 1)")},
                   {"search", prop("string", "substring of class names")},
                   {"max", prop("integer", "search matches at most")}}),
              true, class_tree});
    add_tool({"room_classes",
              "Room classes added by mods and plugins (sdk/RoomClasses): name, base, owner, room / stage numbers, "
              "whether made and recorded by sArea, live instances, errors; and the current room number.",
              obj({}), true, [](const json&) { return room_classes_tool(); }});
    add_tool({"plugins",
              "Plugin DLLs found in plugins\\: file, name, version, author, state (running / failed / stopped / off), "
              "error, hooks, their last log lines.",
              obj({}), false, [](const json&) { return plugins_tool(); }});
    add_tool({"mods",
              "dmc4_hook's mods. No name: their names (search = part of a name). name = a mod: its settings as saved "
              "in dmc4_hook.cfg. set = {key: value} changes settings the way loading the config does (needs the "
              "panel's Dev tools switch; saved with the rest when the game saves its config). Game thread.",
              obj({{"name", prop("string", "a mod's name (get_mod_name), any case")},
                   {"search", prop("string", "part of a mod name")},
                   {"set", prop("object", "settings to change: {key: value}")}}),
              true, mods_tool});
}

} // namespace debug_server
