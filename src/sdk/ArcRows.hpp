#pragma once

#include <cstdint>
#include <string>
#include <vector>

// Rows of the game's own arcs put into the resource cache in any room. The rows' stored (compressed) bytes are read
// from the vanilla arcs under nativePC, laid out as one small ARC in memory (header, TOC, data from 0x8000) and handed
// to the game's own rArchive load (vtbl slot 8) through an MtMemoryStream: per row the game finds the class by its DTI
// id, takes a reference on a copy already cached or decompresses and inserts a new one (Vibed\RE\sResource_findings.md).
// sResource::create of those paths then hits the cache. The arc holds one reference on each row until it is released.
// Load only the rows needed, not a whole arc: the first loaded copy of a path wins. Game thread only.
namespace arc_rows {

struct Row {
    const char* path;       // as in the arc's TOC, no extension ("motion\\pl006\\pl006_09")
    uint32_t    dti;        // the TOC's class id (rMotionList 0x139EE51D)
    const char* arc;        // relative to nativePC ("rom\\enemy\\em_dante.arc")
};

struct Result {
    uintptr_t   arc = 0;    // our rArchive (not in sResource's table), 0 = not loaded
    int         rows = 0;   // rows the arc holds (a row already cached is shared, not reloaded)
    int         failed = 0; // rows the game didn't create
    uint32_t    bytes = 0;  // size of the in-memory arc (stored bytes + header)
    double      ms = 0.0;   // time the load took
    std::string error;
};

// Load `rows` as one in-memory arc named `name` (shown by FileTracker); `st` gets the result (arc 0 = failed).
bool load(const std::string& name, const std::vector<Row>& rows, Result& st);
void release(Result& st);

} // namespace arc_rows
