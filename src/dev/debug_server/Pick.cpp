#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../picker/ObjectPicker.hpp"
#include "../postfx/PostFX.hpp"
#include "../unit_packs/UnitPacks.hpp"
#include "../../sdk/Schedulers.hpp"
#include "../../sdk/Selection.hpp"
#include <vector>

// The object picker's tools (Vibed\PostFX\README.md "Object picker"; the picking itself is dev/picker's, shared with
// the in-game Ctrl + click):
// - `pick`: PostFX reads the frame's depth at a pixel (world point + object ID); on the game thread picker::resolve
//   turns it into a unit: the model that drew the pixel, else the model whose nearest joint is closest. select = true
//   makes it the shared selection (sdk/Selection: PostFX outlines it, the picker's window shows it).
// - `selection`: what's selected (with the .sdl track that made it), or select / clear by address.

namespace debug_server {
namespace {

using namespace mem;

json candidate_json(const picker::Candidate& c) {
    json j{{"address", hex(c.unit)}, {"class", dti_name(c.dti)}, {"line", c.line}, {"distance", c.dist},
           {"at", json::array({c.at[0], c.at[1], c.at[2]})}};
    if (c.joint >= 0) j["joint"] = c.joint;
    else j["by"] = "position";
    return j;
}

// game thread: the unit's class, move line, position, and what made it (its .sdl track, a UnitPacks spawn)
json unit_json(uintptr_t unit) {
    json u{{"address", hex(unit)}};
    const uintptr_t dti = dti_of(unit);
    if (dti) u["class"] = dti_name(dti);
    u["line"] = picker::line_of(unit);
    if (dti && dti_is_a(dti, "ucoord")) {
        float p[3];
        if (safe_read(unit + 0xA0 + 0x30, p, sizeof p)) u["world"] = json::array({p[0], p[1], p[2]});
    }
    schedulers::Owner o;
    if (schedulers::owner_of(unit, o)) {
        json keyed = json::array();
        for (const schedulers::Track& t : o.keyed) keyed.push_back(json{{"index", t.index}, {"type", t.type}, {"name", t.name}});
        u["made_by"] = json{{"path", o.path}, {"scheduler", hex(o.scheduler)}, {"track", o.unit.index},
                            {"name", o.unit.name}, {"keyed", keyed}};
    }
    const std::string from = unit_packs::origin(unit);
    if (!from.empty()) u["unit_packs"] = from;
    return u;
}

json pick_tool(const json& a) {
    long long x = 0, y = 0;
    if (!a.contains("x") || !a.contains("y") || !parse_number(a["x"], x) || !parse_number(a["y"], y))
        throw ToolError("bad_args", "x and y (back-buffer pixels) are needed");
    const float max_dist = (float)a.value("max_dist", 60.0);
    const bool select = a.value("select", false);
    const post_fx::DepthSample s = post_fx::sample_world((int)x, (int)y, 2000);
    if (!s.ok) throw ToolError("no_point", s.error, json{{"texel", json::array({s.texel[0], s.texel[1]})}});
    const json picked = run_on_game_thread([s, max_dist, select]() -> json {
        const picker::Result r = picker::resolve(s, max_dist);
        json out{{"candidates", json::array()}};
        for (const picker::Candidate& c : r.candidates) out["candidates"].push_back(candidate_json(c));
        if (r.unit) {
            json u = unit_json(r.unit);
            for (const picker::Candidate& c : r.candidates)
                if (c.unit == r.unit) {
                    u["distance"] = c.dist;
                    if (c.joint >= 0) u["joint"] = c.joint;
                }
            u["by"] = selection::by_name(r.by);
            out["unit"] = u;
        }
        if (select) {
            selection::set_point(s.world);
            selection::select(r.unit, r.by);
            out["selected"] = r.unit != 0;
        }
        return out;
    });
    json out{{"point", json::array({s.world[0], s.world[1], s.world[2]})}, {"distance", s.distance},
             {"texel", json::array({s.texel[0], s.texel[1]})}, {"id", s.id}, {"candidates", picked["candidates"]}};
    out["unit"] = picked.contains("unit") ? picked["unit"] : json();
    if (picked.contains("selected")) out["selected"] = picked["selected"];
    return out;
}

json selection_tool(const json& a) {
    if (a.value("clear", false)) selection::clear();
    if (a.contains("address")) {
        long long v = 0;
        if (!parse_number(a["address"], v)) throw ToolError("bad_args", "address: a number or a hex string");
        if (v && !picker::alive((uintptr_t)v)) throw ToolError("not_found", "no unit " + hex((uintptr_t)v) + " on a move line or drawn lately");
        selection::select((uintptr_t)v, selection::By::Tool);
    }
    json out{{"by", selection::by_name(selection::by())}};
    const uintptr_t unit = selection::unit();
    out["unit"] = unit ? unit_json(unit) : json();
    float p[3];
    out["point"] = selection::point(p) ? json::array({p[0], p[1], p[2]}) : json();
    return out;
}

} // namespace

void register_pick() {
    json schema{{"type", "object"},
                {"properties",
                 {{"x", {{"type", "integer"}, {"description", "back-buffer pixel x"}}},
                  {"y", {{"type", "integer"}, {"description", "back-buffer pixel y"}}},
                  {"max_dist", {{"type", "number"}, {"description", "cm from the point to a unit's nearest joint for a pick (default 60)"}}},
                  {"select", {{"type", "boolean"}, {"description", "also make it the shared selection and the picked point (none = clear)"}}}}},
                {"required", json::array({"x", "y"})}};
    add_tool({"pick",
              "What's under a screen pixel (back-buffer pixels, as screenshot's crop): the world point there (game cm, from "
              "the frame's depth; nothing for sky), the object ID there (each model draws its own into the depth: the exact "
              "unit, by = id) else the model unit whose nearest joint is closest (within max_dist cm, by = joint), with its "
              "class, line, world position, made_by (the .sdl track that created it and the tracks keying it) and unit_packs "
              "(the spawn it came from); up to 5 candidates by joint. select = true makes it the shared selection (outlined in "
              "the scene, shown in the Object picker window) and the picked point. Depth holds opaque geometry only: effects "
              "and transparent meshes can't be picked. Waits for the next frame.",
              schema, false, pick_tool});
    json sel{{"type", "object"},
             {"properties",
              {{"address", {{"type", "string"}, {"description", "select this unit (0 = clear)"}}},
               {"clear", {{"type", "boolean"}, {"description", "clear the selection"}}}}}};
    add_tool({"selection",
              "The shared selection (the object picker's, UnitPacks' Select, pick's select): unit {address, class, line, "
              "world, made_by, unit_packs} or null, by (id / joint / list / tool), and the last picked point. address selects "
              "a unit (on a move line, or a model drawn lately: not_found otherwise), clear clears it. line -1 = a part another "
              "unit draws (Nero's coat, a weapon).",
              sel, true, selection_tool});
}

} // namespace debug_server
