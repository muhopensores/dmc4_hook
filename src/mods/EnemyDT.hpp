#pragma once

#include "../mod.hpp"

class EnemyDT : public Mod {
public:
    EnemyDT() = default;

    static bool mod_enabled_no_dt;
    static bool mod_enabled_instant_dt;

    void toggle_no_dt(bool enable);
    void toggle_instant_dt(bool enable);

    std::string get_mod_name() override { return "EnemyDT"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
    std::unique_ptr<Patch> patch3;
};
