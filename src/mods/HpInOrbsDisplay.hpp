#pragma once

#include "../mod.hpp"

class HpInOrbsDisplay : public Mod {
public:
    HpInOrbsDisplay() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    std::string get_mod_name() override { return "HpInOrbsDisplay"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
