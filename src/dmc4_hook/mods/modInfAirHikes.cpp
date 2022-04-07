#include "modInfAirHikes.hpp"

bool InfAirHikes::modEnabled{ false };

std::optional<std::string> InfAirHikes::onInitialize() {
    return Mod::onInitialize();
}

void InfAirHikes::toggle(bool enable) {
    if (enable){
        install_patch_offset(0x03ACA3A, patch1, "\x88\x91\x7E\x1E\x00\x00", 6);     // set air hike count 0 rather than wall hike 0 every air hike
        install_patch_offset(0x03ACA51, patch2, "\x90\x90\x90\x90\x90\x90\x90", 7); // wall hike add to jump count
        install_patch_offset(0x03ACA4A, patch3, "\x90\x90\x90\x90\x90\x90\x90", 7); // kills mov 0 to can wall hike
    }
    else {
        patch1.revert();
        patch2.revert();
        patch3.revert();
    }
}

void InfAirHikes::onGUIframe() {
    if (ImGui::Checkbox("Infinite Air Hikes", &modEnabled)) {
        toggle(modEnabled);
    }
}

void InfAirHikes::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("inf_air_hikes").value_or(false);
    toggle(modEnabled);
};

void InfAirHikes::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("inf_air_hikes", modEnabled);
};
