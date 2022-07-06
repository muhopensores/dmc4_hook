#pragma once

#include "../mod.hpp"

class WorkRate : public Mod {
public:
    WorkRate() = default;

    std::string get_mod_name() override { return "WorkRate"; };
    static bool disable_trainer_pause;
    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
    void on_game_pause(bool toggle) override;

private:
    // std::unique_ptr<FunctionHook> hook;
    float m_global_speed = 1.0f;
    bool check_work_rate_ptr(sWorkRate* wr);
};
