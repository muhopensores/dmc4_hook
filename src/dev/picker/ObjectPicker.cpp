#include "ObjectPicker.hpp"
#include "../../ModFramework.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../sdk/uActor.hpp"
#include "../../sdk/Schedulers.hpp"
#include "../../sdk/World2Screen.hpp"
#include "../debug_server/Memory.hpp"
#include "../postfx/Tags.hpp"
#include "../unit_packs/UnitPacks.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <future>
#include <string>

namespace picker {
namespace {

using namespace debug_server::mem;

constexpr uintptr_t kMoveLines = 0x20;   // sUnit::mMoveLine
constexpr uintptr_t kJointNum = offsetof(uModel, mJointNum);
constexpr uintptr_t kJoints = offsetof(uModel, mpJoint);
constexpr uintptr_t kJointWmat = offsetof(UModelJoint, mWmat);
constexpr uintptr_t kParent = 0x18;      // uCoord::mpParent
constexpr uintptr_t kPos = 0x30;         // uCoord::mPos
constexpr uintptr_t kWmat = 0xA0;        // uCoord::mWmat (row vectors: translation in the 4th row)

float dist3(const float* a, const float* b) {
    const float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}

template <typename F> void each_unit(F&& f) {   // f(unit, line, cUnit) -> false stops
    const uintptr_t su = reinterpret_cast<uintptr_t>(devil4_sdk::get_sUnit());
    if (!su) return;
    for (int line = 0; line < 32; ++line) {
        MoveLine ml;
        if (!rd(su + kMoveLines + line * sizeof(MoveLine), ml)) continue;
        int n = 0;
        for (uintptr_t u = reinterpret_cast<uintptr_t>(ml.mTop); u && n < 20000; ++n) {
            cUnit cu;
            if (!rd(u, cu)) break;
            if (!f(u, line, cu)) return;
            u = reinterpret_cast<uintptr_t>(cu.mp_next_unit);
        }
    }
}

} // namespace

std::vector<Candidate> score_units(const float p[3]) {
    std::vector<Candidate> out;
    each_unit([&](uintptr_t u, int line, const cUnit& cu) {
        const uintptr_t dti = dti_of(u);
        if (!dti || !dti_is_a(dti, "umodel") || cu.flags.bits.mBeFlag >= 3) return true;
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
        return true;
    });
    std::sort(out.begin(), out.end(), [](const Candidate& a, const Candidate& b) { return a.dist < b.dist; });
    return out;
}

Result resolve(const post_fx::DepthSample& s, float max_dist) {
    Result r;
    if (!s.ok) return r;
    std::vector<Candidate> c = score_units(s.world);
    if (c.size() > 5) c.resize(5);
    r.candidates = std::move(c);
    if (s.unit && alive(s.unit)) {               // the model that drew the pixel (its ID in the depth): exact
        r.unit = s.unit;
        r.by = selection::By::Id;
    } else if (!r.candidates.empty() && r.candidates[0].dist <= max_dist) {
        r.unit = r.candidates[0].unit;           // no ID there: the nearest joint
        r.by = selection::By::Joint;
    }
    return r;
}

int line_of(uintptr_t unit) {
    int found = -1;
    if (!unit) return found;
    each_unit([&](uintptr_t u, int line, const cUnit&) {
        if (u != unit) return true;
        found = line;
        return false;
    });
    return found;
}

bool alive(uintptr_t unit) { return line_of(unit) >= 0 || tags::drawn_lately(unit); }

} // namespace picker

// ---------------------------------------------------------------------------------------------------------------
using namespace picker;

