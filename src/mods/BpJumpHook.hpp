#pragma once
#if 0
#include "../mod.hpp"

class BpJumpHook : public Mod {
public:
    BpJumpHook() = default;

    static bool randomize_bp_toggle;
    static bool bp_boss_rush_toggle;
    static uintptr_t bp_jump_hook1_continue;
    static uintptr_t bp_jump_hook2_continue;
    static uintptr_t bp_jump_hook3_continue;
    void toggle_randomized_bp(bool enable);
    std::string get_mod_name() override { return "BpJumpHook"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;

private:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
#endif