#pragma once

#include "../mod.hpp"

class EnemyTracker : public Mod {
public:
    EnemyTracker() = default;

    std::string get_mod_name() override { return "EnemyTracker"; };

    std::optional<std::string> on_initialize() override;

    int get_enemy_specific_damage_offset(int enemy_id);

    static int hotkey2;
    static int hotkey3;
    static int hotkey4;

    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;
    void on_config_load(const utility::Config& cfg);
    void on_config_save(utility::Config& cfg);

private:
    std::unique_ptr<FunctionHook> hook;
};
