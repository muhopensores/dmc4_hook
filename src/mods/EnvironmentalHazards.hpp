#pragma once
#include "../mod.hpp"

class EnvironmentalHazards : public Mod {
public:
    EnvironmentalHazards() = default;
    std::string get_mod_name() override { return "EnvironmentalHazards"; };
    // std::vector<std::string> get_search_terms() override { return {"mod sample"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;
    static bool laser_tracks_player;
    static void spawn_bp_laser();

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_stage_start();
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
};
