#pragma once

#include <cstdint>
#include <string>
#include <vector>

class aRoom;

// Room classes derived from the game's (plan Vibed/Rooms/README.md, RE Vibed/RE/room_loading.md). The game finds a room
// by class name: aRoom::changing makes "aRoom%03d" of the next room number and looks it up in the DTI tree, and sArea
// creates only the area classes it recorded when it was built (sArea::createAreaInfo, once, inside WinMain). So:
//   - mods and plugins `add` a class while starting up; the classes are made by `create_all`, which the Rooms mod's hook
//     runs in the sArea constructor right before createAreaInfo (0x8DB1A9): the game's DTI tree is built by its CRT
//     initialisers, after dmc4_hook starts, so nothing can be made earlier. A class can't be removed afterwards.
//   - a class derives from a vanilla room (aRoom200: its scripts, files and behaviour) or the bare aRoom, or from a class
//     added before it; it may override the room's load / init / move / final and its script data, and has its own fields
//     after the base's.
//   - the room number names the class and the room's arc (rom\room\st<room>, optional for these rooms: a missing one is
//     dropped instead of hanging the load); the stage number names its files (scr\st<stage>\...), the base's by default.
// Enter a room with AreaJump::jump_to_room_id (it knows these rooms).
namespace room_classes {

constexpr uint32_t kRoomSize = 0x1C0;   // sizeof(aRoom); a class's own fields follow its base's

// What a callback gets. Call `base(c)` to run the slot it replaces (the base class's).
struct Ctx {
    aRoom* room;
    void* fields;       // this class's own fields (ClassDesc::fields_size bytes, zeroed at creation)
    void* user;         // ClassDesc::user
    int level;          // internal: the class in the chain
    int slot;           // internal: the vtable slot
    int arg;            // internal
    uintptr_t ret;      // internal: the base slot's result
};

uintptr_t base(Ctx& c);

struct Callbacks {
    void (*create)(Ctx& c) = nullptr;   // after construction (fields zeroed), base classes first
    void (*destroy)(Ctx& c) = nullptr;  // before the base destructor, derived classes first
    bool (*load)(Ctx& c) = nullptr;     // each frame while loading; true = loaded (`return base(c) != 0` keeps the game's)
    void (*init)(Ctx& c) = nullptr;     // once loaded: call base(c) to build the room (aRoom::init -> setRoom), then add units
    void (*move)(Ctx& c) = nullptr;     // every frame
    void (*final)(Ctx& c) = nullptr;    // leaving the room: call base(c) (saves the player's state, frees the room's units)
};

struct ClassDesc {
    std::string owner;                  // for the log and clear_callbacks ("Rooms test", a plugin's file name)
    int room = -1;                      // the room number: the class is aRoom<room> (%03d); 0-65519, not a vanilla room's
    std::string base = "aRoom";         // a vanilla room class ("aRoom200") or one added before this one
    int stage = -1;                     // the stage number; -1 = the base's
    uint32_t fields_size = 0;
    void* user = nullptr;
    Callbacks cb;
    // Event scripts (Vibed/RE/event_script.md; null = the base's), as bytes or as text (sdk/EventScript's form,
    // assembled by `add`, which refuses the class when the text is wrong; text wins over bytes):
    const void* script = nullptr;       // vtable slot 11 (SE getScriptPtr): run once while the room loads
    const void* after_script = nullptr; // slot 12 (SE getAfterScriptPtr): run every frame, restarted when it ends
    std::string script_text;
    std::string after_script_text;
    const void* script_player = nullptr;// slot 13: uScriptPlayer data
    std::vector<std::string> names;     // slot 14 (SE getFileName): files the scripts load by index (empty = the base's)
};

bool add(const ClassDesc& d, std::string* error = nullptr);   // while starting up (before created())
void create_all();                                             // the Rooms mod's hook; once
bool created();
void clear_callbacks(const std::string& owner);                // its classes act as their bases from now on

bool is_custom(int room);
const char* label(int room);            // "aRoom900 (Rooms test)", null if not one of these rooms
aRoom* current();                       // the room area (sArea+0x3838), any class
int class_of(const aRoom* room);        // index into list(), -1 = not one of these classes
void* fields_of(aRoom* room);           // its class's own fields, null if not one of these classes

struct Info {
    std::string name, base, owner, error;
    int room = -1, stage = -1;
    uint32_t size = 0;
    bool created = false;
    bool in_area_table = false;         // sArea recorded it (the game can create it)
    int live = 0;                       // instances now
};
std::vector<Info> list();

} // namespace room_classes
