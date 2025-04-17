#pragma once

#include "../mod.hpp"

class TrainerPause : public Mod {
public:
    TrainerPause() = default;

    std::string get_mod_name() override { return "TrainerPause"; };
    static bool mod_enabled;
    std::optional<std::string> on_initialize() override;

    // void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void on_game_pause(bool toggle) override;
    // void on_update_input(utility::Input& input) override;

private:

};
