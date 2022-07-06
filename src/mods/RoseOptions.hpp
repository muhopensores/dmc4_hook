#pragma once

#include "../mod.hpp"

class RoseOptions : public Mod {
public:
    RoseOptions() = default;

    void toggle_disable_rose_despawn_on_hitting_enemy(bool enabled);
    void toggle_disable_rose_despawn_on_hitting_object(bool enabled);
    void toggle_rose_infinite_timer(bool enabled);

    std::string get_mod_name() override { return "RoseOptions"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
};