namespace {

bool  g_enabled = false;
float g_max_dist = 60.0f;
bool  g_gizmo = true;

// the pick in flight (Ctrl + click): PostFX answers it from the next frame drawn
std::shared_future<post_fx::DepthSample> g_pending;
int g_pending_frames = 0;
std::string g_pick_note;                   // the last pick's outcome, for the window
std::vector<Candidate> g_candidates;       // the last pick's, nearest first

// what the window shows about the selection, read on the game thread
struct View {
    uintptr_t          unit = 0;
    int                line = -1;
    std::string        cls, origin;
    bool               coord = false;      // a uCoord: has mPos / mWmat
    float              pos[3] = {}, world[3] = {};
    uintptr_t          parent = 0;
    bool               owned = false;
    schedulers::Owner  owner;
};
View g_view;
uint32_t g_view_rev = 0;
float g_moved[3] = {};                     // the gizmo's moves since the selection changed (for an anchor at the picked point)
int g_owner_age = 0;

std::string hex_of(uintptr_t v) {
    char b[16];
    std::snprintf(b, sizeof b, "0x%08X", (unsigned)v);
    return b;
}

const char* track_kind(uint8_t type) {
    switch (type) {
    case 6: return "int";
    case 7: return "vector";
    case 8: return "float";
    case 9: return "bool";
    case 10: return "ref";
    case 11: return "resource";
    case 12: return "string";
    case 13: return "event";
    case 14: return "custom";
    default: return "?";
    }
}

void refresh_view() {
    const uintptr_t unit = selection::unit();
    const uint32_t rev = selection::revision();
    const bool changed = unit != g_view.unit || rev != g_view_rev;
    g_view_rev = rev;
    if (!unit) { g_view = View{}; return; }
    const int line = line_of(unit);
    if (line < 0 && !tags::drawn_lately(unit)) {   // gone (killed, a room change): nothing to point at
        selection::clear();
        g_view = View{};
        g_pick_note = "the selected unit is gone";
        return;
    }
    if (changed) {
        g_moved[0] = g_moved[1] = g_moved[2] = 0.0f;
        g_view = View{};
        g_view.unit = unit;
        const uintptr_t dti = dti_of(unit);
        g_view.cls = dti ? dti_name(dti) : "?";
        g_view.coord = dti && dti_is_a(dti, "ucoord");
        g_view.origin = unit_packs::origin(unit);
        g_owner_age = 0;
    }
    g_view.line = line;
    if (g_view.coord) {
        safe_read(unit + kPos, g_view.pos, sizeof g_view.pos);
        safe_read(unit + kWmat + 0x30, g_view.world, sizeof g_view.world);
        g_view.parent = rd_ptr(unit + kParent);
    }
    if (g_owner_age-- <= 0) {              // the track scan: on a change, then twice a second
        g_owner_age = 30;
        g_view.owned = schedulers::owner_of(unit, g_view.owner);
    }
}

void finish_pick(const post_fx::DepthSample& s) {
    if (!s.ok) { g_pick_note = s.error; return; }
    selection::set_point(s.world);
    const Result r = resolve(s, g_max_dist);
    g_candidates = r.candidates;
    selection::select(r.unit, r.by);
    char b[160];
    if (r.unit)
        std::snprintf(b, sizeof b, "picked by %s at %.0f %.0f %.0f", selection::by_name(r.by), s.world[0], s.world[1], s.world[2]);
    else
        std::snprintf(b, sizeof b, "no unit there (%.0f %.0f %.0f): the point is kept", s.world[0], s.world[1], s.world[2]);
    g_pick_note = b;
}

// moves the selection by a world-space delta: mPos is in its parent's frame (row vectors: world = local x parent)
void move_by(const glm::vec3& d) {
    const uintptr_t u = g_view.unit;
    glm::vec3 local = d;
    if (g_view.parent) {
        float pm[16];
        if (!safe_read(g_view.parent + kWmat, pm, sizeof pm)) return;
        glm::mat4 P;
        memcpy(&P[0][0], pm, sizeof pm);   // MtMatrix rows = glm columns: world = P * local
        local = glm::inverse(glm::mat3(P)) * d;
    }
    float* pos = reinterpret_cast<float*>(u + kPos);
    float* w = reinterpret_cast<float*>(u + kWmat + 0x30);   // shown right away, also while paused
    for (int i = 0; i < 3; ++i) { pos[i] += local[i]; w[i] += d[i]; }
}

void draw_gizmo() {
    if (!g_gizmo || !g_view.unit || !g_view.coord || g_view.line < 0) return;
    if (g_view.owned && (g_view.owner.keys("mPos"))) return;   // its scheduler puts it back every tick
    float view[16], proj[16];
    w2s::ImGuizmoSetup();
    if (!w2s::GetImGuizmoMatrices(view, proj)) return;
    float m[16];
    if (!safe_read(g_view.unit + kWmat, m, sizeof m)) return;
    for (int i = 0; i < 3; ++i) {          // translate only, without the unit's scale or turn
        for (int j = 0; j < 4; ++j) m[i * 4 + j] = (i == j) ? 1.0f : 0.0f;
    }
    // a room's static models sit at the origin with their geometry in world space: the arrows go where it was picked
    float p[3];
    if (std::fabs(m[12]) + std::fabs(m[13]) + std::fabs(m[14]) < 1.0f && selection::point(p)) {
        m[12] = p[0] + g_moved[0];
        m[13] = p[1] + g_moved[1];
        m[14] = p[2] + g_moved[2];
    }
    ImGuizmo::SetID(0x5E1EC7);
    float before[3] = { m[12], m[13], m[14] };
    if (ImGuizmo::Manipulate(view, proj, ImGuizmo::TRANSLATE, ImGuizmo::WORLD, m)) {
        const glm::vec3 d(m[12] - before[0], m[13] - before[1], m[14] - before[2]);
        move_by(d);
        for (int i = 0; i < 3; ++i) g_moved[i] += d[i];
    }
}

void draw_window() {
    ImGui::SetNextWindowSize(ImVec2(440.0f, 380.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_("Object picker"), &g_enabled)) { ImGui::End(); return; }
    ImGui::TextWrapped(_("Ctrl + click in the game to pick what's there (opaque models and stage geometry; not effects or "
                         "transparent meshes)."));
    if (g_pending.valid()) ImGui::TextDisabled(_("picking..."));
    else if (!g_pick_note.empty()) ImGui::TextDisabled("%s", g_pick_note.c_str());
    float p[3];
    if (selection::point(p)) {
        ImGui::Text(_("Point: %.0f %.0f %.0f"), p[0], p[1], p[2]);
        ImGui::SameLine();
        Mod::help_marker(_("The last picked spot in the world (game cm, Y up). Unit packs can spawn there (Spawn at: the "
                           "picked point)."));
    }
    ImGui::SeparatorText(_("Selected"));
    const View& v = g_view;
    if (!v.unit) {
        ImGui::TextDisabled(_("nothing"));
    } else {
        ImGui::Text("%s  %s", v.cls.c_str(), hex_of(v.unit).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(_("Copy"))) ImGui::SetClipboardText(hex_of(v.unit).c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton(_("Clear"))) selection::clear();
        if (v.line >= 0) ImGui::Text(_("Move line %d, chosen by %s"), v.line, selection::by_name(selection::by()));
        else ImGui::Text(_("A part another unit draws (no move line), chosen by %s"), selection::by_name(selection::by()));
        if (v.coord) {
            ImGui::Text(_("World: %.0f %.0f %.0f"), v.world[0], v.world[1], v.world[2]);
            if (v.parent) ImGui::Text(_("mPos: %.0f %.0f %.0f (in its parent %s)"), v.pos[0], v.pos[1], v.pos[2], hex_of(v.parent).c_str());
        }
        if (!v.origin.empty()) ImGui::Text(_("Unit packs: %s"), v.origin.c_str());
        if (v.owned) {
            const schedulers::Owner& o = v.owner;
            ImGui::Text(_("Made by %s, track %d \"%s\""), o.path.c_str(), o.unit.index, o.unit.name.c_str());
            if (!o.keyed.empty() && ImGui::TreeNode(_("Keyed by its .sdl"))) {
                for (const schedulers::Track& t : o.keyed) ImGui::BulletText("%d  %s  (%s)", t.index, t.name.c_str(), track_kind(t.type));
                ImGui::TreePop();
            }
        }
        if (v.coord && v.line >= 0) {
            ImGui::Checkbox(_("Move with a gizmo"), &g_gizmo);
            ImGui::SameLine();
            Mod::help_marker(_("Drag the arrows to move it (its mPos; the game may move it again: enemies walk, players too). A "
                               "unit whose .sdl keys mPos gets no gizmo: its scheduler puts it back every frame, so moving it "
                               "means moving its keys (the stage editor's job)."));
            if (g_gizmo && v.owned && v.owner.keys("mPos")) ImGui::TextDisabled(_("its .sdl keys mPos: no gizmo"));
        }
    }
    if (g_candidates.size() > 1 && ImGui::TreeNode(_("Near the last pick"))) {
        for (size_t i = 0; i < g_candidates.size(); ++i) {
            const Candidate& c = g_candidates[i];
            char label[160];
            std::snprintf(label, sizeof label, "%s  %s  line %d, %.0f cm%s##%u", dti_name(c.dti).c_str(), hex_of(c.unit).c_str(),
                          c.line, c.dist, c.joint < 0 ? _(" (its position)") : "", (unsigned)i);
            if (ImGui::Selectable(label, c.unit == v.unit)) selection::select(c.unit, selection::By::Joint);
        }
        ImGui::TreePop();
    }
    ImGui::SliderFloat(_("Joint reach"), &g_max_dist, 10.0f, 300.0f, "%.0f cm");
    ImGui::SameLine();
    Mod::help_marker(_("Where the pixel carries no object ID (stage geometry, untagged models), the model with a joint this close "
                       "to the picked point is taken instead."));
    ImGui::End();
}

} // namespace

