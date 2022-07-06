#pragma once

#include "../mod.hpp"

class OneHitKill : public Mod {
public:
    OneHitKill() = default;

    static bool cant_die;
    static bool one_hit_kill;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    void no_death_toggle(bool enable);
    // void toggle2(bool enable);

    std::string get_mod_name() override { return "OneHitKill"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;

private:
    // std::unique_ptr<Patch> patchhp;
    std::unique_ptr<Patch> patchomen;
    std::unique_ptr<FunctionHook> hook;
};
