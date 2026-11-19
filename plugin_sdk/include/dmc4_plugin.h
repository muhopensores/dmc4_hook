/*
 * dmc4_plugin.h: the dmc4_hook plugin API (DLL plugins for Devil May Cry 4 DX9, 32-bit).
 *
 * A plugin is a 32-bit DLL in the game folder's plugins\ folder. dmc4_hook loads every DLL there at startup
 * (before the game's window opens, in file-name order), checks its ABI, and calls its dmc4_plugin_init. From then on
 * the plugin works through the dmc4_api table it was given: callbacks on the game frame and in the hook's menu, its own
 * log, settings saved in dmc4_hook.cfg, and hooks on the game's code.
 *
 * Rules
 * - The interface is plain C: functions take and return C types only, and a plugin must not let a C++ exception leave a
 *   callback. Any compiler that makes 32-bit Windows DLLs works.
 * - dmc4_api only ever grows at the end. api->version says which functions this hook has (see DMC4_PLUGIN_API_VERSION
 *   at each one); a plugin that needs a newer function checks api->version first. A change that breaks plugins raises
 *   DMC4_PLUGIN_ABI instead, and the hook then refuses plugins built for the old one.
 * - Threads: init runs on the game's main thread before the window exists; frame callbacks run on the game thread
 *   once per game frame; panel callbacks run where the hook draws its menu. Don't touch game state from a panel
 *   callback: set a flag there and act on it in the frame callback.
 * - A plugin that crashes inside a callback (an access violation, an uncaught exception) is stopped: its callbacks
 *   are removed and the menu shows where it crashed. The DLL stays loaded (its hooks may still be in the game's code),
 *   so a crash can still leave the game in a bad state. Restart the game after fixing it.
 * - Hooks go through api->hook / api->hook_at, not a MinHook of the plugin's own: two copies of MinHook in one process
 *   don't know about each other's hooks. Install them in dmc4_plugin_init, before the game runs.
 * - Co-op: up to four players can play at once (dmc4_hook's split-screen co-op). Never assume one player: get them from
 *   api->players, and do per-player work for each of them. Most game code reads "the player" from sMediator::mpPlayer;
 *   co-op points it at player 2-4 while their units update, so the same game function acts for whoever is in there.
 *   To act for a particular player from a frame callback, wrap the work in api->players->run_as.
 *
 * A plugin exports two functions (DMC4_PLUGIN_ABI_EXPORT() writes the first):
 *     uint32_t dmc4_plugin_abi(void);   returns DMC4_PLUGIN_ABI
 *     int dmc4_plugin_init(const dmc4_api* api, dmc4_plugin* self, dmc4_plugin_info* info);   0 = loaded
 * See plugin_sdk\example\hello_plugin.cpp.
 */
#ifndef DMC4_PLUGIN_H
#define DMC4_PLUGIN_H

#include <stdint.h>

