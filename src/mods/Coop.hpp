#pragma once

#include "../mod.hpp"

extern unsigned int player_num;

class Coop : public Mod {
public:
    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    Coop() = default;
    std::string get_mod_name() override { return "Coop"; };
    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
    //void on_config_load(const utility::Config& cfg) override;
    //void on_config_save(utility::Config& cfg) override;
private:
    std::unique_ptr<FunctionHook> hook1, hook2;
};