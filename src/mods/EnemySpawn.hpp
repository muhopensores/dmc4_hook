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
    std::size_t m_spawn_scarecrow_leg_command;
    std::size_t m_spawn_scarecrow_arm_command;
    std::size_t m_spawn_mega_command;
    std::size_t m_spawn_bianco_command;
    std::size_t m_spawn_alto_command;
    std::size_t m_spawn_mephisto_command;
    std::size_t m_spawn_faust_command;
    std::size_t m_spawn_frost_command;
    std::size_t m_spawn_assault_command;
    std::size_t m_spawn_blitz_command;
    std::size_t m_spawn_chimera_command;
    std::size_t m_spawn_basilisk_command;
    std::size_t m_spawn_berial_command;
    std::size_t m_spawn_bael_command;
    std::size_t m_spawn_echidna_command;
    std::size_t m_spawn_credo_command;
    std::size_t m_spawn_agnus_command;
    std::size_t m_spawn_sanctus_dia_command;
    std::size_t m_spawn_kyrie_command;
    std::size_t m_spawn_dante_command;
};
