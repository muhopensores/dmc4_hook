#pragma once

#include "../mod.hpp"

class RisingSun : public Mod {
public:
    RisingSun() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;
    static uintptr_t jmp_ret4;
    static uintptr_t jmp_ret5;
    static uintptr_t jmp_ret6;
    static uintptr_t jmp_ret7;

    void toggle(bool enable);

    std::string get_mod_name() override { return "RisingSun"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1, patch2;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3, hook4, hook5, hook6, hook7;
};
