#pragma once

#include "../mod.hpp"

class EnemyStepDisplay : public Mod {
public:
    EnemyStepDisplay() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static bool jc_possible;
    static float jc_possible_timer;

    std::string get_mod_name() override { return "EnemyStepDisplay"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
