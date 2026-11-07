#include "UnitPacks.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/MtMemoryStream.hpp"
#include "../../sdk/cUnit.hpp"
#include "../../sdk/sUnit.hpp"
#include "../../misc/unit_packs/UnitPackManifest.hpp"
#include "../../misc/unit_packs/UnitPackRooms.hpp"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <mutex>
#include <pugixml.hpp>

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

struct RowRef { const char* path; uint32_t dti; const char* arc; };

// Read the rows out of their source arcs and lay them out as one ARC: header, TOC (the source rows with new offsets),
// data from kArcData, each row's stored bytes unchanged. Rows keep their order (a nested list after its members: the
// game's ARCS loader only looks its ids up).
bool build_arc(const std::vector<RowRef>& want_rows, std::vector<uint8_t>& out, std::string& error) {
    const int count = (int)want_rows.size();
    std::vector<TocRow> toc(count);
    std::vector<std::vector<uint8_t>> data(count);
    std::map<std::string, std::vector<int>> by_arc;
    for (int i = 0; i < count; ++i) by_arc[want_rows[i].arc].push_back(i);

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
            const RowRef& want = want_rows[i];
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
    *reinterpret_cast<uint16_t*>(out.data() + 6) = (uint16_t)count;
    uint32_t off = kArcData;
    for (int i = 0; i < count; ++i) {
        toc[i].offset = off;
        memcpy(out.data() + 8 + i * sizeof(TocRow), &toc[i], sizeof(TocRow));
        memcpy(out.data() + off, data[i].data(), data[i].size());
        off += (uint32_t)data[i].size();
    }
    return true;
}

} // namespace

// Load `rows_in` as one in-memory arc called unit_packs\<name>; `st` gets the result (arc 0 = failed).
bool load_rows(const std::string& name, const std::vector<RowRef>& rows_in, Loaded& st) {
    if (st.arc) return true;
    st = Loaded{};
    const auto t0 = std::chrono::steady_clock::now();
    std::vector<uint8_t> bytes;
    if (!build_arc(rows_in, bytes, st.error)) {
        spdlog::warn("UnitPacks: {}: {}", name, st.error);
        return false;
    }
    const uintptr_t arc = reinterpret_cast<uintptr_t(*)()>(kArchiveNewInstance)();   // refcount 1, ours
    if (!arc) { st.error = "rArchive allocation failed"; return false; }
    std::snprintf(&at<char>(arc, kPath), 64, "unit_packs\\%s", name.c_str());   // a name for FileTracker / debuggers
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
    st.failed += (int)rows_in.size() - (int)n;
    st.bytes = (uint32_t)bytes.size();
    st.ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (!ok || st.failed) {
        st.error = !ok ? "the game refused the arc (a row's class not found?)" : std::to_string(st.failed) + " rows not created";
        spdlog::warn("UnitPacks: {}: {} ({} rows, {} failed)", name, st.error, st.rows, st.failed);
        devil4_sdk::release_resource((cResource*)arc);                     // its unload releases the rows it made
        st.arc = 0;
        return false;
    }
    at<uint32_t>(arc, kSize) = st.bytes;                                   // as create_file_load: size, state "loaded"
    at<uint32_t>(arc, kState) |= 1;
    st.arc = arc;
    spdlog::info("UnitPacks: {} loaded: {} rows, {} bytes, {:.1f} ms", name, st.rows, st.bytes, st.ms);
    return true;
}

void release_loaded(Loaded& st) {
    if (st.arc) devil4_sdk::release_resource((cResource*)st.arc);         // last reference: rows released (vtbl +0x2C)
    st = Loaded{};
}

const Loaded& state(int pack) { return g_loaded[pack]; }

bool load(int pack) {
    if (pack < 0 || pack >= kPackCount) return false;
    const Pack& p = kPacks[pack];
    std::vector<RowRef> rows;
    for (int i = 0; i < p.row_count; ++i) rows.push_back(RowRef{ p.rows[i].path, p.rows[i].dti, p.rows[i].arc });
    return load_rows(p.name, rows, g_loaded[pack]);
}

void release(int pack) {
    if (pack < 0 || pack >= kPackCount) return;
    release_loaded(g_loaded[pack]);
}

// ---- The rooms manifest: misc/unit_packs/rooms/<room>.xml, built in through UnitPackRooms (both generated by
// Vibed\StageEditor\unit_arcs\scripts\emit_unit_manifest.py, format in its docstring). Per room a <file> table (the
// rows, by index) and the units the room's .pla / .sdl place, each with the indices of the rows it needs.

std::vector<Room> g_rooms;
std::string g_manifest_error;

