# dmc4_hook plugin SDK

Plugins are 32-bit DLLs that dmc4_hook loads from the game folder's `plugins\` when the game starts. They
get a table of functions (`include\dmc4_plugin.h`): callbacks on every game frame and in the hook's menu, a log,
settings saved in `dmc4_hook.cfg`, and hooks on the game's code through the hook's own MinHook.

## Making one

1. Include `include\dmc4_plugin.h`.
2. Export the two functions the hook looks for:
   - `DMC4_PLUGIN_ABI_EXPORT()` (writes `dmc4_plugin_abi`)
   - `DMC4_PLUGIN_EXPORT int dmc4_plugin_init(const dmc4_api* api, dmc4_plugin* self, dmc4_plugin_info* info)`, which
     fills `info`, registers callbacks and installs hooks, and returns 0.
3. Build a 32-bit DLL. `example\build_example.bat` builds `example\hello_plugin.cpp` with Visual Studio 2022.
4. Copy it to `<game folder>\plugins\` and start the game. The hook's menu lists it under Plugins, with its
   status, log and panel.

## The game's types

`include\dmc4_game.hpp` brings in dmc4_hook's SDK headers that describe the game's memory: MT Framework basics
(`MtObject`, `MtDTI`, `MtMath` ...), the systems (`sMediator`, `sUnit`, `sArea`, `sRender` ...), units and players
(`cUnit`, `uActor`, `uPlayer`, `uPlayerDante`, `uEnemy`, `uLight` ...). Cast the `void*` from `api->game` to them. They
are the same files the hook compiles against (`src\sdk`), and the game's layouts never change, so a plugin built
against an older copy keeps working. The hook's services (`src\sdk\*.cpp`) aren't included: they go through
`dmc4_plugin.h`.

Include paths: `plugin_sdk\include`, `src`, `src\sdk` and `dependencies\glm` of the dmc4_hook repo. MSVC, 32-bit,
C++20. Never construct these classes or define their virtual functions; read and write them from `on_frame` or a hook.

## Rules

- **C interface only:** no C++ types across the boundary, and no C++ exception may leave a callback.
- **Threads:** `on_frame` runs on the game thread once per game frame, after the frame's hits; `on_panel` runs where
  the menu is drawn. Change game state only from `on_frame`.
- **ImGui** in a panel: build with the same ImGui version as the hook (`api->imgui_version`) and
  `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`; call `ImGui::SetAllocatorFunctions` with `api->imgui_allocators` once, and
  `ImGui::SetCurrentContext(api->imgui_context())` at the start of every panel callback.
- **Hooks** only through `api->hook` / `api->hook_at`, from `dmc4_plugin_init`.
- **Co-op:** up to four players. Get them from `api->players`, never treat `DMC4_OBJ_LOCAL_PLAYER` as the only
  one, and run game code meant for player 2-4 inside `api->players->run_as` (it puts that player where the
  game's "the player" reads are, and back afterwards).
- **Rooms** (`api->rooms`, version 4): register room classes only in `dmc4_plugin_init` (the hook makes them before
  the game records its area classes; later is too late). Use room numbers 900-999. A room callback that replaces a
  step (`init`, `final`, `load`) calls `api->rooms->base` to run the room's own step. Event script text is checked
  with `api->rooms->check_script`. The example registers room 901.
- **Crashes:** a plugin that crashes in a callback is stopped and the menu shows where (module + offset). Its DLL
  stays loaded, so restart the game after fixing it.
- **Versions:** `api->version` tells which functions exist (each one says the version that added it in the header).
  A change that breaks plugins raises `DMC4_PLUGIN_ABI`; the hook refuses plugins built for another ABI.
