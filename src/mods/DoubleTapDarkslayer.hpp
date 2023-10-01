#pragma once

#include "../mod.hpp"

class DoubleTapDarkslayer : public Mod {
public:
    DoubleTapDarkslayer() = default;

    static uintptr_t jmp_ret1;

    static uintptr_t jmp_ret2;
    static uintptr_t jmp_jb2;

    static uintptr_t jmp_ret3;
    static uintptr_t jmp_jb3;

    static uintptr_t jmp_ret4;
    static uintptr_t jmp_jb4;

    static uintptr_t jmp_ret5;
    static uintptr_t jmp_jb5;

    static bool mod_enabled;

    static float darkslayerTimer;
    static float darkslayerTimerFill;
    static float zeroFloat;

    std::string get_mod_name() override { return "DoubleTapDarkslayer"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
    std::unique_ptr<FunctionHook> hook4;
    std::unique_ptr<FunctionHook> hook5;
    // std::unique_ptr<Patch> patch;
};
