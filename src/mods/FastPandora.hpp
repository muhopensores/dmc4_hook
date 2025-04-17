#pragma once

#include "../mod.hpp"

class FastPandora : public Mod {
public:
    FastPandora() = default;

    static bool mod_enabled;
    static bool mod2_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;

    void toggle(bool enable);

    std::string get_mod_name() override { return "FastPandora"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<Patch> patch;
    std::unique_ptr<FunctionHook> hook1, hook2, hook3;
};
