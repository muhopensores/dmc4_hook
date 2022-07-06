#pragma once

#include "../mod.hpp"

class SlowWalk : public Mod {
public:
    SlowWalk() = default;

    static bool mod_enabled;

    void toggle(bool enable);

    std::string get_mod_name() override { return "SlowWalk"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
