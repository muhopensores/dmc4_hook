#pragma once

#include "../mod.hpp"

class EnemyReplaceAgain : public Mod {
public:
    EnemyReplaceAgain() = default;

    static bool mod_enabled;
    std::unique_ptr<Patch>& get_patch(int enemy_id);
    uintptr_t get_enemy_address(int enemy_id);
    void replace_enemy_with(int current_enemy_id, int desired_enemy_id);

    static int desired_enemy[];
    static int default_enemy[];

    std::string get_mod_name() override { return "EnemyReplaceAgain"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

private:
    std::unique_ptr<Patch> replacement_address_two_patch, replace_scarecrow_leg_patch, replace_scarecrow_arm_patch, replace_scarecrow_mega_patch,
        replace_angelo_bianco_patch, replace_angelo_alto_patch, replace_mephisto_patch, replace_faust_patch, replace_frost_patch,
        replace_assault_patch, replace_blitz_patch, replace_chimera_patch, replace_basilisk_patch, replace_berial_patch, replace_bael_patch,
        replace_echidna_patch, replace_credo_patch, replace_agnus_patch, replace_sanctus_patch, replace_sanctus_dia_patch, replace_kyrie_patch,
        replace_dante_patch;
};
