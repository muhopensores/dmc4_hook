#pragma once

#include "../mod.hpp"

// These are not Enemy IDs, they are what we use for spawnable enemies
enum class EnemyType : int {
    SCARECROW_LEG = 0,
    SCARECROW_ARM = 1,
    SCARECROW_MEGA = 2,
    ANGELO_BIANCO = 3,
    ANGELO_ALTO = 4,
    MEPHISTO = 5,
    FAUST = 6,
    FROST = 7,
    ASSAULT = 8,
    BLITZ = 9,
    CHIMERA_SEED = 10,
    CUTLASS = 11,
    GLADIUS = 12,
    BASILISK = 13,
    BERIAL = 14,
    BAEL = 15,
    ECHIDNA = 16,
    CREDO = 17,
    AGNUS = 18,
    SANCTUS = 19,
    SANCTUS_DIABOLICA = 20,
    KYRIE = 21,
    DANTE = 22,
};

class EnemySpawn : public Mod {
public:
    EnemySpawn() = default;

    std::string get_mod_name() override { return "EnemySpawn"; };
    static void EnemySpawn::spawn_em00x(EnemyType index);
    static void EnemySpawn::spawn_random_enemy();
    static void EnemySpawn::spawn_dante();

    std::optional<std::string> on_initialize() override;

    // void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    void on_update_input(utility::Input& input) override;

private:
};
