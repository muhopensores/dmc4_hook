#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../postfx/PostFX.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/uActor.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

// The `pick` tool: what's under a screen pixel (the object picker, Vibed\PostFX\README.md "Object picker", depth only).
// PostFX reads the frame's depth at the pixel and turns it into a world point; then, on the game thread, every unit that
// is a uModel is scored by its joint nearest to that point (a model without joints by its position), and the best within
// max_dist is the pick. Stage geometry has no unit of this kind nearby, so a click on a wall or floor gives the point and
// no unit (or a far candidate, past max_dist).

namespace debug_server {
namespace {

using namespace mem;

constexpr uintptr_t kMoveLines = 0x20;   // sUnit::mMoveLine (as Queries.cpp)
constexpr uintptr_t kJointNum = offsetof(uModel, mJointNum);
constexpr uintptr_t kJoints = offsetof(uModel, mpJoint);
constexpr uintptr_t kJointWmat = offsetof(UModelJoint, mWmat);
constexpr uintptr_t kPos = 0x30;         // uCoord::mPos (as find_units)

struct Candidate {
    uintptr_t unit;
    uintptr_t dti;
    int       line;
    int       joint;          // -1: scored by its position
    float     dist;
    float     at[3];          // the joint's (or the unit's) world position
};

float dist3(const float* a, const float* b) {
    const float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}

std::vector<Candidate> score_units(const float* p) {
    std::vector<Candidate> out;
    const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
    if (!su) return out;
    for (int line = 0; line < 32; ++line) {
        MoveLine ml;
        if (!rd(su + kMoveLines + line * sizeof(MoveLine), ml)) continue;
        int n = 0;
        for (uintptr_t u = reinterpret_cast<uintptr_t>(ml.mTop); u && n < 20000; ++n) {
            cUnit cu;
            if (!rd(u, cu)) break;
            const uintptr_t dti = dti_of(u);
            if (dti && dti_is_a(dti, "umodel") && cu.flags.bits.mBeFlag < 3) {
                Candidate c{ u, dti, line, -1, 1e30f, {} };
                uint32_t count = 0;
                uintptr_t joints = 0;
                if (rd(u + kJointNum, count) && rd(u + kJoints, joints) && joints && count > 0 && count <= 512) {
                    for (uint32_t j = 0; j < count; ++j) {
                        float w[3];
                        if (!safe_read(joints + j * sizeof(UModelJoint) + kJointWmat + 0x30, w, sizeof w)) break;
                        const float d = dist3(w, p);
                        if (d < c.dist) { c.dist = d; c.joint = (int)j; std::copy(w, w + 3, c.at); }
                    }
                }
                if (c.joint < 0) {
                    float w[3];
                    if (safe_read(u + kPos, w, sizeof w)) { c.dist = dist3(w, p); std::copy(w, w + 3, c.at); }
                }
                if (c.dist < 1e29f) out.push_back(c);
            }
            u = reinterpret_cast<uintptr_t>(cu.mp_next_unit);
        }
    }
    std::sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b) { return a.dist < b.dist; });
    return out;
}

json candidate_json(const Candidate& c) {
    json j{{"address", hex(c.unit)}, {"class", dti_name(c.dti)}, {"line", c.line}, {"distance", c.dist},
           {"at", json::array({c.at[0], c.at[1], c.at[2]})}};
    if (c.joint >= 0) j["joint"] = c.joint;
    else j["by"] = "position";
    return j;
}

json pick_tool(const json& a) {
    long long x = 0, y = 0;
    if (!a.contains("x") || !a.contains("y") || !parse_number(a["x"], x) || !parse_number(a["y"], y))
        throw ToolError("bad_args", "x and y (back-buffer pixels) are needed");
    const double max_dist = a.value("max_dist", 60.0);
    const post_fx::DepthSample s = post_fx::sample_world((int)x, (int)y, 2000);
    if (!s.ok) throw ToolError("no_point", s.error, json{{"texel", json::array({s.texel[0], s.texel[1]})}});
    const json units = run_on_game_thread([p = s.world, max_dist]() -> json {
        const std::vector<Candidate> c = score_units(p);
        json out{{"candidates", json::array()}};
        for (size_t i = 0; i < c.size() && i < 5; ++i) out["candidates"].push_back(candidate_json(c[i]));
        if (!c.empty() && c[0].dist <= max_dist) out["unit"] = candidate_json(c[0]);
        return out;
    });
    json out{{"point", json::array({s.world[0], s.world[1], s.world[2]})}, {"distance", s.distance},
             {"texel", json::array({s.texel[0], s.texel[1]})}, {"candidates", units["candidates"]}};
    out["unit"] = units.contains("unit") ? units["unit"] : json();
    return out;
}

} // namespace

void register_pick() {
    json schema{{"type", "object"},
                {"properties",
                 {{"x", {{"type", "integer"}, {"description", "back-buffer pixel x"}}},
                  {"y", {{"type", "integer"}, {"description", "back-buffer pixel y"}}},
                  {"max_dist", {{"type", "number"}, {"description", "cm from the point to a unit's nearest joint for a pick (default 60)"}}}}},
                {"required", json::array({"x", "y"})}};
    add_tool({"pick",
              "What's under a screen pixel (back-buffer pixels, as screenshot's crop): the world point there (game cm, from "
              "the frame's depth; nothing for sky), and the model unit whose nearest joint is closest to it (the pick when "
              "within max_dist cm) with up to 5 candidates (address, class, move line, joint, distance). Depth holds opaque "
              "geometry only: effects and transparent meshes can't be picked. Waits for the next frame.",
              schema, false, pick_tool});
}

} // namespace debug_server