std::optional<std::string> ObjectPicker::on_initialize() {
    schedulers::install();
    return Mod::on_initialize();
}

void ObjectPicker::on_frame(fmilliseconds& dt) {
    static uintptr_t s_room = 0;
    static int s_room_no = -1;
    sArea* area = devil4_sdk::get_sArea();
    aRoom* room = area ? area->aRoomPtr : nullptr;
    const int room_no = room ? (int)room->room_no : -1;
    if (reinterpret_cast<uintptr_t>(room) != s_room || room_no != s_room_no) {   // another room: its points mean nothing here
        s_room = reinterpret_cast<uintptr_t>(room);
        s_room_no = room_no;
        selection::forget_point();
        g_candidates.clear();
    }
    if (g_pending.valid()) {
        if (g_pending.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            finish_pick(g_pending.get());
            g_pending = {};
        } else if (++g_pending_frames > 120) {   // no frame drew the scene (a menu, loading): give up on it
            g_pending = {};
            g_pick_note = "no frame drew the scene to pick from";
        }
    }
    refresh_view();
    if (!g_enabled || !g_framework->m_draw_ui) return;
    draw_gizmo();
    const ImGuiIO& io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse && !ImGuizmo::IsOver() &&
        !ImGuizmo::IsUsing() && !g_pending.valid()) {
        g_pending = post_fx::request_sample((int)io.MousePos.x, (int)io.MousePos.y);
        g_pending_frames = 0;
        if (!g_pending.valid()) g_pick_note = "busy: another pick is waiting";
    }
    draw_window();
}

void ObjectPicker::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Object picker"), &g_enabled);
    ImGui::SameLine();
    help_marker(_("Ctrl + click in the game to select what's under the cursor; a window shows what it is (class, position, the "
                  ".sdl track that made it, the unit pack it came from) and a gizmo moves it. The selection is shared: the "
                  "outline (Anti-aliasing > Outline the picked object), Unit packs and the debug server all use it. Opens its "
                  "own window while this menu is open."));
}

void ObjectPicker::on_config_load(const utility::Config& cfg) {
    g_enabled = cfg.get<bool>("object_picker").value_or(false);
    g_max_dist = cfg.get<float>("object_picker_reach").value_or(60.0f);
    g_gizmo = cfg.get<bool>("object_picker_gizmo").value_or(true);
}

void ObjectPicker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("object_picker", g_enabled);
    cfg.set<float>("object_picker_reach", g_max_dist);
    cfg.set<bool>("object_picker_gizmo", g_gizmo);
}