// A unit element -> its .pla tracks, in document order (the elements nest as the tracks' parents do), the same steps
// as Vibed\StageEditor\unit_arcs\scripts\check_manifest_pla.py, which checks them against every unit's source .pla.
void parse_tracks(const pugi::xml_node x, Unit& u) {
    PlaTrack root;
    root.type = 1;
    root.name = "Root";
    PlaTrack unit;
    unit.type  = 4;
    unit.prop  = x.attribute("prop").as_uint();
    unit.line  = x.attribute("line").as_uint();
    unit.index = (uint32_t)std::strtoul(x.attribute("dti").as_string(), nullptr, 16);
    unit.name  = x.attribute("name").as_string();
    u.tracks = { root, unit };
    static const std::pair<const char*, uint32_t> kTypes[] = {
        { "bool", 6 }, { "int", 7 }, { "float", 8 }, { "vector", 9 }, { "ref", 10 }, { "resource", 11 }, { "object", 5 } };
    std::function<void(pugi::xml_node, uint32_t, int)> walk = [&](pugi::xml_node el, uint32_t parent, int depth) {
        for (pugi::xml_node c : el.children()) {
            PlaTrack t;
            for (const auto& [tag, type] : kTypes)
                if (strcmp(c.name(), tag) == 0) t.type = type;
            if (!t.type) continue;
            t.parent = parent;
            t.prop   = c.attribute("prop").as_uint();
            t.index  = c.attribute("i").as_uint();
            t.name   = c.attribute("name").as_string();
            std::string shown = std::string(depth * 2, ' ') + c.name() + " " + t.name;
            switch (t.type) {
            case 5:
                u.tracks.push_back(t);
                u.values += shown + "\n";
                walk(c, (uint32_t)u.tracks.size() - 1, depth + 1);
                continue;
            case 11:
                if (c.attribute("v")) {
                    t.rdti = (uint32_t)std::strtoul(c.attribute("rdti").as_string(), nullptr, 16);
                    t.path = c.attribute("v").as_string();
                }
                shown += " = " + (t.path.empty() ? std::string("(none)") : t.path);
                break;
            case 10:
                t.u = c.attribute("track").as_uint();
                shown += " = track " + std::to_string(t.u);
                break;
            case 9:
                sscanf_s(c.attribute("v").as_string(), "%f %f %f %f", &t.v[0], &t.v[1], &t.v[2], &t.v[3]);
                shown += std::string(" = ") + c.attribute("v").as_string();
                break;
            case 8:
                t.v[0] = c.attribute("v").as_float();
                shown += std::string(" = ") + c.attribute("v").as_string();
                break;
            default:   // bool / int: the 4 bytes as the file has them (unsigned or signed alike)
                t.u = (uint32_t)std::strtoll(c.attribute("v").as_string(), nullptr, 10);
                shown += std::string(" = ") + c.attribute("v").as_string();
                break;
            }
            u.tracks.push_back(t);
            u.values += shown + "\n";
        }
    };
    walk(x, 1, 0);
}

const std::vector<Room>& rooms() { return g_rooms; }
const std::string& manifest_error() { return g_manifest_error; }

int read_manifest() {
    for (Room& r : g_rooms)
        for (Unit& u : r.units) release_loaded(u.loaded);
    g_rooms.clear();
    g_manifest_error.clear();
    for (int k = 0; k < kRoomXmlCount; ++k) {
        const RoomXml& x = kRoomXml[k];
        pugi::xml_document doc;
        const pugi::xml_parse_result res = doc.load_buffer(x.data, x.size);   // copies the text: the array stays const
        const pugi::xml_node root = doc.child("unit_packs");
        if (!res || !root) {
            g_manifest_error += std::string(x.room) + ": " + (res ? "no <unit_packs>" : res.description()) + "\n";
            continue;
        }
        Room room;
        room.name = root.attribute("room").as_string();
        room.arc  = root.attribute("arc").as_string();
        for (pugi::xml_node f : root.children("file")) {
            File file;
            file.path   = f.attribute("path").as_string();
            file.type   = f.attribute("type").as_string();
            file.dti    = (uint32_t)std::strtoul(f.attribute("dti").as_string(), nullptr, 16);
            file.arc    = f.attribute("arc") ? f.attribute("arc").as_string() : room.arc;
            file.stored = f.attribute("stored").as_uint();
            file.global = f.attribute("global").as_bool();
            room.files.push_back(std::move(file));
        }
        for (pugi::xml_node x : root.children("unit")) {
            Unit u;
            u.name   = x.attribute("name").as_string();
            u.cls    = x.attribute("class").as_string();
            u.source = x.attribute("source").as_string();
            u.line   = x.attribute("line").as_int();
            u.track  = x.attribute("track").as_int();
            u.stored = x.attribute("stored").as_uint();
            for (const char* p = x.attribute("files").as_string(); *p;) {
                char* end = nullptr;
                const long i = std::strtol(p, &end, 10);
                if (end == p) break;
                if (i >= 0 && i < (long)room.files.size()) u.files.push_back((int)i);
                p = end;
            }
            parse_tracks(x, u);
            room.units.push_back(std::move(u));
        }
        g_rooms.push_back(std::move(room));
    }
    std::sort(g_rooms.begin(), g_rooms.end(), [](const Room& a, const Room& b) { return a.name < b.name; });
    int units = 0;
    for (const Room& r : g_rooms) units += (int)r.units.size();
    spdlog::info("UnitPacks: manifest: {} rooms, {} units", g_rooms.size(), units);
    return units;
}

