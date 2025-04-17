#pragma once

#include "../mod.hpp"

class FastStart : public Mod {
public:
    FastStart() = default;

    std::string get_mod_name() override { return "FastStart"; };
    std::optional<std::string> on_initialize() override;

    void toggle(bool enable);
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook_dti;
    std::unique_ptr<FunctionHook> hook_u_fade;
    std::unique_ptr<Patch> bp_fade_patch1;
    std::unique_ptr<Patch> bp_fade_patch2;
    std::unique_ptr<Patch> shaderlog_patch;
};
