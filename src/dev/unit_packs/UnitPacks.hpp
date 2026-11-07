#pragma once

#include "../../mod.hpp"
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

struct Loaded {
    uintptr_t   arc = 0;          // our rArchive (not in sResource's table), 0 = not loaded
    int         rows = 0;         // rows the arc holds (a row already cached is shared, not reloaded)
    int         failed = 0;       // rows the game didn't create
    uint32_t    bytes = 0;        // size of the in-memory arc (stored bytes + header)
    double      ms = 0.0;         // time the load took
    std::string error;
};

struct Spawned {
    uintptr_t   unit = 0;
    int         pack = 0, resource = 0, line = 0;
};

bool load(int pack);              // game thread; fills state(pack)
void release(int pack);           // game thread
const Loaded& state(int pack);
uintptr_t spawn(int pack, int resource, const float pos[3], std::string& error);   // game thread

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
