#pragma once

#include "../mod.hpp"

namespace background_rendering {
    LRESULT WINAPI on_message_wnd_proc_return(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);
}

class BackgroundRendering : public Mod {
public:
    BackgroundRendering() = default;
    
    Mod::ModType get_mod_type() override { return SLOW; };

    std::string get_mod_name() override { return "BackgroundRendering"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;

private:
};
