#pragma once

#include "../mod.hpp"

class InfAirHikes : public Mod {
public:
    InfAirHikes() = default;

    static bool mod_enabled_nero;
    static bool mod_enabled_dante;

    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    std::string get_mod_name() override { return "InfAirHikes"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};
