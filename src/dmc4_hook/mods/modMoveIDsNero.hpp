#pragma once

#include "../mod.hpp"

class MoveIdsNero : public Mod {
public:
    MoveIdsNero();

    static uintptr_t jmp_return;
    static uint32_t moveIDNero;

    std::string getModName() override { return "MoveIdsNero"; };

    std::optional<std::string> onInitialize() override;

private:
    hl::Hooker hook;
};