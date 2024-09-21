#pragma once

#include "../mod.hpp"

class TauntSelect : public Mod {
public:
    TauntSelect() = default;

    static bool mod_enabled;
    static bool mod_enabled2;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static uintptr_t jmp_ret3;

    std::string get_mod_name() override { return "TauntSelect"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook, hook2, hook3;
};