bool load_unit(int room, int unit) {
    if (room < 0 || room >= (int)g_rooms.size() || unit < 0 || unit >= (int)g_rooms[room].units.size()) return false;
    Room& r = g_rooms[room];
    Unit& u = r.units[unit];
    std::vector<RowRef> rows;
    for (int i : u.files) rows.push_back(RowRef{ r.files[i].path.c_str(), r.files[i].dti, r.files[i].arc.c_str() });
    return load_rows(r.name + "\\" + u.name, rows, u.loaded);
}

void release_unit(int room, int unit) {
    if (room < 0 || room >= (int)g_rooms.size() || unit < 0 || unit >= (int)g_rooms[room].units.size()) return;
    release_loaded(g_rooms[room].units[unit].loaded);
}

// The .pla bytes (dmc4xml/pla.py layout, PLA\0 v17): header 0x10, tracks 0x1C (type, prop, parent, move line, name
// offset, dti / index, value offset), each value at the next 16-byte boundary (4 bytes, 16 for a vector; a resource
// value = the offset of its "u32 dti + path" entry in the name area), then the name area at a 4-byte boundary. With
// `pos`, the unit's mPos value becomes `pos` (added when the unit has none: classes without one skip it).
std::vector<uint8_t> build_pla(const Unit& u, const float* pos) {
    std::vector<PlaTrack> tracks = u.tracks;
    if (pos) {
        bool found = false;
        for (PlaTrack& t : tracks)
            if (t.type == 9 && t.parent == 1 && t.name == "mPos") { memcpy(t.v, pos, 3 * sizeof(float)); found = true; }
        if (!found) {
            PlaTrack t;
            t.type = 9; t.prop = 20; t.parent = 1; t.name = "mPos";   // 20 = MtProperty vector3
            memcpy(t.v, pos, 3 * sizeof(float));
            tracks.push_back(t);
        }
    }
    std::vector<uint8_t> names;
    std::vector<uint32_t> name_ofs, res_ofs;
    auto put = [&](const void* p, size_t n) { names.insert(names.end(), (const uint8_t*)p, (const uint8_t*)p + n); };
    for (const PlaTrack& t : tracks) {
        name_ofs.push_back((uint32_t)names.size());
        put(t.name.c_str(), t.name.size() + 1);
    }
    for (const PlaTrack& t : tracks) {
        res_ofs.push_back(0);
        if (t.type != 11 || t.path.empty()) continue;
        res_ofs.back() = (uint32_t)names.size();
        put(&t.rdti, 4);
        put(t.path.c_str(), t.path.size() + 1);
    }
    const uint32_t base = 0x10 + 0x1C * (uint32_t)tracks.size();
    std::vector<uint8_t> values;
    std::vector<uint32_t> at;
    for (size_t k = 0; k < tracks.size(); ++k) {
        const PlaTrack& t = tracks[k];
        if (t.type < 6) { at.push_back(0); continue; }
        const uint32_t p = base + (uint32_t)values.size(), a = (p + 15) & ~15u;
        values.resize(values.size() + (a - p));
        uint8_t slot[16] = {};
        size_t n = 4;
        if (t.type == 9) { memcpy(slot, t.v, 16); n = 16; }
        else if (t.type == 8) memcpy(slot, t.v, 4);
        else if (t.type == 11) memcpy(slot, &res_ofs[k], 4);
        else memcpy(slot, &t.u, 4);
        values.insert(values.end(), slot, slot + n);
        at.push_back(a);
    }
    const uint32_t end = base + (uint32_t)values.size(), name_area = (end + 3) & ~3u;
    std::vector<uint8_t> out(name_area + names.size(), 0);
    memcpy(out.data(), "PLA\0", 4);
    *reinterpret_cast<uint16_t*>(&out[4]) = 17;
    *reinterpret_cast<uint16_t*>(&out[6]) = (uint16_t)tracks.size();
    *reinterpret_cast<uint32_t*>(&out[12]) = name_area;
    for (size_t k = 0; k < tracks.size(); ++k) {
        const PlaTrack& t = tracks[k];
        const uint32_t row[7] = { t.type, t.prop, t.parent, t.line, name_ofs[k], t.index, at[k] };
        memcpy(&out[0x10 + 0x1C * k], row, sizeof(row));
    }
    memcpy(&out[base], values.data(), values.size());
    memcpy(&out[name_area], names.data(), names.size());
    return out;
}

// rPlacement::MyDTI::newInstance; uPlacement::MyDTI::newInstance and its fields (+0x18 mpPlacement, +0x1C create in
// setup). A uPlacement's setup runs createUnitFromNative 0x7A4930 (per unit track: newInstance + sUnit::addBottom on
// its move line, then every value through the class's MtPropertyList, uPlacement::apply 0x7A4B10), then kills itself
// (vtbl +0x30); its destructor releases the rPlacement. The game does the same for em025 (0x879B2A: +0x1C = 1, line 1).
constexpr uintptr_t kPlacementNew = 0x00472860, kUPlacementNew = 0x007A4650;
constexpr uintptr_t kUPlaRes = 0x18, kUPlaCreate = 0x1C;
constexpr int       kUPlaLine = 1;

