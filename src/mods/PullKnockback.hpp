#pragma once

#include "../mod.hpp"

class PullKnockback: public Mod {
public:
    static uintptr_t jmp_ret1;

    PullKnockback() = default;

    std::string get_mod_name() override { return "PullKnockback"; };
    std::optional<std::string> on_initialize() override;
    //void on_config_load(const utility::Config& cfg) override;
    //void on_config_save(utility::Config& cfg) override;
    //void on_gui_frame(int display) override;
private:
    std::unique_ptr<FunctionHook> hook1;
};