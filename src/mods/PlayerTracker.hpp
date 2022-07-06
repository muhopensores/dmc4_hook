#pragma once

#include "../mod.hpp"

class PlayerTracker : public Mod {
public:
    PlayerTracker() = default;

    static uintptr_t jmp_return;
    // static uintptr_t* player_base_ptr;
    static uPlayer* player_ptr;
    static bool lock_on_alloc;
    std::string get_mod_name() override { return "PlayerTracker"; };

    std::optional<std::string> on_initialize() override;

    void on_gui_frame() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
