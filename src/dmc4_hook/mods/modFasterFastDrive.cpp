#include "../mods.h"
#include "modFasterFastDrive.hpp"

bool FasterFastDrive::modEnabled{ false };

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
    if (ImGui::Checkbox("Faster Fast Drive", &modEnabled)) {
        toggle(modEnabled);
    }
}

void FasterFastDrive::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("faster_fast_drive").value_or(false);
    toggle(modEnabled);
};

void FasterFastDrive::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("faster_fast_drive", modEnabled);
};
