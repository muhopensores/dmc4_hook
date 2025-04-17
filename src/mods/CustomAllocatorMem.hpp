#pragma once

#include "../mod.hpp"


class CustomAllocatorMem: public Mod {
public:
    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;

    CustomAllocatorMem() = default;

    void toggle(bool enable);
    std::string get_mod_name() override { return "CustomAllocatorMem"; };
    Mod::ModType get_mod_type() override { return SLOW; }
    std::optional<std::string> on_initialize() override;
    //void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;
    // void on_gui_frame(int display) override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4;
    std::unique_ptr<Patch> patch1, patch2, patch3, patch4, patch5, patch6, patch7, patch8, patch9, patch10;
};