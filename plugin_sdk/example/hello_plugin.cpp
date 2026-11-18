// hello_plugin.cpp: a small dmc4_hook plugin, showing the plugin API except hooks: init and its info, a frame callback,
// a menu panel, a saved setting, the log, reading the game through api->game (version 2), the players through
// api->players (version 3: co-op can have up to four) and a room class of its own (api->rooms, version 4: room 901,
// stage 200's room with a frame counter; Enter in the panel goes there during a mission).
// Build: build_example.bat (32-bit MSVC). Install: copy hello_plugin.dll into the game folder's plugins\.
#include "dmc4_plugin.h"
#include "imgui.h"
#include <cstdio>
#include <cstring>

static const dmc4_api* api;   // valid as long as the game runs
static dmc4_plugin* self;     // this plugin, for every api call
static bool log_frames = true;
static uint64_t frames = 0;
static bool in_gameplay = false;   // read on the game thread, shown in the panel
static float style = 0.0f;
static int players = 0;
static bool want_room = false;     // the panel's Enter button, acted on in the frame callback (game thread)
static int room_frames = -1;       // frames in room 901 so far, -1 = not in it

// room 901: aRoom200's room (its files, scripts, doors) plus this plugin's memory and a step of its own each frame
struct RoomFields {
    uint32_t frames;
};

static void room_move(const dmc4_room_call* call) {
    api->rooms->base(call);                       // the room's own frame first
    auto* f = static_cast<RoomFields*>(call->fields);
    ++f->frames;
}

static void room_final(const dmc4_room_call* call) {
    char line[96];
    snprintf(line, sizeof line, "left room 901 after %u frames", static_cast<RoomFields*>(call->fields)->frames);
    api->log(self, DMC4_LOG_INFO, line);
    api->rooms->base(call);                       // saves the player, frees the room's units
}

// the game thread, once per game frame
static void on_frame(void*) {
    ++frames;
    if (api->version >= 2) {   // api->game exists from version 2 on; game code runs here, on the game thread
        in_gameplay = api->game->in_gameplay() != 0;
        style = api->game->style_rank();
    }
    if (api->version >= 3) players = api->players->count();
    if (api->version >= 4) {
        void* room = api->rooms->current();
        auto* f = static_cast<RoomFields*>(api->rooms->fields(room));   // NULL unless it's a registered class's room
        room_frames = f && api->rooms->room_number(room) == 901 ? (int)f->frames : -1;
        if (want_room) {
            want_room = false;
            if (!api->rooms->enter(901)) api->log(self, DMC4_LOG_WARN, "can't go to room 901 now (not in a mission?)");
        }
    }
    if (log_frames && frames % 600 == 0) {
        char line[96];
        snprintf(line, sizeof line, "%llu frames seen (the hook counts %llu)", (unsigned long long)frames,
                 (unsigned long long)api->frame_count());
        api->log(self, DMC4_LOG_INFO, line);
    }
}

// the hook's menu, inside this plugin's section of the Plugins list
static void on_panel(void*) {
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(api->imgui_context()));
    if (ImGui::Checkbox("Log every 600 frames", &log_frames)) api->config_set(self, "log_frames", log_frames ? "1" : "0");
    ImGui::Text("Frames seen: %llu", (unsigned long long)frames);
    if (api->version >= 2) ImGui::Text("In gameplay: %s, style rank %.0f", in_gameplay ? "yes" : "no", style);
    if (api->version >= 3) ImGui::Text("Players: %d", players);
    if (api->version >= 4) {
        if (ImGui::Button("Enter room 901")) want_room = true;
        ImGui::SameLine();
        if (room_frames >= 0) ImGui::Text("in room 901, %d frames", room_frames);
        else ImGui::TextDisabled("not in room 901");
    }
}

DMC4_PLUGIN_ABI_EXPORT()

DMC4_PLUGIN_EXPORT int dmc4_plugin_init(const dmc4_api* a, dmc4_plugin* s, dmc4_plugin_info* info) {
    api = a;
    self = s;
    info->name = "Hello";
    info->version = "1.0";
    info->author = "dmc4_hook";
    info->description = "The plugin_sdk example: counts game frames and keeps one setting.";

    // the panel draws with the hook's ImGui: same version, and the hook's allocators
    if (std::strcmp(api->imgui_version, IMGUI_VERSION) != 0) {
        api->log(self, DMC4_LOG_ERROR, "built with another ImGui version than the hook's: no panel");
    } else {
        void *alloc_fn, *free_fn, *user_data;
        api->imgui_allocators(&alloc_fn, &free_fn, &user_data);
        ImGui::SetAllocatorFunctions(reinterpret_cast<ImGuiMemAllocFunc>(alloc_fn), reinterpret_cast<ImGuiMemFreeFunc>(free_fn),
                                     user_data);
        api->on_panel(self, on_panel, nullptr);
    }

    const char* saved = api->config_get(self, "log_frames");   // "plugin.hello_plugin.log_frames" in dmc4_hook.cfg
    log_frames = !saved || saved[0] != '0';
    api->on_frame(self, on_frame, nullptr);

    if (api->version >= 4) {                     // room classes are registered here, in init, or not at all
        dmc4_room_class room = {};
        room.size = sizeof room;
        room.room = 901;
        room.base = "aRoom200";
        room.stage = -1;                         // the base's files
        room.fields_size = sizeof(RoomFields);
        room.move = room_move;
        room.final = room_final;
        api->rooms->register_class(self, &room);   // logs why when refused
    }
    api->log(self, DMC4_LOG_INFO, "hello from the example plugin");
    return 0;   // anything else: the hook stops the plugin and shows the value
}
