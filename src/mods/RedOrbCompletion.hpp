#pragma once

#include "../mod.hpp"

class RedOrbCompletion : public Mod {
public:
    RedOrbCompletion() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    void custom_imgui_window();

    std::string get_mod_name() override { return "RedOrbCompletion"; };
    Mod::ModType get_mod_type() override { return SLOW; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
