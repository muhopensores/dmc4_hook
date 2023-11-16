#pragma once
#define DEBUG_DRAW_EXPLICIT_CONTEXT
#include "../dependencies/debug-draw/debug_draw.hpp"
#include "../mod.hpp"

class DebugDraw : public Mod {
public:
  DebugDraw() = default;
  // mod name string for config
  std::string get_mod_name() override { return "DebugDraw"; };
  // called by m_mods->init() you'd want to override this
  std::optional<std::string> on_initialize() override;

  void custom_imgui_window();

  void on_config_load(const utility::Config& cfg) override;
  void on_config_save(utility::Config& cfg) override;

  void on_gui_frame() override;
private:

  // function hook instance for our detour, convinient wrapper 
  // around minhook
  std::unique_ptr<FunctionHook> m_function_hook;
};
