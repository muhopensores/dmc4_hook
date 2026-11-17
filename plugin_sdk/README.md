# dmc4_hook plugin SDK

Plugins are 32-bit DLLs that dmc4_hook loads from the game folder's `dmc4_hook_plugins\` when the game starts. They
get a table of functions (`include\dmc4_plugin.h`): callbacks on every game frame and in the hook's menu, a log,
settings saved in `dmc4_hook.cfg`, and hooks on the game's code through the hook's own MinHook.

## Making one

1. Include `include\dmc4_plugin.h`.
2. Export the two functions the hook looks for:
   - `DMC4_PLUGIN_ABI_EXPORT()` (writes `dmc4_plugin_abi`)
   - `DMC4_PLUGIN_EXPORT int dmc4_plugin_init(const dmc4_api* api, dmc4_plugin* self, dmc4_plugin_info* info)`, which
     fills `info`, registers callbacks and installs hooks, and returns 0.
3. Build a 32-bit DLL. `example\build_example.bat` builds `example\hello_plugin.cpp` with Visual Studio 2022.
4. Copy it to `<game folder>\dmc4_hook_plugins\` and start the game. The hook's menu lists it under Plugins, with its
   status, log and panel.

## Rules

- **C interface only:** no C++ types across the boundary, and no C++ exception may leave a callback.
- **Threads:** `on_frame` runs on the game thread once per game frame, after the frame's hits; `on_panel` runs where
  the menu is drawn. Change game state only from `on_frame`.
- **ImGui** in a panel: build with the same ImGui version as the hook (`api->imgui_version`) and
  `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`; call `ImGui::SetAllocatorFunctions` with `api->imgui_allocators` once, and
  `ImGui::SetCurrentContext(api->imgui_context())` at the start of every panel callback.
- **Hooks** only through `api->hook` / `api->hook_at`, from `dmc4_plugin_init`.
- **Crashes:** a plugin that crashes in a callback is stopped and the menu shows where (module + offset). Its DLL
  stays loaded, so restart the game after fixing it.
- **Versions:** `api->version` tells which functions exist (each one says the version that added it in the header).
  A change that breaks plugins raises `DMC4_PLUGIN_ABI`; the hook refuses plugins built for another ABI.
