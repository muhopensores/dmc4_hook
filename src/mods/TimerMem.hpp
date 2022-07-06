#pragma once

#include "../mod.hpp"

class TimerMem : public Mod {
public:
    TimerMem() = default;

    static bool instant_honeycomb_enabled;
    static uintptr_t timer_jmp_ret;
    static uintptr_t back_forward_jmp_ret;
    static float timer_mem;

    std::string get_mod_name() override { return "TimerMem"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> timer_hook;
    std::unique_ptr<FunctionHook> back_forward_hook;
};
