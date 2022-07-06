#pragma once

#include "../mod.hpp"

class InputStates : public Mod {
public:
    InputStates() = default;

    static uintptr_t jmp_return;
    static uintptr_t jmp_return2;
    static uintptr_t jmp_return3;

    static uint32_t inputpressed;
    static float input_timer;
    static float input_timer2;
    static bool touchpad_rose_enabled;
    static bool rose_timer_active;

    void rose_input();
    void rose_buffer();
    void play_rose();
    void on_timer_callback();

    std::string get_mod_name() override { return "InputStates"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
};
