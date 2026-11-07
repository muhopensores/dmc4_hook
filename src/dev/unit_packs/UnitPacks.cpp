#include "UnitPacks.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MtMemoryStream.hpp"
#include "../../misc/unit_packs/UnitPackManifest.hpp"
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <map>
#include <mutex>

namespace unit_packs {

namespace {

constexpr uintptr_t kArchiveNewInstance = 0x008E1500;   // rArchive::MyDTI::newInstance: no arguments, eax = the arc (0x70 bytes) or 0
constexpr uintptr_t kPath = 0x04, kRefs = 0x44, kAttr = 0x48, kState = 0x4C, kSize = 0x50, kId = 0x58;   // cResource
constexpr uintptr_t kArcRows = 0x60, kArcCount = 0x64;   // rArchive: cResource* rows[], count
constexpr uint32_t  kAttrSyncRows = 0x200;               // rArchive::load creates every row before returning
constexpr uint32_t  kArcData = 0x8000;                   // where the data starts in the game's arcs
constexpr uint16_t  kArcVersion = 17;
constexpr uintptr_t kPos = 0x30;                         // uCoord::mPos (cm, game axes)
constexpr uintptr_t kStageSetSchedulerLoop = 0x138D;     // uStageSetScheduler::mLoop (st405's .pla sets 1)

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

#pragma pack(push, 1)
struct TocRow {
    char     path[64];
    uint32_t dti, stored, size, offset;   // size: bits 0-28 decompressed size, 29-31 quality tier (kept as the source has it)
};
#pragma pack(pop)
static_assert(sizeof(TocRow) == 0x50, "arc TOC row");

std::vector<Loaded> g_loaded(kPackCount);

// Read the pack's rows out of their source arcs and lay them out as one ARC: header, TOC (the source rows with new
// offsets), data from kArcData, each row's stored bytes unchanged. Rows keep the pack's order (a nested list after
// its members: the game's ARCS loader only looks its ids up).
bool build_arc(const Pack& p, std::vector<uint8_t>& out, std::string& error) {
    std::vector<TocRow> toc(p.row_count);
    std::vector<std::vector<uint8_t>> data(p.row_count);
    std::map<std::string, std::vector<int>> by_arc;
    for (int i = 0; i < p.row_count; ++i) by_arc[p.rows[i].arc].push_back(i);

    for (const auto& [arc, wanted] : by_arc) {
        const std::string file = std::string("nativePC\\") + arc;
        FILE* f = nullptr;
        if (fopen_s(&f, file.c_str(), "rb") != 0 || !f) { error = "can't open " + file; return false; }
        char magic[4] = {};
        uint16_t version = 0, count = 0;
        const bool head_ok = fread(magic, 1, 4, f) == 4 && fread(&version, 2, 1, f) == 1 && fread(&count, 2, 1, f) == 1;
        if (!head_ok || memcmp(magic, "ARC\0", 4) != 0 || version != kArcVersion) {
            fclose(f);
            error = file + " is not a DX9 arc (ARC v17)";
            return false;
        }
        std::vector<TocRow> rows(count);
        if (fread(rows.data(), sizeof(TocRow), count, f) != count) { fclose(f); error = "short TOC in " + file; return false; }
        for (int i : wanted) {
            const PackRow& want = p.rows[i];
            const TocRow* hit = nullptr;
            for (const TocRow& r : rows)
                if (r.dti == want.dti && _strnicmp(r.path, want.path, sizeof(r.path)) == 0) { hit = &r; break; }
            if (!hit) { fclose(f); error = std::string(want.path) + " is not in " + file + " (another version of the arc?)"; return false; }
            data[i].resize(hit->stored);
            if (fseek(f, (long)hit->offset, SEEK_SET) != 0 || fread(data[i].data(), 1, hit->stored, f) != hit->stored) {
                fclose(f);
                error = std::string("can't read ") + want.path + " from " + file;
                return false;
            }
            toc[i] = *hit;
        }
        fclose(f);
    }

    size_t total = kArcData;
    for (const auto& d : data) total += d.size();
    out.assign(total, 0);
    memcpy(out.data(), "ARC\0", 4);
    *reinterpret_cast<uint16_t*>(out.data() + 4) = kArcVersion;
    *reinterpret_cast<uint16_t*>(out.data() + 6) = (uint16_t)p.row_count;
    uint32_t off = kArcData;
    for (int i = 0; i < p.row_count; ++i) {
        toc[i].offset = off;
        memcpy(out.data() + 8 + i * sizeof(TocRow), &toc[i], sizeof(TocRow));
        memcpy(out.data() + off, data[i].data(), data[i].size());
        off += (uint32_t)data[i].size();
    }
    return true;
}

} // namespace

const Loaded& state(int pack) { return g_loaded[pack]; }

bool load(int pack) {
    if (pack < 0 || pack >= kPackCount) return false;
    Loaded& st = g_loaded[pack];
    if (st.arc) return true;
    st = Loaded{};
    const Pack& p = kPacks[pack];
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<uint8_t> bytes;
    if (!build_arc(p, bytes, st.error)) {
        spdlog::warn("UnitPacks: {}: {}", p.name, st.error);
        return false;
    }
    const uintptr_t arc = reinterpret_cast<uintptr_t(*)()>(kArchiveNewInstance)();   // refcount 1, ours
    if (!arc) { st.error = "rArchive allocation failed"; return false; }
    std::snprintf(&at<char>(arc, kPath), 64, "unit_packs\\%s", p.name);   // a name for FileTracker / debuggers
    at<uint64_t>(arc, kId) = 0;                                            // not in sResource's table
    at<uint32_t>(arc, kAttr) |= kAttrSyncRows;
    MtMemoryStream stream(bytes.data(), (uint32_t)bytes.size());
    const bool ok = load_resource_from((void*)arc, stream);                // rArchive::load 0x8E17B0
    const uintptr_t rows = at<uintptr_t>(arc, kArcRows);
    const uint32_t  n    = at<uint32_t>(arc, kArcCount);
    for (uint32_t k = 0; rows && k < n; ++k) {
        const uintptr_t r = at<uintptr_t>(rows, 4 * k);
        if (!r) { ++st.failed; continue; }
        ++st.rows;
    }
    st.failed += p.row_count - (int)n;
    st.bytes = (uint32_t)bytes.size();
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (!ok || st.failed) {
        st.error = !ok ? "the game refused the arc (a row's class not found?)" : std::to_string(st.failed) + " rows not created";
        spdlog::warn("UnitPacks: {}: {} ({} rows, {} failed)", p.name, st.error, st.rows, st.failed);
        devil4_sdk::release_resource((cResource*)arc);                     // its unload releases the rows it made
        st.arc = 0;
        return false;
    }
    at<uint32_t>(arc, kSize) = st.bytes;                                   // as create_file_load: size, state "loaded"
    at<uint32_t>(arc, kState) |= 1;
    st.arc = arc;
    spdlog::info("UnitPacks: {} loaded: {} rows, {} bytes, {:.1f} ms", p.name, st.rows, st.bytes, st.ms);
    return true;
}

void release(int pack) {
    if (pack < 0 || pack >= kPackCount) return;
    Loaded& st = g_loaded[pack];
    if (st.arc) devil4_sdk::release_resource((cResource*)st.arc);         // last reference: rows released (vtbl +0x2C)
    st = Loaded{};
}

// What the room's .pla does for the laser: a unit of the pack's spawn class with its resource field set, put on its move
// line; its setup (first move) does the rest. The reference from sResource::create goes to the unit.
uintptr_t spawn(int pack, int resource, const float pos[3], std::string& error) {
    if (pack < 0 || pack >= kPackCount || !kPacks[pack].spawn) { error = "no spawn for this pack"; return 0; }
    const PackSpawn& s = *kPacks[pack].spawn;
    if (resource < 0 || resource >= s.resource_count) { error = "no such resource"; return 0; }
    if (!devil4_sdk::get_sUnit()) { error = "no sUnit"; return 0; }
    // 0x80: cache only. The pack must be loaded (or the room is the unit's own); never fall back to a disk load
    const uintptr_t res = (uintptr_t)devil4_sdk::get_stuff_from_files((MtDTI*)s.resource_dti, (char*)s.resources[resource], 0x80);
    if (!res) { error = std::string(s.resources[resource]) + " is not loaded: load the pack first"; return 0; }
    const uintptr_t unit = reinterpret_cast<uintptr_t(*)()>(s.new_instance)();
    if (!unit) { devil4_sdk::release_resource((cResource*)res); error = "allocation failed"; return 0; }
    at<uintptr_t>(unit, s.field) = res;
    float* p = &at<float>(unit, kPos);
    p[0] = pos[0]; p[1] = pos[1]; p[2] = pos[2];
    if (strcmp(s.cls, "uStageSetScheduler") == 0) at<uint8_t>(unit, kStageSetSchedulerLoop) = 1;
    devil4_sdk::sUnit_spawn((void*)unit, s.move_line);
    spdlog::info("UnitPacks: spawned {} with {} on line {} at {:.0f} {:.0f} {:.0f}", s.cls, s.resources[resource], s.move_line, pos[0], pos[1], pos[2]);
    return unit;
}

} // namespace unit_packs

