#pragma once
#include "../mod.hpp"
struct uMissionMenu;
class ArcadeMode : public Mod {
public:
    ArcadeMode() = default;

    static bool mod_enabled;
    static bool launched_via_cmd;
    static bool user_modified_settings;
    static uMissionMenu g_our_mission_menu;

    // A start armed from elsewhere (netplay's lobby): the next Mission Menu starts it, then it's spent. It never touches
    // the settings above, so the cfg keeps the player's own.
    struct Launch {
        int difficulty = 3;   // the Mission Menu's difficulty row (Human 0 .. Hell And Hell 6)
        int mission = 1;
        int bp_floor = 0;     // > 0: Bloody Palace from this floor instead of the mission
        int character = 6;    // the menu's character (0-3 Dante, 4-7 Nero)
    };
    static bool arm_launch(const Launch& launch);   // false: the start checks couldn't be hooked
    // Netplay's lobby holds this machine's own start (-arcade's, or the mod's) while it is open: every machine starts
    // when the host's start arms it (arm_launch).
    static void hold_own_start(bool hold);
    std::string get_mod_name() override { return "ArcadeMode"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_msel_mstart, hook_bpstart;
};
