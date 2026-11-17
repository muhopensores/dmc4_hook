#pragma once

#include "../../mod.hpp"

// Plugins: loads plugin DLLs from the game folder's plugins\ at startup and gives them the plugin API
// (plugin_sdk\include\dmc4_plugin.h; plan Vibed\Plugins\README.md). The loader and the api are PluginHost.cpp.
//
// Their frame callbacks run from a hook in sDevil4Main::move right after sCollisionGame::checkAttackMain (0x8AF03F,
// mov ecx, [ebp+0x10364]): the game thread, once per game frame, every unit ticked and the frame's hits resolved.
class Plugins : public Mod {
public:
    Plugins() = default;
    std::string get_mod_name() override { return "Plugins"; };
    std::vector<std::string> get_search_terms() override { return {"plugin", "plugins", "dll", "api", "sdk"}; }

    static bool mod_enabled;   // load plugins at start

    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook_frame;
};
