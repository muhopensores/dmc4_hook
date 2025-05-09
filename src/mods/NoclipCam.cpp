#include "NoclipCam.hpp"

bool  NoclipCam::noclip_cam_enabled = false;

void NoclipCam::toggle_noclip_cam(bool toggle) {
    if (toggle) {
        install_patch_offset(0xF9318, patch_noclip_cam1, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0xF9334, patch_noclip_cam2, "\x90\x90\x90\x90\x90\x90", 6);
        install_patch_offset(0x180C1, patch_noclip_cam3, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch_noclip_cam1.reset();
        patch_noclip_cam2.reset();
        patch_noclip_cam3.reset();
    }
}

void NoclipCam::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Noclip Cam"), &noclip_cam_enabled)) {
        toggle_noclip_cam(noclip_cam_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Remove camera presets and instead use a camera that can move through walls\nEnable before entering a stage"));
}

std::optional<std::string> NoclipCam::on_initialize() {
    return Mod::on_initialize();
}


void NoclipCam::on_config_load(const utility::Config& cfg) {
    noclip_cam_enabled = cfg.get<bool>("noclip_cam").value_or(false);
    if (noclip_cam_enabled) toggle_noclip_cam(noclip_cam_enabled);
}

void NoclipCam::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("noclip_cam", noclip_cam_enabled);
}
