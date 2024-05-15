#pragma once

#include "../mod.hpp"

struct Room;

class AreaJump : public Mod {
public:
    AreaJump() = default;

    static uintptr_t jmp_return;
    static cAreaJump* c_area_jump_ptr;

    std::string get_mod_name() override { return "AreaJump"; };

    std::optional<std::string> on_initialize() override;

    void on_gui_frame() override;
    void on_update_input(utility::Input& input) override;

private:
    void jump_to_stage(const Room* stage);

    std::unique_ptr<FunctionHook> hook;
};
