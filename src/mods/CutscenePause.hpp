#pragma once

#include "../mod.hpp"

class CutscenePause : public Mod {
public:
    CutscenePause() = default;

    static bool mod_enabled;
    static float cutsceneProgress;
    static float cutsceneLength;
    static uintptr_t jmp_ret;

    std::string get_mod_name() override { return "CutscenePause"; };

    std::optional<std::string> on_initialize() override;
    void on_update_input(utility::Input& input) override;
    
    // void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1;
};
