#pragma once

#include "../mod.hpp"

class DeltaTime : public Mod {
public:
    DeltaTime() = default;

    static uintptr_t jmp_return;
    static float current_delta_time;

    std::string get_mod_name() override { return "DeltaTime"; };

    std::optional<std::string> on_initialize() override;

private:
    std::unique_ptr<FunctionHook> hook;
};
