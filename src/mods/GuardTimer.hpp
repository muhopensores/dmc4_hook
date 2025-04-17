#pragma once

#include "../mod.hpp"

class GuardTimer : public Mod {
public:
    GuardTimer() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static float lastGuardTime;

    std::string  get_mod_name() override { return "GuardTimer"; };

    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
    // void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2, hook3;
};
