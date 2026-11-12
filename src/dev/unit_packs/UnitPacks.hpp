#pragma once

#include "../../mod.hpp"
#include "../../sdk/ArcRows.hpp"
#include <cstdint>
#include <string>
#include <vector>

// UnitPacks: loads a stage unit's files into the resource cache in any room, so the unit can be spawned outside the room
// it ships with (stage editor S6 / S7, Vibed\StageEditor\README.md). A pack (misc/unit_packs, generated from
// Vibed\StageEditor\unit_arcs\packs) lists the arc rows the unit needs: what its code loads by path, its nested lists'
// members, its effects' textures and flipbooks, its own .sdl. Loading a pack reads those rows' stored (XMEM-compressed)
// bytes from the vanilla arcs under nativePC, lays them out as a small ARC in memory (header, TOC, data from 0x8000) and
// hands it to the game's own rArchive load (vtbl slot 8) through an MtMemoryStream: per row the game finds the class by
// its DTI id, takes a reference on a copy already cached (the current room's, game_cmn's) or decompresses and inserts a
// new one (sub_8DF380 / rArchive::DecompressStream; Vibed\RE\sResource_findings.md). The unit's own sResource::create
// calls then hit the cache. The pack's rArchive holds one reference on each row until it is released; units spawned
// from the pack hold their own.
//
// Only the rows are loaded, not the whole room arc: unrelated rows with the same paths would clash (the first loaded
// copy of a path wins). Loads and spawns run on the game thread (on_frame); the panel only queues them.

namespace unit_packs {

using Loaded = arc_rows::Result;   // sdk/ArcRows: arc, rows, failed, bytes, ms, error

struct Spawned {
    uintptr_t   unit = 0;
    int         pack = 0, resource = 0, line = 0;
};

bool load(int pack);              // game thread; fills state(pack)
void release(int pack);           // game thread
const Loaded& state(int pack);
uintptr_t spawn(int pack, int resource, const float pos[3], std::string& error);   // game thread

// The rooms manifest (misc/unit_packs/rooms/<room>.xml, built in): per room the rows its units need and the units its
// .pla / .sdl place, each with the indices of its rows, in load order.
struct File {
    std::string path, type, arc;      // arc: relative to nativePC
    uint32_t    dti = 0, stored = 0;
    bool        global = false;       // also in the always-loaded arcs
};
// One track of a unit's one-unit .pla (dmc4xml/pla.py layout): 1 root, 4 unit, 5 object, 6 bool, 7 int, 8 float,
// 9 vector, 10 ref (a track index in this .pla), 11 resource (rdti + path; no path = null).
struct PlaTrack {
    uint32_t    type = 0, prop = 0, parent = 0, line = 0, index = 0;   // index: the dti field (array index; class hash for the unit)
    std::string name, path;
    uint32_t    u = 0, rdti = 0;     // int / bool / ref value; resource class hash
    float       v[4] = {};           // float (v[0]) / vector value
};
struct Unit {
    std::string           name, cls, source, values;   // values: one "kind name = value" line each, for the panel
    int                   line = 0, track = 0;
    uint32_t              stored = 0;      // bytes of its rows that aren't in the always-loaded arcs
    std::vector<int>      files;
    std::vector<PlaTrack> tracks;          // the unit as a .pla: Root, the unit, its values and objects
    Loaded                loaded;
};
struct Room {
    std::string       name, arc;
    std::vector<File> files;
    std::vector<Unit> units;
};

int  read_manifest();                     // game thread; releases what the old manifest's units loaded; -> unit count
const std::vector<Room>& rooms();
const std::string& manifest_error();
bool load_unit(int room, int unit);       // game thread
void release_unit(int room, int unit);    // game thread
// The unit as its room creates it: a .pla of just this unit (mPos replaced by `pos` when given) handed to a one-shot
// uPlacement, which makes the unit, sets its values and kills itself (as rooms do). Returns the uPlacement or 0.
uintptr_t spawn_unit(int room, int unit, const float* pos, std::string& error);   // game thread
std::vector<uint8_t> build_pla(const Unit& u, const float* pos);

} // namespace unit_packs

class UnitPacks : public Mod {
public:
    UnitPacks() = default;
    std::string get_mod_name() override { return "UnitPacks"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
