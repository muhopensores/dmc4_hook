#pragma once

#include "../mod.hpp"

class PhotoMode : public Mod {
public:
    
    static bool mod_enabled;
    static bool photo_mode_open;

    static float freecamSpeed;
    static float freecamModifierSpeed;
    static bool toggle_gameplay_cam;
    static bool projectileTest;
    static bool freecamMouseControls;
    static bool freecamKeyboardControls;
    static bool freecamGamepadControls;

    PhotoMode() = default;
    std::string get_mod_name() override { return "PhotoMode"; };
    //std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

};