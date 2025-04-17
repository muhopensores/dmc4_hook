#pragma once

#include "../mod.hpp"

class WorkRate : public Mod {
public:
    WorkRate() = default;

    static bool force_custom_turbo;
    static float custom_turbo;
    static bool hotkey_paused;

    std::string get_mod_name() override { return "WorkRate"; };
    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_game_pause(bool toggle) override;
    void on_update_input(utility::Input& input) override;

private:
    // std::unique_ptr<FunctionHook> hook;
    float m_global_speed = 1.0f;
    bool check_work_rate_ptr(sWorkRate* wr);
};
