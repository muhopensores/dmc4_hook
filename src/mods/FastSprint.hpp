#pragma once

#include "../mod.hpp"

class FastSprint : public Mod {
public:
    FastSprint() = default;

    static uintptr_t jmp_ret;
    static bool mod_enabled;
    static float newSprintTimer;

    std::string get_mod_name() override { return "FastSprint"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
    //std::unique_ptr<Patch> patch;
};
