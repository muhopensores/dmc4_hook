#pragma once
#include "../mod.hpp"

class InfinitePinTimer : public Mod {
public:
    InfinitePinTimer() = default;
    std::string get_mod_name() override { return "InfinitePinTimer"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;
    static uintptr_t jmp_ret1;

    void toggle1(bool enable);

    // void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<Patch> patch1, patch2;
    // std::unique_ptr<FunctionHook> hook1;
};
