#pragma once

#include "../mod.hpp"

class MoveIdsNero : public Mod {
public:
    MoveIdsNero() = default;

    static uintptr_t jmp_return;
    static uint32_t move_id_nero;

    std::string get_mod_name() override { return "MoveIdsNero"; };

    std::optional<std::string> on_initialize() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
