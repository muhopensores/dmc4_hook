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
#define DMC4_PLUGIN_API_VERSION 3  /* additions: functions appended to dmc4_api (and to the tables it points to) */

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
} dmc4_api;

#ifdef __cplusplus
}
#endif

#endif /* DMC4_PLUGIN_H */
