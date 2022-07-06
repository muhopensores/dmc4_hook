#pragma once

#include "../mod.hpp"

class DisableDarkslayer : public Mod {
public:
    DisableDarkslayer() = default;

    static bool mod_enabled_up;
    static bool mod_enabled_down;
    static bool mod_enabled_left;
    static bool mod_enabled_right;

    void toggle_up(bool enable);
    void toggle_down(bool enable);
    void toggle_left(bool enable);
    void toggle_right(bool enable);

    std::string get_mod_name() override { return "DisableDarkslayer"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
    std::unique_ptr<Patch> patch4;
};
