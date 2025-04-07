#pragma once

#include "../mod.hpp"

class NoclipCam : public Mod {
public:
    NoclipCam() = default;
    static bool noclip_cam_enabled;

    std::string get_mod_name() override { return "NoclipCam"; };

    std::optional<std::string> on_initialize() override;
    //void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame() override;
    void toggle_noclip_cam(bool toggle);

private:
    std::unique_ptr<Patch> patch_noclip_cam1;
    std::unique_ptr<Patch> patch_noclip_cam2;
    std::unique_ptr<Patch> patch_noclip_cam3;
};
