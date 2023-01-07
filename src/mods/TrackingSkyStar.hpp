#pragma once

#include "../mod.hpp"

class TrackingSkyStar : public Mod {
public:
    TrackingSkyStar() = default;

    static bool mod_enabled;
    static uintptr_t tracking_sky_star_continue;

    std::string get_mod_name() override { return "TrackingSkyStar"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
