#pragma once

#include "../mod.hpp"

class AerialStinger: public Mod {
public:
    AerialStinger() = default;
    
    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    
    void toggle(bool enable);

    std::string get_mod_name() override { return "AerialStinger"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

    private:
    std::unique_ptr<Patch> patch1, patch2;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5;
};