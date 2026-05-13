#pragma once

#include "../mod.hpp"

class HealthSettings : public Mod {
public:
    HealthSettings() = default;

    static uintptr_t jmp_return;
    static bool all_invincible;
    static bool player_invincible;
    static bool enemy_invincible;
    static bool damageMultiplierToggle;
    static bool mustStyleToggle;
    static bool mightStyleToggle;
    static float enemy_hp_display;
    static float outgoingDamageMultiplier;
    static float incomingDamageMultiplier;
    static float mightStyleMultiplier;

    static bool cant_die;
    static bool one_hit_kill;
    static uintptr_t one_hit_kill_jmp_ret;
    static uintptr_t one_hit_kill_jmp_out;
    void no_death_toggle(bool enable);

    std::string get_mod_name() override { return "HealthSettings"; };
    std::vector<std::string> get_search_terms() override { return {"inf hp", "inf all hp", "infinite hp", "health", "damage multipliers", "damage modifiers", "incoming", "outgoing, one hit kill"}; }
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void onFrame(fmilliseconds& dt) override;
    void on_update_input(utility::Input& input) override;

private:
    std::unique_ptr<FunctionHook> health_hook;

    std::unique_ptr<Patch> omen_patch;
    std::unique_ptr<FunctionHook> one_hit_kill_hook;
};
