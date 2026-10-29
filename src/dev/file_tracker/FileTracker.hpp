#pragma once

#include "../../mod.hpp"
#include <cstdint>
#include <string>
#include <vector>

// FileTracker: what the game has loaded right now, and what it loads and drops as you play. A debug readout for finding
// which file a motion / effect / model comes from (which cutscene LMT holds the animation you saw), nothing in it changes
// the game.
//
// Where it reads (DX9, RE note Vibed\RE\sResource_findings.md): every loaded file is a cResource in sDevil4Resource's
// hash table (sResource::mpTable[8192] at +0x1038, the pointer at 0xE552D0), whether it came from an .arc or from a
// loose file under nativePC. A cResource keeps its game path (+0x04, 64 chars), reference count (+0x44), attributes
// (+0x48), state (+0x4C: 1 loaded, 0x10 failed, bits 16-18 priority) and size (+0x50); its class and extension come from
// the resource's vtable (slot 4 getDTI -> MtDTI name, slot 6 getExt). An .arc is itself a resource (class rArchive) and
// its loader (vtbl slot 8, 0x8E17B0) creates one cResource per entry of the arc and keeps their pointers in the arc
// (rArchive +0x60, count +0x64), so a file can be traced back to the arc it was unpacked from, and an entry's reference
// count tells whether anything but its arc uses it (the arc holds one reference).
//
// The scan runs on the game thread (on_frame) every `scan_interval` frames, under the table's own critical section
// (sResource +0x04, what the game takes for the table), and keeps a snapshot for the panel plus a log of what appeared and
// vanished between scans (a file loaded and freed within one interval is missed).

namespace file_tracker {

struct Entry {
    uintptr_t   rsrc = 0;
    std::string path, ext, cls, arc;     // arc: the .arc it was unpacked from, "" = loaded on its own (loose file / top-level)
    uint32_t    refs = 0, size = 0, state = 0, attr = 0;
    bool        is_arc = false;
    int         children = 0;            // rArchive: entries in it
    int         used = 0;                // rArchive: entries something besides the arc holds
};

struct Event {
    uint64_t frame = 0;                  // the tracker's scan frame
    double   seconds = 0.0;              // since the tracker started
    bool     added = true;               // false = gone
    Entry    e;
};

extern bool mod_enabled;
extern int  scan_interval;               // frames between scans

void scan();                             // game thread only
std::vector<Entry> entries();            // the last scan (a copy)
std::vector<Event> events();             // the log, oldest first (a copy)
void clear_log();
bool save_list(const char* file);        // the last scan as text, one entry per line
bool save_log(const char* file);

} // namespace file_tracker

class FileTracker : public Mod {
public:
    FileTracker() = default;
    std::string get_mod_name() override { return "FileTracker"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
