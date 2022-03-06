#pragma once

#include "../mod.hpp"

class DeltaTime : public Mod
{
public:
    DeltaTime();

    static uintptr_t jmp_return;
    static float currentDeltaTime;

    std::string getModName() override { return "DeltaTime"; };

    std::optional<std::string> onInitialize() override;

private:
    hl::Hooker hook;
};
