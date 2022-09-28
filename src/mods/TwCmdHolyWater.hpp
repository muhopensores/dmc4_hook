#pragma once

#include "../mod.hpp"

// Define ModName class that is a Mod
class TwCmdHolyWater : public Mod {
public:
    // using default constructor
    TwCmdHolyWater() = default;

    // override getModName() method with your mod name for logging.
    std::string get_mod_name() override { return "TwCmdHolyWater"; };

    // override onInitialize() method with routines required to initialize the
    // mod. return Mod::onInitialize() on success or error string on failure.
    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    // void onGUIframe() override;

private:
    utility::Timer* m_cooldown; // i've kept those as ptrs to avoid malloc/free calls
                              // all the time which would cause memory fragmenation
                              // not sure how modern c++ handles this shit idk
};
