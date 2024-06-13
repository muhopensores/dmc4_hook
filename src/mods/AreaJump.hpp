#pragma once

#include "../mod.hpp"

struct Room;

class AreaJump : public Mod {
public:
    AreaJump() = default;

    std::string get_mod_name() override { return "AreaJump"; };

    std::optional<std::string> on_initialize() override;

    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    void jump_to_stage(const Room* stage);
};
