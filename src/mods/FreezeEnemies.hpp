#pragma once

#include "../mod.hpp"

class FreezeEnemies : public Mod {
public:
    FreezeEnemies() = default;

    static bool mod_enabled;

    void toggle(bool enable);

    std::string get_mod_name() override { return "FreezeEnemies"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    // std::unique_ptr<FunctionHook> hook;
    std::unique_ptr<Patch> scarecrow_patch;
    std::unique_ptr<Patch> frost_patch;
    std::unique_ptr<Patch> armour_patch;
    std::unique_ptr<Patch> ghost_patch;
    std::unique_ptr<Patch> flying_sword_patch;
    std::unique_ptr<Patch> fish_sword_patch;
    std::unique_ptr<Patch> seed_patch;
    std::unique_ptr<Patch> assault_patch;
    std::unique_ptr<Patch> dog_patch;
    std::unique_ptr<Patch> blitz_patch;
};
