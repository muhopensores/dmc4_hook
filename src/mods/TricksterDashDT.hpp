#pragma once
#include "../mod.hpp"

class TricksterDashDT : public Mod {
public:
    TricksterDashDT() = default;

    static bool mod_enabled;

    void toggle1(bool enable);

    std::string get_mod_name() override { return "TricksterDashDT"; };
    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;
    // void on_frame(fmilliseconds& dt) override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;

private:
    std::unique_ptr<Patch> patch1;
};
