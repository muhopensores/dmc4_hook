#include "../mods.h"
#include "modFasterFastDrive.hpp"

bool FasterFastDrive::modEnabled{ false };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
bool FasterFastDrive::easyFastDriveEnabled = false;

std::optional<std::string> FasterFastDrive::onInitialize() {
    return Mod::onInitialize();
}

void FasterFastDrive::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3D0DFB, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch.revert();
    }
}

void FasterFastDrive::onGUIframe() {
    if (ImGui::Checkbox("Fast Quick Drive", &modEnabled)) {
        toggle(modEnabled);
    }
    ImGui::SameLine();
    HelpMarker("Quick Drive skips the initial slash");
    ImGui::SameLine(205);
    ImGui::Checkbox("Easy Quick Drive", &easyFastDriveEnabled);
    ImGui::SameLine();
    HelpMarker("Prop is cancellable into quick drive until frame 10");
}

void FasterFastDrive::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("faster_fast_drive").value_or(false);
    easyFastDriveEnabled = cfg.get<bool>("easy_fast_drive").value_or(false);
    toggle(modEnabled);
};

void FasterFastDrive::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("faster_fast_drive", modEnabled);
    cfg.set<bool>("easy_fast_drive", easyFastDriveEnabled);
};
