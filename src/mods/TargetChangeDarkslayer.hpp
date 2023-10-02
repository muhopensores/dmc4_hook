#pragma once

#include "../mod.hpp"

class TargetChangeDarkslayer : public Mod {
public:
    TargetChangeDarkslayer() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_conditional;
    void toggle(bool enable);

    std::string get_mod_name() override { return "TargetChangeDarkslayer"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<Patch> patch;
};
