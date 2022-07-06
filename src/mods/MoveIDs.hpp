#pragma once

#include "../mod.hpp"

class MoveIds : public Mod {
public:
    MoveIds() = default;

    static uintptr_t jmp_return;
    static uint32_t move_id;

    std::string get_mod_name() override { return "MoveIds"; };

    std::optional<std::string> on_initialize() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
