#pragma once
#if 1
#include "../mod.hpp"

class FireworksTweaks : public Mod {
public:
    FireworksTweaks() = default;

    static bool mod_enabled;
    static uintptr_t jmp_ret1;
    static uintptr_t jmp_ret2;

    std::string get_mod_name() override { return "FireworksTweaks"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void on_frame(fmilliseconds& dt) override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    std::unique_ptr<FunctionHook> hook1, hook2;

    static float fireworksStartFrame;
};
#endif