// ---------------------------------------------------------------------------------------------------------------
using namespace unit_packs;

namespace {
bool g_enabled = false;
int  g_pick[64] = {};            // per pack: the spawn resource picked in the panel
std::vector<Spawned> g_spawned;
std::string g_last_error;
int g_prune = 0;

struct Request { enum Kind { LOAD, RELEASE, SPAWN, DESPAWN } kind; int pack = 0, resource = 0; uintptr_t unit = 0; };
std::mutex g_req_lock;
std::vector<Request> g_requests;

void queue(Request r) { std::lock_guard<std::mutex> lock(g_req_lock); g_requests.push_back(r); }

bool alive(const Spawned& s) { return devil4_sdk::check_exists_in_moveline((void*)s.unit, s.line); }
} // namespace

std::optional<std::string> UnitPacks::on_initialize() { return Mod::on_initialize(); }

void UnitPacks::on_frame(fmilliseconds& dt) {
    std::vector<Request> todo;
    {
        std::lock_guard<std::mutex> lock(g_req_lock);
        todo.swap(g_requests);
    }
    for (const Request& r : todo) {
        switch (r.kind) {
        case Request::LOAD:
            if (!load(r.pack)) g_last_error = state(r.pack).error;
            break;
        case Request::RELEASE:
            release(r.pack);
            break;
        case Request::SPAWN: {
            uPlayer* pl = devil4_sdk::get_local_player();
            if (!pl) { g_last_error = "no player"; break; }
            const float* pp = reinterpret_cast<const float*>((uintptr_t)pl + 0x30);   // uCoord::mPos
            std::string err;
            const uintptr_t u = spawn(r.pack, r.resource, pp, err);
            if (u) g_spawned.push_back(Spawned{ u, r.pack, r.resource, kPacks[r.pack].spawn->move_line });
            else g_last_error = err;
            break;
        }
        case Request::DESPAWN:
            for (size_t i = 0; i < g_spawned.size(); ++i) {
                if (g_spawned[i].unit != r.unit) continue;
                if (alive(g_spawned[i])) uactor_sdk::despawn((void*)g_spawned[i].unit);   // its kill state; the game frees it
                g_spawned.erase(g_spawned.begin() + i);
                break;
            }
            break;
        }
    }
    if (++g_prune >= 30) {               // units the game already removed (room change, their own end)
        g_prune = 0;
        for (size_t i = 0; i < g_spawned.size();) {
            if (alive(g_spawned[i])) ++i;
            else g_spawned.erase(g_spawned.begin() + i);
        }
    }
}

