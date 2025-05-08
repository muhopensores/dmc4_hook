#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod {
public:
    EnemySpawn() = default;

    std::string get_mod_name() override { return "EnemySpawn"; };
    static void EnemySpawn::spawn_em00x(int index);
    static void EnemySpawn::spawn_random_enemy();
    static void EnemySpawn::spawn_dante();

    std::optional<std::string> on_initialize() override;

    // void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_update_input(utility::Input& input) override;

private:
};
