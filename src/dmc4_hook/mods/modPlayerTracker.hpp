#pragma once

#include "../mod.hpp"

class PlayerTracker : public Mod
{
public:
    PlayerTracker();

    static uintptr_t jmp_return;
    // static uintptr_t* player_base_ptr;
    static uPlayer* player_ptr;
    static bool lockOnAlloc;
    std::string getModName() override { return "PlayerTracker"; };

    std::optional<std::string> onInitialize() override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};
