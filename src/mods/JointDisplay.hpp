#pragma once
#include "../mod.hpp"

class JointDisplay : public Mod {
public:
    JointDisplay() = default;
    std::string get_mod_name() override { return "JointDisplay"; };
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool mod_enabled;

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    // std::unique_ptr<Patch> patch1;
    // std::unique_ptr<FunctionHook> hook1;
};
