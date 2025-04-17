#pragma once

#include <Windows.h>

#include "../mod.hpp"

class Borderless : public Mod {
public:
    struct Monitor {
        std::string name;
        RECT rect;
        RECT work_rect;
    };

    Borderless() = default;

    Mod::ModType get_mod_type() override { return SLOW; }

    std::string get_mod_name() override { return "Borderless"; };

    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    auto& get_monitors() { return m_monitors; }

private:
    std::vector<Monitor> m_monitors;
    LONG m_default_style;
    LONG m_style;
    int m_x, m_y, m_w, m_h;
    bool m_change_style;
    bool m_change_pos;

    int m_style_choice;
    bool m_is_choice_fulfilled;
    bool m_enabled;
    void apply();
    void undo();
};