void UnitPacks::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Unit packs"), &g_enabled);
    ImGui::SameLine();
    help_marker(_("Loads a stage gadget's files (the temple laser, ...) from the room it ships with into the game's file cache, so "
                  "it can be spawned in any room. Only the files it needs are read, from the game's own arcs; nothing is written. "
                  "Release frees them again (units already spawned keep what they use). For the stage editor."));
    if (!g_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("UnitPacks");
    for (int i = 0; i < kPackCount; ++i) {
        const Pack& p = kPacks[i];
        const Loaded& st = state(i);
        ImGui::PushID(i);
        ImGui::Spacing();
        ImGui::TextUnformatted(p.name);
        ImGui::SameLine();
        help_marker(p.about);
        ImGui::SameLine();
        if (!st.arc) {
            if (ImGui::Button(_("Load"))) queue(Request{ Request::LOAD, i });
        } else if (ImGui::Button(_("Release"))) queue(Request{ Request::RELEASE, i });
        ImGui::SameLine();
        if (st.arc)
            ImGui::Text(_("%d files, %.1f KB, %.1f ms"), st.rows, st.bytes / 1024.0, st.ms);
        else
            ImGui::TextDisabled(_("%d files from %s"), p.row_count, p.room);
        if (!st.error.empty()) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", st.error.c_str());

        if (p.spawn && st.arc) {
            const PackSpawn& s = *p.spawn;
            int& pick = g_pick[i % 64];
            if (pick >= s.resource_count) pick = 0;
            ImGui::PushItemWidth(260.0f);
            if (ImGui::BeginCombo("##res", s.resources[pick])) {
                for (int k = 0; k < s.resource_count; ++k)
                    if (ImGui::Selectable(s.resources[k], k == pick)) pick = k;
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(_("Spawn"))) queue(Request{ Request::SPAWN, i, pick });
            ImGui::SameLine();
            help_marker(_("Spawns the unit the way its room does (for the laser: a stage scheduler playing the chosen .sdl, which "
                          "creates the laser). The .sdl's positions are the original room's world coordinates, so in another room "
                          "the laser appears where it stood in that room."));
        }
        ImGui::PopID();
    }
    if (!g_spawned.empty()) {
        ImGui::Spacing();
        ImGui::TextUnformatted(_("Spawned"));
        for (size_t k = 0; k < g_spawned.size(); ++k) {
            const Spawned& s = g_spawned[k];
            ImGui::PushID((int)k);
            ImGui::Text("%s  %s", kPacks[s.pack].spawn->cls, kPacks[s.pack].spawn->resources[s.resource]);
            ImGui::SameLine();
            if (ImGui::SmallButton(_("Despawn"))) queue(Request{ Request::DESPAWN, s.pack, s.resource, s.unit });
            ImGui::PopID();
        }
    }
    if (!g_last_error.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", g_last_error.c_str());
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void UnitPacks::on_config_load(const utility::Config& cfg) {
    g_enabled = cfg.get<bool>("unit_packs").value_or(false);
}

void UnitPacks::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("unit_packs", g_enabled);
}
