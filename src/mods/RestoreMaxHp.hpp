#pragma once

#include "../mod.hpp"

class RestoreMaxHp : public Mod {
public:
    RestoreMaxHp() = default;

    static bool mod_enabled;

    std::string get_mod_name() override { return "RestoreMaxHp"; };

    std::optional<std::string> on_initialize() override;

    void restore_health_and_timer(SMediator* s_med_ptr, uPlayer* player);

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
};