uintptr_t spawn_unit(int room, int unit, const float* pos, std::string& error) {
    if (room < 0 || room >= (int)g_rooms.size() || unit < 0 || unit >= (int)g_rooms[room].units.size()) { error = "no such unit"; return 0; }
    const Unit& u = g_rooms[room].units[unit];
    if (u.stored && !u.loaded.arc) { error = u.name + ": load its files first"; return 0; }
    if (u.line < 0 || u.line >= 32) { error = "move line out of range"; return 0; }
    if (!devil4_sdk::get_sUnit()) { error = "no sUnit"; return 0; }
    const std::vector<uint8_t> bytes = build_pla(u, pos);
    const uintptr_t res = reinterpret_cast<uintptr_t(*)()>(kPlacementNew)();   // refcount 1, ours
    if (!res) { error = "rPlacement allocation failed"; return 0; }
    std::snprintf(&at<char>(res, kPath), 64, "unit_packs\\%s\\%s", g_rooms[room].name.c_str(), u.name.c_str());
    at<uint64_t>(res, kId) = 0;                                               // not in sResource's table
    MtMemoryStream stream(bytes.data(), (uint32_t)bytes.size());
    if (!load_resource_from((void*)res, stream)) {                            // rPlacement::decompress 0x472970
        devil4_sdk::release_resource((cResource*)res);
        error = "the game refused the .pla";
        return 0;
    }
    at<uint32_t>(res, kSize) = (uint32_t)bytes.size();
    at<uint32_t>(res, kState) |= 1;
    const uintptr_t upla = reinterpret_cast<uintptr_t(*)()>(kUPlacementNew)();
    if (!upla) { devil4_sdk::release_resource((cResource*)res); error = "uPlacement allocation failed"; return 0; }
    at<uintptr_t>(upla, kUPlaRes) = res;                                      // our reference goes to it
    at<uint8_t>(upla, kUPlaCreate) = 1;
    devil4_sdk::sUnit_spawn((void*)upla, kUPlaLine);
    spdlog::info("UnitPacks: spawning {} ({}) from {}, {} tracks", u.name, u.cls, g_rooms[room].name, u.tracks.size());
    return upla;
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

// LOAD_UNIT / RELEASE_UNIT / SPAWN_UNIT: pack = the manifest room, resource = the unit (SPAWN_UNIT: at pos when
// unit != 0); DESPAWN_UNIT: unit = the created unit
struct Request { enum Kind { LOAD, RELEASE, SPAWN, DESPAWN, WARP, READ_MANIFEST, LOAD_UNIT, RELEASE_UNIT, SPAWN_UNIT,
                             DESPAWN_UNIT } kind; int pack = 0, resource = 0; uintptr_t unit = 0; float pos[3] = {}; };
std::mutex g_req_lock;
std::vector<Request> g_requests;

void queue(Request r) { std::lock_guard<std::mutex> lock(g_req_lock); g_requests.push_back(r); }

bool alive(const Spawned& s) { return devil4_sdk::check_exists_in_moveline((void*)s.unit, s.line); }

// What a spawned stage scheduler made, read on the game thread and drawn from a copy: uStageSetScheduler+0x1384
// mpSdlUnit = its uScheduler; per track state (0x34 bytes, Vibed/RE/sdl_scheduler_runtime.md): +0 the file's track
// (+4 parent = the move line for a unit track, +8 name), +4 low byte the track type, +8 the bound object, +0xC the
// MtProperty copy (+0 name, +6 attr, +0xC field address unless attr 0x80 = accessor).
constexpr uintptr_t kSdlUnit = 0x1384, kSdlTrackCount = 0x18, kSdlStates = 0x2C, kStateSize = 0x34;
constexpr uint8_t   kTrackUnit = 2, kTrackVector = 7;
constexpr uintptr_t kParent = 0x18, kParentJoint = 0x1C, kQuat = 0x40, kLmat = 0x60, kWmat = 0xA0;   // uCoord
struct VecView  { std::string name; float v[3]; };
struct UnitView { std::string name; float pos[3]; float local[3]; std::vector<VecView> vectors; };   // pos: world (mWmat)
struct SpawnView { uintptr_t unit = 0; std::vector<UnitView> units; std::vector<uintptr_t> objs; };
std::vector<SpawnView> g_view;

// Moving what a scheduler made: its units get the spawned uStageSetScheduler as mpParent (joint -1), and
// uCoord::updateWorldMatrix 0xA33070 then gives them mLmat x the parent's mWmat (row vectors; a dead parent is dropped
// there). The laser's collision capsule rides its mWmat too (setAttackColl 0x882F2C: mpRefMat = &mWmat). The .sdl
// keys stay as they are, local to the parent now. anchor = the first unit's first position: it lands on target.
struct Placement { bool active = false; float target[3] = {}; float turn = 0.0f; float anchor[3] = {}; bool have_anchor = false; };
std::map<uintptr_t, Placement> g_place;   // by spawned unit
bool g_manifest_read = false;      // parsed once, the first time the window shows
bool g_spawn_here = true;          // manifest spawns at the player's position (else where the room puts them)

// Units created from the manifest. The uPlacement makes them during its setup, a frame or so after the spawn, so the
// target move line is noted before and compared once the uPlacement is gone: what's new on it is the spawn's.
std::vector<uintptr_t> line_units(int line) {
    std::vector<uintptr_t> out;
    sUnit* su = devil4_sdk::get_sUnit();
    if (!su || line < 0 || line >= 32) return out;
    for (cUnit* c = (cUnit*)su->mMoveLine[line].mTop; c; c = c->mp_next_unit) out.push_back((uintptr_t)c);
    return out;
}
struct Pending { uintptr_t placement; int room, unit, line, frames; std::vector<uintptr_t> before; };
struct Made { uintptr_t unit; int room, index, line; };
std::vector<Pending> g_pending;
std::vector<Made> g_made;
int  g_room_pick = 0;
bool g_room_only = true;           // list only units that need files from a room
ImGuiTextFilter g_unit_filter;
float g_player_pos[3] = {};
bool  g_have_player = false;

std::string text_at(uintptr_t p) { return p ? std::string(reinterpret_cast<const char*>(p)) : std::string("?"); }

SpawnView view_of(const Spawned& s) {
    SpawnView out{ s.unit };
    const uintptr_t sdl = at<uintptr_t>(s.unit, kSdlUnit);
    const uintptr_t states = sdl ? at<uintptr_t>(sdl, kSdlStates) : 0;
    const uint16_t  count = sdl ? at<uint16_t>(sdl, kSdlTrackCount) : 0;
    if (!states || count > 4096) return out;
    std::vector<uintptr_t> units;   // the created units still alive, in the order of out.units
    for (uint16_t k = 0; k < count; ++k) {
        const uintptr_t st = states + k * kStateSize, track = at<uintptr_t>(st, 0), obj = at<uintptr_t>(st, 8);
        if (!track || !obj || at<uint8_t>(st, 4) != kTrackUnit) continue;
        if (!devil4_sdk::check_exists_in_moveline((void*)obj, (int)at<uint32_t>(track, 4))) continue;
        const float* p = &at<float>(obj, kPos);
        const float* w = &at<float>(obj, kWmat + 0x30);   // the world matrix's translation row
        out.units.push_back(UnitView{ text_at(at<uintptr_t>(track, 8)), { w[0], w[1], w[2] }, { p[0], p[1], p[2] } });
        units.push_back(obj);
    }
    for (uint16_t k = 0; k < count; ++k) {
        const uintptr_t st = states + k * kStateSize, obj = at<uintptr_t>(st, 8);
        if (at<uint8_t>(st, 4) != kTrackVector || (at<uint16_t>(st, 0xC + 6) & 0x80)) continue;
        const auto it = std::find(units.begin(), units.end(), obj);
        const uintptr_t field = at<uintptr_t>(st, 0xC + 0xC);
        if (it == units.end() || !field) continue;
        const float* v = reinterpret_cast<const float*>(field);
        out.units[it - units.begin()].vectors.push_back(VecView{ text_at(at<uintptr_t>(st, 0xC)), { v[0], v[1], v[2] } });
    }
    out.objs = units;
    return out;
}

// The spawned scheduler's frame: a turn about the up axis, then target - turned anchor, so the anchor lands on target.
// Written into its mPos / mQuat (what its own update builds from) and its mLmat / mWmat (in case a scheduler without
// a model never updates them), then the created units are parented to it. Off: the units are let go again.
void apply_placement(const Spawned& s, const SpawnView& v) {
    Placement& pl = g_place[s.unit];
    if (!pl.have_anchor && !v.units.empty()) {
        memcpy(pl.anchor, v.units[0].local, sizeof(pl.anchor));
        pl.have_anchor = true;
    }
    if (!pl.active) {
        for (uintptr_t obj : v.objs)
            if (at<uintptr_t>(obj, kParent) == s.unit) at<uintptr_t>(obj, kParent) = 0;
        return;
    }
    const float a = pl.turn * 3.14159265f / 180.0f, c = std::cos(a), sn = std::sin(a);
    const float* an = pl.anchor;
    const float t[3] = { pl.target[0] - (an[0] * c + an[2] * sn), pl.target[1] - an[1], pl.target[2] - (-an[0] * sn + an[2] * c) };
    const float m[16] = { c, 0, -sn, 0,  0, 1, 0, 0,  sn, 0, c, 0,  t[0], t[1], t[2], 1 };
    float* pos = &at<float>(s.unit, kPos);
    pos[0] = t[0]; pos[1] = t[1]; pos[2] = t[2];
    float* q = &at<float>(s.unit, kQuat);   // x y z w: the same turn about Y
    q[0] = 0.0f; q[1] = std::sin(a * 0.5f); q[2] = 0.0f; q[3] = std::cos(a * 0.5f);
    memcpy(&at<float>(s.unit, kLmat), m, sizeof(m));
    memcpy(&at<float>(s.unit, kWmat), m, sizeof(m));
    for (uintptr_t obj : v.objs) {
        at<uintptr_t>(obj, kParent) = s.unit;
        at<int32_t>(obj, kParentJoint) = -1;
    }
}

float distance(const float a[3], const float b[3]) {
    const float x = a[0] - b[0], y = a[1] - b[1], z = a[2] - b[2];
    return std::sqrt(x * x + y * y + z * z);
}
// The manifest's rooms: pick a room, then load / release the files of any unit it places.
void draw_rooms() {
    if (!g_manifest_read) { g_manifest_read = true; queue(Request{ Request::READ_MANIFEST }); }
    if (!ImGui::CollapsingHeader(_("Rooms"), ImGuiTreeNodeFlags_DefaultOpen)) return;
    const std::vector<Room>& rs = rooms();
    int units = 0;
    for (const Room& r : rs) units += (int)r.units.size();
    ImGui::TextDisabled(_("%d rooms, %d units"), (int)rs.size(), units);
    ImGui::SameLine();
    Mod::help_marker(_("The units each room's .pla / .sdl place, with the files they need (built in from the hook's "
                       "misc\\unit_packs\\rooms\\<room>.xml; names only, the files come from the game's own arcs). Load "
                       "puts a unit's files into the game's file cache, Release frees them. Spawn creates the unit the way "
                       "its room does (a one-unit .pla given to the game's uPlacement), with every value its room gives it; "
                       "units from a .sdl get each value's first key. Units whose files are all in the always-loaded arcs "
                       "spawn without loading."));
    if (!manifest_error().empty()) ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", manifest_error().c_str());
    if (rs.empty()) return;
    if (g_room_pick >= (int)rs.size()) g_room_pick = 0;
    ImGui::PushItemWidth(120.0f);
    if (ImGui::BeginCombo(_("Room"), rs[g_room_pick].name.c_str())) {
        for (int i = 0; i < (int)rs.size(); ++i)
            if (ImGui::Selectable(rs[i].name.c_str(), i == g_room_pick)) g_room_pick = i;
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    g_unit_filter.Draw(_("Filter (name or class)"), 200.0f);
    ImGui::Checkbox(_("Only units with room files"), &g_room_only);
    ImGui::SameLine();
    Mod::help_marker(_("Hides units whose files are all in the always-loaded arcs (lights, filters, ...): they need no loading."));
    ImGui::SameLine();
    ImGui::Checkbox(_("Spawn at my position"), &g_spawn_here);
    ImGui::SameLine();
    Mod::help_marker(_("On: the unit's mPos becomes your position (its rotation and the rest stay as its room has them). Off: "
                       "it appears where its room puts it, in this room's space."));
    const Room& room = rs[g_room_pick];
    const ImGuiTableFlags flags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_ScrollY |
                                  ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp;
    if (!ImGui::BeginTable("units", 4, flags, ImVec2(0.0f, 260.0f))) return;
    ImGui::TableSetupScrollFreeze(0, 1);
    ImGui::TableSetupColumn(_("Unit"));
    ImGui::TableSetupColumn(_("Class"));
    ImGui::TableSetupColumn(_("Files"));
    ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 190.0f);
    ImGui::TableHeadersRow();
    for (int i = 0; i < (int)room.units.size(); ++i) {
        const Unit& u = room.units[i];
        if (g_room_only && !u.stored && !u.loaded.arc) continue;
        if (!g_unit_filter.PassFilter(u.name.c_str()) && !g_unit_filter.PassFilter(u.cls.c_str())) continue;
        ImGui::PushID(i);
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(u.name.c_str());
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("%s #%d, move line %d\n%s", u.source.c_str(), u.track, u.line, u.values.c_str());
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(u.cls.c_str());
        ImGui::TableNextColumn();
        ImGui::Text(_("%d, %.2f MB"), (int)u.files.size(), u.stored / 1e6);
        ImGui::TableNextColumn();
        if (!u.loaded.arc) {
            if (ImGui::SmallButton(_("Load"))) queue(Request{ Request::LOAD_UNIT, g_room_pick, i });
            if (!u.loaded.error.empty()) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), _("failed"));
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", u.loaded.error.c_str());
            }
        } else {
            if (ImGui::SmallButton(_("Release"))) queue(Request{ Request::RELEASE_UNIT, g_room_pick, i });
        }
        if (u.loaded.arc || !u.stored) {
            ImGui::SameLine();
            if (ImGui::SmallButton(_("Spawn"))) {
                Request r{ Request::SPAWN_UNIT, g_room_pick, i, g_spawn_here && g_have_player ? 1u : 0u };
                memcpy(r.pos, g_player_pos, sizeof(r.pos));
                queue(r);
            }
        }
        if (u.loaded.arc) {
            ImGui::SameLine();
            ImGui::TextDisabled(_("%d, %.0f ms"), u.loaded.rows, u.loaded.ms);
        }
        ImGui::PopID();
    }
    ImGui::EndTable();
    if (!g_made.empty()) {
        ImGui::TextUnformatted(_("Spawned from rooms"));
        for (size_t k = 0; k < g_made.size(); ++k) {
            const Made& m = g_made[k];
            const Unit& u = rs[m.room].units[m.index];
            ImGui::PushID((int)k + 1000);
            ImGui::BulletText("%s  %s  (%s, line %d)", u.name.c_str(), u.cls.c_str(), rs[m.room].name.c_str(), m.line);
            ImGui::SameLine();
            if (ImGui::SmallButton(_("Despawn"))) queue(Request{ Request::DESPAWN_UNIT, 0, 0, m.unit });
            ImGui::PopID();
        }
    }
}

