#pragma once

#include "../mod.hpp"

// Define ModName class that is a Mod
class Quicksilver : public Mod {
public:
    // using default constructor
    Quicksilver() = default;

    static bool mod_enabled_nero;
    static bool mod_enabled_dante;

    // override getModName() method with your mod name for logging.
    std::string get_mod_name() override { return "Quicksilver"; };

    // override onInitialize() method with routines required to initialize the
    // mod. return Mod::onInitialize() on success or error string on failure.
    std::optional<std::string> on_initialize() override;

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;

    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    utility::Timer* m_timer; // i've kept those as ptrs to avoid malloc/free calls
                           // all the time which would cause memory fragmenation
                           // not sure how modern c++ handles this shit idk
    std::size_t m_command;
    std::size_t m_shorthand;
};
