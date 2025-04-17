#pragma once

#include "../mod.hpp"

class SwordSpin: public Mod {
public:
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;
    static bool mod_enabled;

    SwordSpin() = default;

    void toggle(bool enable);
    std::string get_mod_name() override { return "SwordSpin"; };
    std::optional<std::string> on_initialize() override;
     void on_config_load(const utility::Config& cfg) override;
     void on_config_save(utility::Config& cfg) override;
     void on_gui_frame(int display) override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};