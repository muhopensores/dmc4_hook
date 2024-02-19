#pragma once

#include "../mod.hpp"

class AssaultsDontJump : public Mod {
public:
    AssaultsDontJump() = default;

    static bool mod_enabled;

    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;
    //void toggle(bool enable);

    std::string get_mod_name() override { return "AssaultsDontJump"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
    //std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook;
};
