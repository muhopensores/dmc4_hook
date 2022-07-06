#pragma once

#include "../mod.hpp"

class DamageMultiplier : public Mod {
public:
    DamageMultiplier() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;
    static float enemy_hp_display;

    std::string get_mod_name() override { return "DamageMultiplier"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;
    //void onFrame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
