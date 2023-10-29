#pragma once

#include "../mod.hpp"

class GuardSteer : public Mod {
public:
    GuardSteer() = default;

    static bool mod_enabled;
    static uintptr_t guard_steer_continue;

    std::string get_mod_name() override { return "GuardSteer"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
