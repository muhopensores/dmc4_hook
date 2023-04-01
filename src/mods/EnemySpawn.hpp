#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod {
public:
    EnemySpawn() = default;

    std::string get_mod_name() override { return "EnemySpawn"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;

private:
};