#ifdef __cplusplus
#define DMC4_EXTERN_C extern "C"
extern "C" {
#else
#define DMC4_EXTERN_C
#endif

#define DMC4_PLUGIN_ABI 1          /* breaking changes: a plugin built for another ABI isn't loaded */
#define DMC4_PLUGIN_API_VERSION 5 /* additions: functions appended to dmc4_api (and to the tables it points to) */

#define DMC4_PLUGIN_EXPORT DMC4_EXTERN_C __declspec(dllexport)
#define DMC4_PLUGIN_ABI_EXPORT() DMC4_PLUGIN_EXPORT uint32_t dmc4_plugin_abi(void) { return DMC4_PLUGIN_ABI; }

/* the hook's record of a loaded plugin; pass it back to every api function that asks for it */
typedef struct dmc4_plugin dmc4_plugin;

typedef void (*dmc4_callback)(void* user);

enum { DMC4_LOG_INFO = 0, DMC4_LOG_WARN = 1, DMC4_LOG_ERROR = 2 };

/* filled by the plugin in dmc4_plugin_init, with strings that live as long as the DLL (string literals) */
typedef struct dmc4_plugin_info {
    uint32_t size;            /* sizeof(dmc4_plugin_info) in the hook */
    const char* name;         /* shown in the menu; the file name when NULL */
    const char* version;
    const char* author;
    const char* description;
} dmc4_plugin_info;

/* ---- the game (api->game, version 2): dmc4_hook's SDK (sdk/Devil4.hpp) for plugins ----
 *
 * Pointers to the game's own objects (units, players, resources, effects) come back as void*; their layouts are in
 * dmc4_hook's sdk headers. A unit can be destroyed between two frames, so don't keep such a pointer past the frame you
 * got it in without checking it again (in_moveline). Everything here runs game code: call it from the game thread
 * (on_frame, or a hook on the game's code), not from a panel. Booleans are ints: 0 = false. Functions that take a
 * player want a uPlayer (DMC4_OBJ_LOCAL_PLAYER, or one found on unit line 13). */

enum dmc4_object {
    DMC4_OBJ_AREA = 0,           /* sArea: rooms, the game state (paused flag) */
    DMC4_OBJ_UNIT = 1,           /* sUnit: the 32 unit lines */
    DMC4_OBJ_RENDER = 2,         /* sRender */
    DMC4_OBJ_MEDIATOR = 3,       /* sMediator: mission, player, cameras */
    DMC4_OBJ_LOCAL_PLAYER = 4,   /* sMediator's player (uPlayer), NULL outside a stage. NOT "the" player in co-op: player 1
                                  * in a frame callback, but whoever's update is running inside a hook on game code. Use
                                  * api->players (version 3) to go through the players. */
    DMC4_OBJ_LOCAL_CAMERA = 5,   /* uCameraCtrl */
    DMC4_OBJ_PLAYER_CAMERA = 6,  /* cCameraPlayer */
    DMC4_OBJ_STYLISH_COUNT = 7,  /* sStylishCount: the style meter */
    DMC4_OBJ_WORK_RATE = 8       /* sWorkRate: game speed */
};

enum dmc4_body { DMC4_BODY_LOWER = 0, DMC4_BODY_UPPER = 1, DMC4_BODY_THIRD = 2 };   /* a player's motion controllers */

typedef struct dmc4_game_api {
    uint32_t size;   /* sizeof(dmc4_game_api) in the hook: functions are only appended (each says its api version) */

    /* ---- version 2 ---- */

    /* the game's singletons (dmc4_object); NULL while they don't exist */
    void* (*object)(int which);

    /* state */
    int (*is_paused)(void);          /* the game is paused (aGame) */
    int (*in_gameplay)(void);        /* a player exists and the game isn't paused */
    int (*is_playing)(void);         /* the local player exists with HP above 0 */
    int (*in_bloody_palace)(void);   /* mission 50 */
    int (*is_loading)(void);         /* no room yet, or its enemies are loading */
    int (*enemy_count)(void);        /* units on the enemy line (15) */
    float (*style_rank)(void);       /* the style meter's current tier (sStylishCount) */
    int (*key_down)(uint32_t vk);    /* a key in the game's own keyboard state (Windows virtual-key code) */

    /* units: 32 lines (sUnit::mMoveLine), each a linked list of cUnit (players 13, enemies 15, shells 18, effects 20) */
    void* (*line_top)(int line);                     /* the first unit of a line, NULL when empty */
    int (*in_moveline)(void* unit, int line);        /* the unit is (still) on that line */
    void* (*dti_of)(void* object);                   /* the object's MtDTI (class info) */
    void (*center)(void* unit, float* out_xyz);      /* a uModel's centre position (vtable slot 33) */

    /* resources: sResource::create / release (a reference each). mode 1 = load now. */
    void* (*load_resource)(void* dti, const char* path, uint32_t mode);
    void (*release_resource)(void* resource);

    /* effects (.efl): returns the uDevil4Effect, NULL if it couldn't be made */
    /* by path (without .efl), following `owner` (or placed at pos / rot when stationary: pos 4 floats, rot a quaternion
     * x y z w), on unit line unit_line (0x14 for effects), group / material masks (0xFFFFFFFF = all) */
    void* (*effect)(const char* efl_path, void* owner, int unit_line, uint32_t group, uint32_t material, int stationary,
                    const float* pos, const float* rot);
    void (*effect_end)(void* effect);                /* the effect's own culling call: it fades out and goes */
    /* from the game's effect table (0x39 lists of .efl paths): placed (stay) or following a parent (const) */
    void* (*effect_stay)(int list, int index, const float* quat, const float* pos, void* owner, int register_on_owner);
    void* (*effect_const)(int list, int index, void* parent, uint32_t group, uint32_t material, int unit_line, void* owner);
    /* a player's own effect list (128 entries): register under an id, then kill (at once) or finish (let it fade) */
    void (*register_effect)(void* effect, void* player, int type, int id);
    void (*kill_effect)(void* player, int id);
    void (*finish_effect)(void* player, int id);

    /* sounds */
    void (*player_se)(void* player, int id);         /* a sound from the player's own sound bank, on the player */

    /* players */
    /* one body half (dmc4_body): motion = bank << 8 | index, attr, interpolation frames, start frame, speed */
    void (*request_motion)(void* player, int body, int motion, uint32_t attr, float interp, float start, float speed);
    /* the whole body (uPlayer::requestSetMotion_2): motion id, mode, speed, start frame, interpolation frames */
    void (*request_motion_all)(void* player, uint32_t motion, uint32_t mode, float speed, float start, float interp);
    void (*neutral_air)(void* player);               /* back to the neutral air state (falling, can act) */
    void* (*held_weapon)(void* player);              /* the weapon unit in hand, or NULL */
    uint32_t (*ground_material)(void* player);       /* the effect material mask of the ground under the player */
    /* motion banks (uModel +0xA38, 16 slots): load an .lmt into one, empty it, or read it (an rMotionList, NULL = empty) */
    int (*load_motion_bank)(void* model, const char* lmt_path, int bank);
    void (*unload_motion_bank)(void* model, int bank);
    void* (*motion_bank)(void* model, int bank);
    /* cancel scripts (.ctn): the next motion request reads bank : index of the player's rCharTbl; or run a script built
     * in memory (it must outlive the motion; call on the tick after the motion request) */
    void (*borrow_ctn)(void* player, uint32_t bank, uint32_t index);
    void (*install_ctn_script)(void* player, const void* script);

    /* ---- version 5 ---- */

    /* enemies the hook can spawn (its EnemySpawn list: 0 Scarecrow (Leg) ... 22 Dante); the English name, NULL past it */
    int (*enemy_type_count)(void);
    const char* (*enemy_type_name)(int type);
    /* spawns one enemy of that type near the local player; 1 = spawned, 0 = no player or no such type */
    int (*spawn_enemy)(int type);
} dmc4_game_api;

/* ---- the players (api->players, version 3): which player is which, with and without co-op ---- */
typedef struct dmc4_players_api {
    uint32_t size;   /* sizeof(dmc4_players_api) in the hook: functions are only appended */

    /* ---- version 3 ---- */

    int (*count)(void);                    /* players in the stage: 1 without co-op, up to 4 with it, 0 outside a stage */
    void* (*get)(int index);               /* that player's uPlayer (0 = player 1 ... 3), NULL = none */
    int (*index_of)(void* unit);           /* 0-3 for a player, -1 for anything else */
    int (*has_keyboard)(void* player);     /* the keyboard's keys are this player's (co-op lets any player have them) */
    void* (*keyboard_player)(void);        /* the player who has the keyboard, NULL = nobody / not in a stage */
    /* where that player's view is on the game's screen, as fractions (left, top, right, bottom); 0 when he has none
     * there (without split screen only player 1's view shows, the whole screen) */
    int (*view_rect)(int index, float* rect4);
    void* (*current)(void);                /* sMediator's player right now: whose context game code is running in */
    /* fn(user) with `player` as the game's player (sMediator::mpPlayer, his camera, pad and lock-on values), for game
     * functions that act on "the player". Returns 1 when fn ran in his context. 0 = fn wasn't called: not a player, or
     * another player's context is active (inside a hook on a player's update: there, the running player is current()).
     * Game thread only. A crash in fn stops the plugin and still puts the context back. */
    int (*run_as)(dmc4_plugin* self, void* player, dmc4_callback fn, void* user);
} dmc4_players_api;

/* ---- rooms (api->rooms, version 4): new room classes, derived from the game's ----
 *
 * The game finds a room by class name: a room number n makes it create class "aRoom<n>" (three digits at least), and
 * the room loads rom\room\st<n>.arc (optional for these rooms: loose files work) and its files scr\st<stage>\... A
 * plugin registers classes while it starts (in dmc4_plugin_init; the hook makes them right before the game records its
 * area classes, and they can't be removed afterwards). A class derives from a vanilla room ("aRoom200": its files,
 * scripts and behaviour) or the bare "aRoom", or from a class registered before it, and may add:
 *   - callbacks around the room's own steps (each gets a dmc4_room_call; call api->rooms->base(call) to run the step it
 *     replaces: init must, final should);
 *   - event scripts as text (doors, enemy waves, fades: the format is in dmc4_hook's src/sdk/EventScript.hpp,
 *     Vibed/RE/event_script.md), and the file names they use by index;
 *   - its own memory in every instance (fields_size bytes, zeroed).
 * Room numbers 900-999 are free (no vanilla class); a vanilla room's number is refused. Enter a room with enter().
 * A plugin that is stopped (a crash) leaves its rooms running as their base class. Plan: Vibed/Rooms/README.md. */
typedef struct dmc4_room_call {
    void* room;      /* the room (an aRoom, sdk/sArea.hpp) */
    void* fields;    /* this class's fields_size bytes in it */
    void* user;      /* dmc4_room_class.user */
    void* internal;  /* the hook's: pass the call to base() */
} dmc4_room_call;

typedef struct dmc4_room_class {
    uint32_t size;              /* sizeof(dmc4_room_class) where the plugin was built */
    int room;                   /* the room number (900-999) */
    const char* base;           /* the base class: "aRoom200", "aRoom", or a room class registered before; NULL = "aRoom" */
    int stage;                  /* the files' stage number, -1 = the base's */
    uint32_t fields_size;       /* bytes of the plugin's own per-room memory */
    void* user;                 /* passed back in dmc4_room_call.user */
    void (*create)(const dmc4_room_call* call);   /* after construction */
    void (*destroy)(const dmc4_room_call* call);  /* before the base destructor */
    int (*load)(const dmc4_room_call* call);      /* each frame while loading: nonzero = loaded (return base(call)) */
    void (*init)(const dmc4_room_call* call);     /* once loaded: call base(call) (builds the room), then add units */
    void (*move)(const dmc4_room_call* call);     /* every frame */
    void (*final)(const dmc4_room_call* call);    /* leaving: call base(call) (saves the player, frees the room's units) */
    const char* script;         /* event script text run once while the room loads; NULL = the base's */
    const char* after_script;   /* event script text run every frame (restarted at its end); NULL = the base's */
    const char* const* names;   /* the scripts' files by index (.pla, .sbc, scheduler names...); NULL = the base's */
    int name_count;
} dmc4_room_class;

typedef struct dmc4_rooms_api {
    uint32_t size;   /* sizeof(dmc4_rooms_api) in the hook: functions are only appended */

    /* ---- version 4 ---- */

    /* adds a room class; only in dmc4_plugin_init. 1 = added, 0 = refused (the reason goes to the plugin's log: a taken
     * room number, a bad script with its line, no such base class...). Strings and the names array are copied. */
    int (*register_class)(dmc4_plugin* self, const dmc4_room_class* desc);
    /* runs the step a callback replaces (the base class's); its result (load: nonzero = loaded) */
    int (*base)(const dmc4_room_call* call);
    void* (*current)(void);                   /* the room now (any class), NULL = none */
    int (*room_number)(void* room);           /* aRoom +0x28, -1 for NULL */
    int (*stage_number)(void* room);          /* aRoom +0x2C, -1 for NULL */
    void* (*fields)(void* room);              /* the fields of a registered class's room, NULL for other rooms */
    /* starts going to room n (a vanilla room or a registered one), as AreaJump does; 1 = started. Game thread, in a stage */
    int (*enter)(int room);
    /* checks event script text without registering anything: 1 = fine; else 0 and the error (with the line) in error */
    int (*check_script)(const char* text, char* error, uint32_t error_size);
} dmc4_rooms_api;

/* ---- the hook's mods (api->mods, version 5): read their settings, hold one for a while ----
 *
 * Mods are named as in the debug server's mods list (their class names: "InfDT", "HealthSettings", ...) and a setting
 * by its dmc4_hook.cfg key ("infinite_dt", "one_hit_kill"); values are text as in the cfg ("true", "1.5"). A held
 * setting is applied at once (as when the config loads) and stays until the plugin releases it or stops; the user's
 * own value is kept: it's what the config saves, and it comes back on release. Game thread (hold / release apply the
 * mod's settings). */
typedef struct dmc4_mods_api {
    uint32_t size;   /* sizeof(dmc4_mods_api) in the hook: functions are only appended */

    /* ---- version 5 ---- */

    /* the setting's current value, NULL = no such mod / key; the string stays valid until the plugin's next get */
    const char* (*get)(dmc4_plugin* self, const char* mod, const char* key);
    /* holds the setting at alue: 1 = applied, 0 = no such mod / key (logged) */
    int (*hold)(dmc4_plugin* self, const char* mod, const char* key, const char* value);
    /* gives one setting back (the user's value comes back), or every one the plugin holds */
    void (*release)(dmc4_plugin* self, const char* mod, const char* key);
    void (*release_all)(dmc4_plugin* self);
} dmc4_mods_api;

/* ---- menus (api->menus, version 5): a row of the plugin's in the pause menu, opening a list menu ----
 *
 * The pause menu has five rows (Items, Skill List, Map, System, Return) and no room for more, so the plugin's row takes
 * the Map row's place, in one room or everywhere. Choosing it opens a list menu in the game's look (rows of
 * "label  < value >" over a dim backdrop, the game's cursor and sounds): up / down picks a row, left / right calls
 * change(-1 / +1) on a row with a value, A calls decide, B closes it (back to the pause menu). The callbacks run on the
 * game thread, while the game is paused. One pause row at a time (the last plugin to set one). */
typedef struct dmc4_list_menu {
    const char* title;
    void* user;                                                                    /* passed to every callback */
    int (*row_count)(void* user);
    /* row's text into label / value (both zero-terminated, at most size - 1 bytes); an empty value = an action row */
    void (*row_text)(void* user, int row, char* label, uint32_t label_size, char* value, uint32_t value_size);
    void (*change)(void* user, int row, int step);                                /* left / right: -1 / +1 */
    void (*decide)(void* user, int row);                                           /* A */
    /* pages (each NULL = none): the menu shows one page; the plugin switches it (from decide), and the cursor is kept
     * per page. Images are frames of the pause sheet + 1 (0 = text), drawn with the pause menu's own sprites: up to 5
     * labels and 8 values a page; a page with images uses the pause menu's row spacing (5 rows fit). */
    int (*page)(void* user);
    int (*title_image)(void* user);                                                /* the page's title frame + 1 */
    void (*row_images)(void* user, int row, int* label_image, int* value_image);
    int (*back)(void* user);                                                       /* B: nonzero = handled */
} dmc4_list_menu;

typedef struct dmc4_pause_row {
    uint32_t size;              /* sizeof(dmc4_pause_row) where the plugin was built */
    int room;                   /* only in this room (a room number, e.g. one of the plugin's 900-999); -1 = everywhere */
    const char* label;          /* drawn in place of "Map" */
    const char* description;    /* the pause menu's help line while the row is selected */
    dmc4_list_menu menu;        /* opened when the row is chosen */
    /* Images instead of text, as frames of the pause menu's sheet (id\pause\main01.anm) + 1; 0 = text. The sheet holds
     * the cut practice mode's labels (28 PRACTICE END, 19 PLAYER SETTING, 20 ENEMY SETTING, 21 PRACTICE OPTION, 40
     * NORMAL, 38 INFINITY, 39 NO DAMAGE, 30 D.T. GAUGE, 36 ON, 37 OFF ...): Vibed\RE\pause_menu.md. */
    int label_image;            /* the row's label, in the Map label's place */
    int label_image_width;      /* narrows that frame (pixels, 0 = as it is): edits the frame for every user of the sheet */
    int title_image;            /* in place of the PAUSE MENU title while the list is up */
} dmc4_pause_row;

/* A row of the plugin's in the Mission Menu, in My History's place (its row tables hold 15 rows: one is taken over, not
 * added). Choosing it calls decide on the game thread; the menu then goes on selecting (decide can start a mission into
 * one of the plugin's rooms with api->rooms->enter). */
typedef struct dmc4_mission_row {
    uint32_t size;              /* sizeof(dmc4_mission_row) where the plugin was built */
    const char* label;          /* drawn in place of "My History" */
    const char* description;    /* the menu's description box while the row is selected */
    void* user;
    void (*decide)(void* user);
    /* the label as frames of the Mission Menu's sheet (id\pc\mm_mm01.anm) + 1, 0 = text: 38 PRACTICE (the cut practice
     * row's own label, white) and 25 (silver), for the label sprite and the selected one */
    int label_image;
    int label_image_selected;
} dmc4_mission_row;

typedef struct dmc4_menus_api {
    uint32_t size;   /* sizeof(dmc4_menus_api) in the hook: functions are only appended */

    /* ---- version 5 ---- */

    /* sets the plugin's pause row (strings are copied; the callbacks and user must stay valid); 1 = set */
    int (*pause_row)(dmc4_plugin* self, const dmc4_pause_row* row);
    void (*clear_pause_row)(dmc4_plugin* self);
    int (*menu_open)(void);     /* a list menu is open now */
    /* sets / clears the plugin's Mission Menu row (strings copied; decide and user must stay valid); 1 = set */
    int (*mission_row)(dmc4_plugin* self, const dmc4_mission_row* row);
    void (*clear_mission_row)(dmc4_plugin* self);
} dmc4_menus_api;

typedef struct dmc4_api {
    uint32_t abi;             /* DMC4_PLUGIN_ABI of the hook */
    uint32_t version;         /* DMC4_PLUGIN_API_VERSION of the hook: the functions up to that version are present */
    uint32_t size;            /* sizeof(dmc4_api) in the hook */

    /* ---- version 1 ---- */

    /* one line in the plugin's log (the menu keeps the last 100) and in dmc4_hook_log.txt */
    void (*log)(dmc4_plugin* self, int level, const char* text);

    /* fn(user) on the game thread once per game frame, after every unit has ticked and the frame's hits are
     * resolved (sDevil4Main::move, after sCollisionGame::checkAttackMain). Returns an id > 0, 0 on failure. */
    int (*on_frame)(dmc4_plugin* self, dmc4_callback fn, void* user);

    /* fn(user) while the hook's menu draws the plugin's section (open in the Plugins list); ImGui calls are allowed:
     * start with ImGui::SetCurrentContext((ImGuiContext*)api->imgui_context()). Returns an id > 0, 0 on failure. */
    int (*on_panel)(dmc4_plugin* self, dmc4_callback fn, void* user);

    /* removes an on_frame / on_panel callback by its id (also from inside that callback) */
    void (*remove_callback)(dmc4_plugin* self, int id);

    /* game frames counted by the frame hook since the game started */
    uint64_t (*frame_count)(void);

    /* the plugin's settings, kept in dmc4_hook.cfg as "plugin.<dll name without .dll>.<key>" and saved with the
     * hook's config. config_get returns NULL for a key that isn't set; the string stays valid until the plugin's next
     * config_get. */
    const char* (*config_get)(dmc4_plugin* self, const char* key);
    void (*config_set)(dmc4_plugin* self, const char* key, const char* value);

    /* ImGui: the hook's context (an ImGuiContext*, NULL until the menu exists) and its allocators, for
     * ImGui::SetAllocatorFunctions. The plugin must be built with the same ImGui version (imgui_version) and
     * IMGUI_DISABLE_OBSOLETE_FUNCTIONS. */
    void* (*imgui_context)(void);
    void (*imgui_allocators)(void** alloc_fn, void** free_fn, void** user_data);
    const char* imgui_version;

    /* hooks on the game's code (the hook's own MinHook, so they can't collide with dmc4_hook's or another plugin's).
     * hook: a whole function; *original receives the address to call the original. hook_at: a jump at `address` to
     * detour, overwriting `size` bytes (whole instructions, at least 5); the detour runs those instructions itself
     * and jumps back to *resume = address + size. Both return 1 on success, 0 if the address can't be hooked (one
     * of them is already hooked, for example). Call them from dmc4_plugin_init. */
    int (*hook)(dmc4_plugin* self, uintptr_t address, void* detour, uintptr_t* original);
    int (*hook_at)(dmc4_plugin* self, uintptr_t address, void* detour, uint32_t size, uintptr_t* resume);

    /* ---- version 2 ---- */

    /* the game: singletons, state, units, resources, effects, sounds, motions (dmc4_game_api above) */
    const dmc4_game_api* game;

    /* ---- version 3 ---- */

    /* the players: count, each one, who has the keyboard, running code in a player's context (dmc4_players_api above) */
    const dmc4_players_api* players;

    /* ---- version 4 ---- */

    /* room classes derived from the game's: new rooms with their own code, scripts and memory (dmc4_rooms_api above) */
    const dmc4_rooms_api* rooms;

    /* ---- version 5 ---- */

    /* the hook's mods: their settings, held for a while (dmc4_mods_api above) */
    const dmc4_mods_api* mods;

    /* byte patches on the game's code or data, undone when the plugin stops (and by unpatch). expect (size bytes, or
     * NULL) must match what's there now, else nothing is written. Returns an id > 0, 0 on failure (logged). Game thread
     * or init. Don't patch over another mod's patch or hook. */
    int (*patch)(dmc4_plugin* self, uintptr_t address, const void* bytes, uint32_t size, const void* expect);
    void (*unpatch)(dmc4_plugin* self, int id);

    /* a row in the pause menu opening a list menu (dmc4_menus_api above) */
    const dmc4_menus_api* menus;
} dmc4_api;

#ifdef __cplusplus
}
#endif

#endif /* DMC4_PLUGIN_H */
