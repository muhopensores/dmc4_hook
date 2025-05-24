#pragma once

#include "../mod.hpp"

class RedOrbCompletion : public Mod {
public:
    RedOrbCompletion() = default;

    static bool mod_enabled;
    static ImVec2 window_pos;

    std::string get_mod_name() override { return "RedOrbCompletion"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
