#pragma once

#include "../mod.hpp"

class DebugCam : public Mod {
public:
    DebugCam() = default;

    // static bool mod_enabled;
    static uintptr_t jmp_ret1;

    static float freecamSpeed;
    static float freecamModifierSpeed;
    static bool freecamMouseControls;
    static bool freecamKeyboardControls;
    static bool freecamGamepadControls;

    // void toggle(bool enable);
    std::string get_mod_name() override { return "DebugCam"; };
    std::optional<std::string> on_initialize() override;

    void on_gui_frame() override;
    // void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1;
};

enum REGION_MODE {
    REGION_FULLSCREEN,
    REGION_FREE,
    REGION_TOP,
    REGION_BOTTOM,
    REGION_LEFT,
    REGION_RIGHT,
    REGION_TOPLEFT,
    REGION_BOTTOMLEFT,
    REGION_TOPRIGHT,
    REGION_BOTTOMRIGHT
};
static const char* validViewportsComboStr =
    "FULLSCREEN\0"
    "FREE\0"
    "TOP\0"
    "BOTTOM\0"
    "LEFT\0"
    "RIGHT\0"
    "TOP LEFT\0"
    "BOTTOM LEFT\0"
    "TOP RIGHT\0"
    "BOTTOM RIGHT\0\0";
