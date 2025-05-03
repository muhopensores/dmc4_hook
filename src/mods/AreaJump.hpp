#pragma once

#include "../mod.hpp"

struct Room;

class AreaJump : public Mod {
public:
    AreaJump() = default;

    std::string get_mod_name() override { return "AreaJump"; };

    struct Room {
      int id;
      const char* name;
    };

    static constexpr std::array<Room, 83> room_items = {
        Room {503, "Berial"},                    // DevilMayCry4_DX9.exe+A56768
        Room {504, "Bael"},                      // DevilMayCry4_DX9.exe+A56528
        Room {505, "Echidna"},                   // DevilMayCry4_DX9.exe+A55FE8
        Room {506, "Agnus"},                     // DevilMayCry4_DX9.exe+A55F88
        Room {507, "Credo"},                     // DevilMayCry4_DX9.exe+A56728
        Room {700, "Dante"},                     // DevilMayCry4_DX9.exe+A56508
        Room {0,   "Opera House"},
        Room {1,   "Opera House Plaza"},
        Room {2,   "Storehouse"},
        Room {3,   "Cathedral"},
        Room {4,   "Terrace / Business District"},
        Room {5,   "Residential District"},
        Room {6,   "Port Caerula"},
        Room {7,   "Customs House"},
        Room {8,   "First Mining Area"},
        Room {9,   "Ferrum Hills"},
        Room {10,  "M17 Opera House"},
        Room {11,  "M17 Opera House Plaza"},
        Room {12,  "Business District / Terrace"},
        Room {13,  "M20 Opera House Plaza"},
        Room {100, "Second Mining Area"},
        Room {105, "Fortuna Castle Gate"},
        Room {200, "Grand Hall - Fortuna Castle"},
        Room {201, "Large Hall"},
        Room {202, "Dining Room"},
        Room {203, "Torture Chamber"},
        Room {204, "Central Courtyard"},
        Room {205, "Foris Falls - Bridge Area"},
        Room {206, "Gallery"},
        Room {207, "Library"},
        Room {209, "Soldier's Graveyard"},
        Room {210, "Master's Chamber"},
        Room {211, "Spiral Well"},
        Room {212, "Underground Laboratory"},
        Room {213, "R&D Access"},
        Room {214, "Game Room"},
        Room {215, "Containment Room"},
        Room {216, "Angel Creation"},
        Room {217, "Foris Falls - Detour Area"},
        Room {300, "Forest Entrance"},
        Room {301, "Windswept Valley"},
        Room {302, "Ruined Church"},
        Room {303, "Ruined Valley"},
        Room {304, "Ancient Training Ground"},
        Room {305, "Lapis River"},
        Room {306, "Ancient Plaza"},
        Room {307, "Den of the She-Viper"},
        Room {308, "Forgotten Ruins"},
        Room {309, "Hidden Pit"},
        Room {310, "Ruined Lowlands"},
        Room {311, "Lost Woods"},
        Room {400, "Gran Album Bridge"},
        Room {401, "Grand Hall - Order of the Sword HQ"},
        Room {402, "Key Chamber"},
        Room {403, "The Gauntlet"},
        Room {404, "Agnus' Room"},
        Room {405, "Security Corridor"},
        Room {406, "Experiment Disposal"},
        Room {407, "Meeting Room"},
        Room {408, "Ascension Chamber"},
        Room {409, "Advent Chamber"},
        Room {500, "Machina Ex Deus"},
        Room {501, "Stairway to Heaven"},
        Room {502, "Sacred Heart"},
        Room {510, "M18"},
        Room {512, "Sky Above Fortuna"},
        Room {800, "Secret Mission 1"},
        Room {801, "Secret Mission 2"},
        Room {802, "Secret Mission 3"},
        Room {803, "Secret Mission 4"},
        Room {804, "Secret Mission 5"},
        Room {805, "Secret Mission 6"},
        Room {806, "Secret Mission 7"},
        Room {807, "Secret Mission 8"},
        Room {808, "Secret Mission 9"},
        Room {809, "Secret Mission 10"},
        Room {810, "Secret Mission 11"},
        Room {811, "Secret Mission 12"},
        Room {705, "Bloody Palace 1-19"},
        Room {704, "Bloody Palace 21-39"},
        Room {703, "Bloody Palace 41-59"},
        Room {701, "Bloody Palace 61-79"},
        Room {702, "Bloody Palace 81-99"}
    };

    // BP:
    static bool randomize_bp_toggle;
    static bool bp_boss_rush_toggle;
    static uintptr_t bp_jump_hook1_continue;
    static uintptr_t bp_jump_hook2_continue;
    static uintptr_t bp_jump_hook3_continue;
    void toggle_randomized_bp(bool enable);
    static const Room* bp_stage(int floor);
    static void jump_to_stage(const Room* stage);
    static bool is_valid_room_id(int id);
    static const Room* find_room_by_id(int id);
    static const Room* find_room_by_name(const csys::String& name);
    std::optional<std::string> on_initialize() override;

    void on_gui_frame(int display) override;
    void on_update_input(utility::Input& input) override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:

    // BP:
    std::unique_ptr<FunctionHook> hook1;
    std::unique_ptr<FunctionHook> hook2;
    std::unique_ptr<FunctionHook> hook3;
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};
