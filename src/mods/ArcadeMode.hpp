#pragma once
#include "../mod.hpp"

class ArcadeMode : public Mod {
public:
    ArcadeMode() = default;

    static bool mod_enabled;
    std::string get_mod_name() override { return "ArcadeMode"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_msel_mstart, hook_bpstart;
};
