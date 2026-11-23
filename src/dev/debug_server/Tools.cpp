#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/sMediator.hpp"
#include "../../sdk/Players.hpp"
#include "../../sdk/RoomClasses.hpp"
#include "../../sdk/FrameHooks.hpp"

// S0 tools (plan Vibed/GameMCP/README.md): version, status, log. S1's queries are in Queries.cpp, S2's actions in
// Actions.cpp, S3's input / pause / step / wait in Input.cpp, S5's sdl link in Sdl.cpp, S4's screenshot in Screenshot.cpp, the memory access they share in Memory.cpp.

namespace debug_server {
namespace {

// cArea vtable slot 10 = getName (a static string), for the area layers (aGame / aMission... / aRoomNNN)
const char* area_name(const void* area) {
    if (!area) return nullptr;
    void* const* vt = *reinterpret_cast<void* const* const*>(area);
    if (!vt) return nullptr;
    using Fn = const char*(__thiscall*)(const void*);
    return reinterpret_cast<Fn>(vt[10])(area);
}

json status() {
    json s;
    s["frame"] = frame_hooks::frames();
    s["load_gen"] = load_gen();
    sArea* area = devil4_sdk::get_sArea();
    s["area"] = area != nullptr;
    if (area) {
        const void* layers[3] = {area->aGamePtr, area->currentMissionPtr, area->aRoomPtr};
        const char* keys[3] = {"game", "mission", "room"};
        json l = json::object();
        for (int i = 0; i < 3; ++i) {
            const char* n = area_name(layers[i]);
            l[keys[i]] = n ? json(n) : json();
        }
        s["layers"] = l;
        aRoom* r = area->aRoomPtr;
        const uintptr_t dti = r ? mem::dti_of(reinterpret_cast<uintptr_t>(r)) : 0;
        if (r && dti && mem::dti_is_a(dti, "aroom")) {   // the room layer can be another cArea (MissionTitle)
            s["room"] = r->room_no;
            s["stage"] = r->stage_no;
            s["room_mode"] = r->mode;
            s["custom_room"] = room_classes::class_of(r) >= 0;
        }
    }
    if (sMediator* m = devil4_sdk::get_sMediator()) s["mission"] = m->missionID;
    s["players"] = player_info::count();
    const json input = input_status();   // kept: items() only refers to it
    for (const auto& [k, v] : input.items()) s[k] = v;
    return s;
}

} // namespace

void register_tools() {
    add_tool({"version", "The server and hook: name, phase, build time, the tools there are.",
              json{{"type", "object"}, {"properties", json::object()}}, false, [](const json&) {
                  return json{{"server", "dmc4_hook debug server"}, {"phase", "S5 v1"},
                              {"built", std::string(__DATE__) + " " + __TIME__}, {"tools", tool_names()}};
              }});
    add_tool({"status",
              "The game now: frames counted by the frame hook, the area layers (game / mission / room class names), "
              "room and stage numbers, the room's mode (7 = play), whether it is a registered custom room, the "
              "mission number, the player count. Runs on the game thread.",
              json{{"type", "object"}, {"properties", json::object()}}, true, [](const json&) { return status(); }});
    add_tool({"log",
              "dmc4_hook's log lines (the last 4000 kept): those after `since` (a seq from an earlier reply's `next`), "
              "or with since < 0 the last -since lines; at most `max` (default 200). Levels: 0 trace ... 4 error.",
              json{{"type", "object"},
                   {"properties", {{"since", {{"type", "integer"}, {"description", "seq to read after; < 0 = last N"}}},
                                   {"max", {{"type", "integer"}, {"description", "lines at most (default 200)"}}}}}},
              false, [](const json& a) {
                  const long long since = a.contains("since") ? a["since"].get<long long>() : -50;
                  const int max = a.contains("max") ? a["max"].get<int>() : 200;
                  return log_lines(since, max);
              }});
    register_queries();
    register_actions();
    register_input();
    register_sdl();
    register_screenshot();
    register_pick();
}

} // namespace debug_server
