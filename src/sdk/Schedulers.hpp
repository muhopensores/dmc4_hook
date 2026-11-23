#pragma once

#include <cstdint>
#include <string>
#include <vector>

// The live schedulers (uScheduler: a playing .sdl) and what they made, for every tool that works with stage units: the
// debug server's .sdl link (sdl_list / sdl_patch / sdl_clock), the object picker (which .sdl track made the picked unit)
// and the stage editor (Vibed\StageEditor\README.md, S5). RE: Vibed\RE\sdl_scheduler_runtime.md.
//
// - Every live scheduler passes uScheduler::move (0x90A850) once a tick, whoever owns it (room, moving floor, enemy,
//   title...), sometimes on a sMain job thread: an entry hook records it (under a mutex). One hook for all users.
// - Per track the scheduler keeps a 0x34-byte state (uScheduler +0x2C, count at +0x18): +0 the file's 0x18-byte track
//   (+4 parent = the move line for a unit track, +8 name), +4 low byte the track type, +8 the bound object: the unit a
//   unit track (type 2) created, or the object a property track keys (the unit itself for its own properties).
namespace schedulers {

bool install();                    // the move hook (idempotent; also installs sdk/FrameHooks for the tick count)

struct Live {
    uintptr_t id;                  // the uScheduler
    uintptr_t rsrc;                // its rScheduler (+0x38)
    uint64_t  seen;                // frame_hooks::frames() at its last move
};
std::vector<Live> live();          // any thread: the recorded objects that are still schedulers with a file, by address
void forget();                     // drops the record (a room change: addresses come back as other objects)

std::string path_of(uintptr_t rsrc);   // the resource's game path ("scr\st405\sdl\blade01"), "" if unreadable

struct Track {
    int         index = -1;
    uint8_t     type = 0;          // 2 unit, 6 int, 7 vector, 8 float, 9 bool, 10 ref, 11 resource, 13 event...
    std::string name;
};
// The scheduler track that created `unit`, and the tracks that key it (mPos, mAngle, mDisp...). Game thread.
struct Owner {
    uintptr_t          scheduler = 0, rsrc = 0;
    std::string        path;
    Track              unit;       // the unit track
    std::vector<Track> keyed;      // property tracks bound to the unit
    bool keys(const char* property) const;
};
bool owner_of(uintptr_t unit, Owner& out);

} // namespace schedulers
