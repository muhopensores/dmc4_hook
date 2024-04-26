#pragma once

#include "../mod.hpp"

class PlayerTracker : public Mod {
public:
    PlayerTracker() = default;

    static uintptr_t jmp_return;
    static uPlayer* player_ptr;
    static bool lock_on_alloc;
    static bool pin_imgui_enabled;

    std::string  get_mod_name() override { return "PlayerTracker"; };
    Mod::ModType get_mod_type() override { return SLOW; };

    std::optional<std::string> on_initialize() override;
    void custom_imgui_window();
    void on_gui_frame() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook;
};
