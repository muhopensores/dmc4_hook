#pragma once
#include "../mod.hpp"
class MoveLine;

class PhotoMode : public Mod {
public:
    
    static bool mod_enabled;
    static bool joint_rotation_enable;
    static float freecamSpeed;
    static float freecamModifierSpeed;
    static bool toggle_gameplay_cam;
    static bool projectileTest;
    static bool freecamMouseControls;
    static bool freecamKeyboardControls;
    static bool freecamGamepadControls;
    static float HUDCooldown;

    PhotoMode() = default;
    std::string get_mod_name() override { return "PhotoMode"; };
    std::vector<std::string> get_search_terms() override {
        return {
            "photo mode", "screenshot", "camera"
        };
    }
    void joint_rotation_toggle(bool enable);
    static void drawLightInfo(MoveLine* moveline, int& numOfObjs);

    //std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

    // void on_config_load(const utility::Config& cfg) override;
    // void on_config_save(utility::Config& cfg) override;
private:
    std::unique_ptr<Patch> patch1, patch2, patch3, patch4;
};
