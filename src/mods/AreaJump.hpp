#pragma once

#include "../mod.hpp"

class AreaJump : public Mod {
public:
    AreaJump() = default;

    static uintptr_t jmp_return;
    static cAreaJump* c_area_jump_ptr;

    std::string get_mod_name() override { return "AreaJump"; };

    std::optional<std::string> on_initialize() override;

    void on_gui_frame() override;

private:
    void jump_to_stage(int stage);
    std::unique_ptr<FunctionHook> hook;
};
