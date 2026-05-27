#pragma once

#include "../mod.hpp"

class AnimationOverlay : public Mod {
public:
    AnimationOverlay() = default;

    std::string get_mod_name() override { return "AnimationOverlay"; };
    std::vector<std::string> get_search_terms() override {
        return {"animation", "anim frame", "animation overlay", "attack id"};
    }

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    static bool mod_enabled;
    static bool show_advanced;
    static ImVec2 window_pos;
};
