#pragma once

#include "../mod.hpp"

class InfAllHealth : public Mod {
public:
    InfAllHealth() = default;

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

    std::string get_mod_name() override { return "InfAllHealth"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void onFrame(fmilliseconds& dt) override;
    void on_update_input(utility::Input& input) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
