#pragma once

#include "../mod.hpp"

class StageCallback : public Mod {
public:
    StageCallback() = default;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    std::string get_mod_name() override { return "StageCallback"; };
    std::optional<std::string> on_initialize() override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};
