#pragma once

#include "../mod.hpp"

class EnemyStepDisplay : public Mod {
public:
    EnemyStepDisplay() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static bool jc_possible;
    static float jc_possible_timer;
    static bool showExtraStats;
    static ImVec2 windowPos;
    static void update_jc_possible();

    static bool chart_enabled;
    static void record_damage();
    static void record_enemy_step_attempt();

    std::string get_mod_name() override { return "EnemyStepDisplay"; };
    std::vector<std::string> get_search_terms() override {
        return {"jc", "enemy step"
        };
    }
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2, hook3;
};
