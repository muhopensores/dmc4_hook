#pragma once

#include "../mod.hpp"

class BrainrotPalace: public Mod {
public:

    BrainrotPalace() = default;
    std::string get_mod_name() override { return "BrainrotPalace"; };
    // std::vector<std::string> get_search_terms() override { return {"mod sample"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_reset() override;
    void after_reset() override;

    void on_stage_start() override;
    void on_stage_end() override;

    bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
};

