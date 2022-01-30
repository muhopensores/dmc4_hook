#pragma once

#include "../mod.hpp"

class InputStates : public Mod
{
public:
    InputStates();

    static uintptr_t jmp_return;
    static uint32_t inputpressed;
    static float inputTimer;

    std::string getModName() override { return "InputStates"; };

    std::optional<std::string> onInitialize() override;

private:
    hl::Hooker hook;
};