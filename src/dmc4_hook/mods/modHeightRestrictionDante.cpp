// include your mod header file
#include "modHeightRestrictionDante.hpp"

#if 1
// static variable defined in cpp file makes it local to current file
bool HeightRestrictionDante::modEnabled{ false };

// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> HeightRestrictionDante::onInitialize() {

	return Mod::onInitialize();
}

void HeightRestrictionDante::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x3B764E, patch, "\x0F\x84\x7B\x03\x00\x00", 6);
      
    }
    else
    {
        patch.revert();
   
    }
}

void HeightRestrictionDante::onGUIframe()
{
    if (ImGui::Checkbox("Dante", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void HeightRestrictionDante::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("dante_height_restriction_removed").value_or(false);
    toggle(modEnabled);
};

void HeightRestrictionDante::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("dante_height_restriction_removed", modEnabled);
};

#endif