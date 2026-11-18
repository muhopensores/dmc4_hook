#pragma once

#include "../../mod.hpp"

// Rooms: derived room classes (sdk/RoomClasses; plan Vibed\Rooms\README.md). Its hook makes the classes mods and plugins
// added at startup, in the sArea constructor right before the game records its area classes (0x8DB1A9), and its panel
// lists them. Test class (R2): aRoom900 derived from aRoom200 (stage 200's files and scripts), logging from its init /
// move / final; enter it with the Enter button or AreaJump's room id 900.
class Rooms : public Mod {
public:
    Rooms() = default;
    std::string get_mod_name() override { return "Rooms"; };
    std::vector<std::string> get_search_terms() override { return {"room", "rooms", "stage", "custom room", "area"}; }

    static bool test_room;   // add the test class aRoom900 (at the next start)

    std::optional<std::string> on_initialize() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<FunctionHook> hook_area;
};
