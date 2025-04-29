#pragma once

#include "../mod.hpp"

class TrickDown : public Mod {
public:
    TrickDown() = default;

    static bool mod_enabled;
    static uintptr_t trick_down_jmp_ret;
    static uintptr_t floor_touch_jmp_ret;
    static uintptr_t landing_anim_jmp_ret;

    std::string get_mod_name() override { return "TrickDown"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> trick_down_hook;
    std::unique_ptr<FunctionHook> floor_touch_hook;
    std::unique_ptr<FunctionHook> landing_anim_hook;
};
