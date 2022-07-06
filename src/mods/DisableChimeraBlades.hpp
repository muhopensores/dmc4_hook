#pragma once

#include "../mod.hpp"

class DisableChimeraBlades : public Mod {
public:
    DisableChimeraBlades() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret;

    std::string get_mod_name() override { return "DisableChimeraBlades"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
