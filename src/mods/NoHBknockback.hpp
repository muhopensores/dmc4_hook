#pragma once

#include "../mod.hpp"

class NoHbKnockback : public Mod {
public:
    NoHbKnockback() = default;

    static bool mod_enabled;
    static uintptr_t no_helm_breaker_knockback_continue; // = 0x0051C389;
    static uintptr_t no_helm_breaker_knockback_je;       // = 0x0051C367;

    std::string get_mod_name() override { return "NoHbKnockback"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