// The floating window: packs (Load / Release, Spawn) and what each spawned unit made, where it stands.
void draw_window() {
    ImGui::SetNextWindowSize(ImVec2(560.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin(_("Unit packs"), &g_enabled)) { ImGui::End(); return; }
    ImGui::PushID("UnitPacks");
    for (int i = 0; i < kPackCount; ++i) {
        const Pack& p = kPacks[i];
        const Loaded& st = state(i);
        ImGui::PushID(i);
        ImGui::TextUnformatted(p.name);
        ImGui::SameLine();
        Mod::help_marker(p.about);
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
            Mod::help_marker(_("Spawns the unit the way its room does (for the laser: a stage scheduler playing the chosen .sdl, which "
                          "creates the laser). The .sdl's positions are the original room's world coordinates, so in another room "
                          "the laser appears where it stood in that room until you turn on Place under Spawned."));
        }
        ImGui::PopID();
        ImGui::Separator();
    }
    draw_rooms();
    if (g_have_player) ImGui::Text(_("You: %.0f %.0f %.0f"), g_player_pos[0], g_player_pos[1], g_player_pos[2]);
    ImGui::SameLine();
    Mod::help_marker(_("Positions are the game's world coordinates in cm (Y up). Warp puts you on a unit (careful: a laser "
                       "hurts). Values marked \"in its file\" are the .sdl's own, before Place moves them."));
    if (!g_spawned.empty()) {
        ImGui::SeparatorText(_("Spawned"));
        for (size_t k = 0; k < g_spawned.size(); ++k) {
            const Spawned& s = g_spawned[k];
            ImGui::PushID((int)k);
            ImGui::Text("%s  %s", kPacks[s.pack].spawn->cls, kPacks[s.pack].spawn->resources[s.resource]);
            ImGui::SameLine();
            if (ImGui::SmallButton(_("Despawn"))) queue(Request{ Request::DESPAWN, s.pack, s.resource, s.unit });
            Placement& pl = g_place[s.unit];
            ImGui::Checkbox(_("Place"), &pl.active);
            ImGui::SameLine();
            Mod::help_marker(_("Moves everything this spawn made: its first unit's starting point goes to Target, the rest keeps its "
                               "place relative to it, turned by Turn about the up axis. Off: back where its room's file puts it."));
            if (pl.active) {
                ImGui::SameLine();
                if (ImGui::SmallButton(_("To me")) && g_have_player) memcpy(pl.target, g_player_pos, sizeof(pl.target));
                ImGui::PushItemWidth(260.0f);
                ImGui::DragFloat3(_("Target"), pl.target, 5.0f, 0.0f, 0.0f, "%.0f");
                ImGui::SliderFloat(_("Turn"), &pl.turn, -180.0f, 180.0f, "%.0f deg");
                ImGui::PopItemWidth();
                if (pl.have_anchor) ImGui::TextDisabled(_("Starting point in its file: %.0f %.0f %.0f"), pl.anchor[0], pl.anchor[1], pl.anchor[2]);
            }
            const auto v = std::find_if(g_view.begin(), g_view.end(), [&](const SpawnView& w) { return w.unit == s.unit; });
            if (v == g_view.end() || v->units.empty()) ImGui::TextDisabled(_("  (nothing created yet)"));
            else for (size_t u = 0; u < v->units.size(); ++u) {
                const UnitView& uv = v->units[u];
                ImGui::PushID((int)u);
                auto row = [&](const char* label, const float* pos) {
                    ImGui::BulletText("%s  %.0f %.0f %.0f", label, pos[0], pos[1], pos[2]);
                    if (g_have_player) { ImGui::SameLine(); ImGui::TextDisabled(_("%.0f cm away"), distance(pos, g_player_pos)); }
                    ImGui::SameLine();
                    ImGui::PushID(label);
                    if (ImGui::SmallButton(_("Warp"))) { Request r{ Request::WARP }; memcpy(r.pos, pos, sizeof(r.pos)); queue(r); }
                    ImGui::PopID();
                };
                row(uv.name.c_str(), uv.pos);
                ImGui::Indent();
                for (const VecView& vec : uv.vectors)   // the .sdl's own values: local to the placement when Place is on
                    ImGui::BulletText(_("%s (in its file)  %.0f %.0f %.0f"), vec.name.c_str(), vec.v[0], vec.v[1], vec.v[2]);
                ImGui::Unindent();
                ImGui::PopID();
            }
            ImGui::PopID();
        }
    }
    if (!g_last_error.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.4f, 1.0f), "%s", g_last_error.c_str());
    }
    ImGui::PopID();
    ImGui::End();
}
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
        case Request::READ_MANIFEST:
            read_manifest();
            if (g_room_pick >= (int)rooms().size()) g_room_pick = 0;
            break;
        case Request::LOAD_UNIT:
            if (!load_unit(r.pack, r.resource)) g_last_error = rooms()[r.pack].units[r.resource].loaded.error;
            break;
        case Request::RELEASE_UNIT:
            release_unit(r.pack, r.resource);
            break;
        case Request::SPAWN_UNIT: {
            if (r.pack < 0 || r.pack >= (int)rooms().size() || r.resource < 0 || r.resource >= (int)rooms()[r.pack].units.size()) break;
            const int line = rooms()[r.pack].units[r.resource].line;
            std::vector<uintptr_t> before = line_units(line);
            std::string err;
            const uintptr_t upla = spawn_unit(r.pack, r.resource, r.unit ? r.pos : nullptr, err);
            if (upla) g_pending.push_back(Pending{ upla, r.pack, r.resource, line, 0, std::move(before) });
            else g_last_error = err;
            break;
        }
        case Request::DESPAWN_UNIT:
            for (size_t i = 0; i < g_made.size(); ++i) {
                if (g_made[i].unit != r.unit) continue;
                if (devil4_sdk::check_exists_in_moveline((void*)g_made[i].unit, g_made[i].line)) uactor_sdk::despawn((void*)g_made[i].unit);
                g_made.erase(g_made.begin() + i);
                break;
            }
            break;
        case Request::WARP:
            if (uPlayer* pl = devil4_sdk::get_local_player()) {
                float* pp = &at<float>((uintptr_t)pl, kPos);
                pp[0] = r.pos[0]; pp[1] = r.pos[1]; pp[2] = r.pos[2];
            }
            break;
        }
    }
    for (size_t i = 0; i < g_pending.size();) {   // manifest spawns: what their uPlacement made, once it's gone
        Pending& p = g_pending[i];
        if (devil4_sdk::check_exists_in_moveline((void*)p.placement, 1) && ++p.frames < 120) { ++i; continue; }
        int made = 0;
        for (uintptr_t c : line_units(p.line))
            if (std::find(p.before.begin(), p.before.end(), c) == p.before.end()) {
                g_made.push_back(Made{ c, p.room, p.unit, p.line });
                ++made;
            }
        if (!made) g_last_error = rooms()[p.room].units[p.unit].name + ": nothing was created (see the log)";
        g_pending.erase(g_pending.begin() + i);
    }
    if (++g_prune >= 30) {               // units the game already removed (room change, their own end)
        g_prune = 0;
        for (size_t i = 0; i < g_spawned.size();) {
            if (alive(g_spawned[i])) ++i;
            else g_spawned.erase(g_spawned.begin() + i);
        }
        for (size_t i = 0; i < g_made.size();) {
            if (devil4_sdk::check_exists_in_moveline((void*)g_made[i].unit, g_made[i].line)) ++i;
            else g_made.erase(g_made.begin() + i);
        }
    }
    std::vector<SpawnView> view;
    for (const Spawned& s : g_spawned)
        if (alive(s)) {
            view.push_back(view_of(s));
            apply_placement(s, view.back());
        }
    for (auto it = g_place.begin(); it != g_place.end();)   // despawned or gone
        if (std::none_of(g_spawned.begin(), g_spawned.end(), [&](const Spawned& s) { return s.unit == it->first; })) it = g_place.erase(it);
        else ++it;
    uPlayer* pl = devil4_sdk::get_local_player();
    g_view.swap(view);
    g_have_player = pl != nullptr;
    if (pl) memcpy(g_player_pos, &at<float>((uintptr_t)pl, kPos), sizeof(g_player_pos));
    if (g_enabled && g_framework->m_draw_ui) draw_window();   // inside the ImGui frame, like PhotoMode's window
}

void UnitPacks::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    ImGui::Checkbox(_("Unit packs"), &g_enabled);
    ImGui::SameLine();
    help_marker(_("Loads a stage gadget's files (the temple laser, ...) from the room it ships with into the game's file cache, so "
                  "it can be spawned in any room. Only the files it needs are read, from the game's own arcs; nothing is written. "
                  "Release frees them again (units already spawned keep what they use). Opens its own window while this menu is "
                  "open. For the stage editor."));
}

void UnitPacks::on_config_load(const utility::Config& cfg) {
    g_enabled = cfg.get<bool>("unit_packs").value_or(false);
}

void UnitPacks::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("unit_packs", g_enabled);
}
