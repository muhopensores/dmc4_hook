#include "modFastPandora.hpp"

bool FastPandora::modEnabled{ false };

std::optional<std::string> FastPandora::onInitialize() {
    return Mod::onInitialize();
}

void FastPandora::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x03BCE7E, patch, "\x66\x83\xBF\x72\x17\x00\x00\x01", 8);
    }
    else {
        patch.revert();
    }
}

void FastPandora::onGUIframe() {
    if (ImGui::Checkbox("Fast Pandora", &modEnabled)) {
        toggle(modEnabled);
    }
    ImGui::SameLine();
    HelpMarker("Cycle to grounded Pandora moves in DT speed outside of DT");
}

void FastPandora::onConfigLoad(const utils::Config& cfg){
    modEnabled = cfg.get<bool>("fast_pandora").value_or(false);
    toggle(modEnabled);
}

void FastPandora::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("fast_pandora", modEnabled);
}
