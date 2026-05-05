#pragma once

#include "../mod.hpp"
#include "csys/include/csys/string.h"
#include "SDK/uEnemy.hpp"

struct Room;

class AreaJump : public Mod {
public:
    AreaJump() = default;

    std::string get_mod_name() override { return "AreaJump"; };
    std::vector<std::string> get_search_terms() override { return {"area jump", "bloody palace", "room teleport"
};
    }
    struct Room {
      int id;
      const char* name;
    };

    static std::array<Room, 83> room_items;

    // BP:
    static bool randomize_bp_toggle;
    static bool bp_boss_rush_toggle;
    static uintptr_t bp_jump_hook1_continue;
    static uintptr_t bp_jump_hook2_continue;
    static uintptr_t bp_jump_hook3_continue;
    static uintptr_t randomized_bp_1_continue;
    static uintptr_t randomized_bp_2_continue;
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
    std::unique_ptr<FunctionHook> randomized_bp_1_continue_hook;
    std::unique_ptr<FunctionHook> randomized_bp_2_continue_hook;
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<Patch> patch2;
};

#if 0
std::array<AreaJump::Room, 101> AreaJump::bp_descriptions {
    Room {1, ("Arm Scarecrow x3")},
    Room {2, ("Leg Scarecrow x4")},
    Room {3, ("Leg Scarecrow x3, Arm Scarecrow x3")},
    Room {4, ("Frost x2")},
    Room {5, ("Bianco Angelo x2")},
    Room {6, ("Mephisto x3")},
    Room {7, ("Gladius x6")},
    Room {8, ("Chimera Seed x9")},
    Room {9, ("Chimera Seed x9, Scarecrow x3")},
    Room {10, ("Arm Scarecrow x6, Leg Scarecrow x6")},
    Room {11, ("Assault x2")},
    Room {12, ("Mega Scarecrow x2")},
    Room {13, ("Assault x2, Bianco Angelo x2")},
    Room {14, ("Faust x1")},
    Room {15, ("Bianco Angelo x2, Alto Angelo x1")},
    Room {16, ("Leg Scarecrow x3, Arm Scarecrow x3, Mega Scarecrow x1")},
    Room {17, ("Alto Angelo x2")},
    Room {18, ("Blitz x1")},
    Room {19, ("Basilisk x4")},
    Room {20, ("Berial")},
    Room {21, ("Mephisto x2, Frost x1")},
    Room {22, ("Frost x2, Faust x1")},
    Room {23, ("Assault x3, Frost x2")},
    Room {24, ("Assault x3, Basilisk x4")},
    Room {25, ("Assault x3, Blitz x1")},
    Room {26, ("Blitz x1, Basilisk x4")},
    Room {27, ("Frost x2, Bianco Angelo x2")},
    Room {28, ("Assault x2, Gladius x8")},
    Room {29, ("Blitz x1, Gladius x5")},
    Room {30, ("Frost x4, Assault x6, Blitz x1")},
    Room {31, ("Chimera Seed x6, Mephisto x3")},
    Room {32, ("Arm Scarecrow x3, Faust x1")},
    Room {33, ("Mephisto x2, Mega Scarecrow x1")},
    Room {34, ("Mephisto x6")},
    Room {35, ("Mephisto x3, Frost x1")},
    Room {36, ("Leg Scarecrow x2, Faust x1")},
    Room {37, ("Mephisto x2, Assault x3")},
    Room {38, ("Mephisto x2, Blitz x1")},
    Room {39, ("Chimera Seed x9, Faust x1")},
    Room {40, ("Bael")},
    Room {41, ("Chimera Seed x9")},
    Room {42, ("Chimera x5, Chimera Seed x5")},
    Room {43, ("Chimera x4")},
    Room {44, ("Chimera Seed x6, Gladius x6")},
    Room {45, ("Chimera x2, Blitz x1")},
    Room {46, ("Basilisk x2, Chimera Seed x4")},
    Room {47, ("Chimera Seed x5, Bianco Angelo x2, Alto Angelo x1")},
    Room {48, ("Leg Scarecrow x1, Chimera Seed x6")},
    Room {49, ("Chimera x3, Faust x1")},
    Room {50, ("Arm Scarecrow x3, Leg Scarecrow x3, Chimera Seed x12, Assault x4")},
    Room {51, ("Arm Scarecrow x6")},
    Room {52, ("Leg Scarecrow x3, Gladius x8")},
    Room {53, ("Mega Scarecrow x3, Frost x2")},
    Room {54, ("Arm Scarecrow x2, Leg Scarecrow x3 Alto Angelo x1")},
    Room {55, ("Mega Scarecrow x1, Faust x1")},
    Room {56, ("Mega Scarecrow x1, Alto Angelo x2")},
    Room {57, ("Arm Scarecrow x2, Leg Scarecrow x2, Mephisto x3")},
    Room {58, ("Mega Scarecrow x1, Basilisk x5")},
    Room {59, ("Chimera x2, Mega Scarecrow x1, Chimera Seed x4")},
    Room {60, ("Echidna")},
    Room {61, ("Bianco Angelo x4")},
    Room {62, ("Cutlass x2, Bianco Angelo x3")},
    Room {63, ("Blitz x1, Bianco Angelo x2")},
    Room {64, ("Bianco Angelo x1, Gladius x6")},
    Room {65, ("Bianco Angelo x7, Alto Angelo x1")},
    Room {66, ("Alto Angelo x1, Basilisk x4")},
    Room {67, ("Blitz x1, Alto Angelo x2")},
    Room {68, ("Arm Scarecrow x6, Bianco Angelo x2")},
    Room {69, ("Alto Angelo x1, Frost x2")},
    Room {70, ("Bianco Angelo x10, Alto Angelo x2")},
    Room {71, ("Gladius x16")},
    Room {72, ("Mega Scarecrow x1, Gladius x9")},
    Room {73, ("Gladius x8, Frost x1")},
    Room {74, ("Cutlass x2, Basilisk x4")},
    Room {75, ("Alto Angelo x1, Cutlass x3")},
    Room {76, ("Cutlass x4, Gladius x8")},
    Room {77, ("Basilisk x10")},
    Room {78, ("Frost x2, Basilisk x4")},
    Room {79, ("Cutlass x2, Basilisk x2, Gladius x4")},
    Room {80, ("Angelo Credo")},
    Room {81, ("Mega Scarecrow x1, Arm Scarecrow x3, Leg Scarecrow x2")},
    Room {82, ("Mephisto x4, Faust x1")},
    Room {83, ("Frost x3")},
    Room {84, ("Chimera x4, Chimera Seed x10")},
    Room {85, ("Blitz x2, Chimera Seed x3")},
    Room {86, ("Bianco Angelo x2, Alto Angelo x1")},
    Room {87, ("Basilisk x4, Gladius x7")},
    Room {88, ("Cutlass x3, Bianco Angelo x2")},
    Room {89, ("Mega Scarecrow x3, Blitz x1")},
    Room {90, ("Arm Scarecrow x15, Leg Scarecrow x15, Mega Scarecrow x3, Chimera Seed x3")}, // said 20 scarecrows, was 30
    Room {91, ("Faust x3")},
    Room {92, ("Alto Angelo x3, Bianco Angelo x4")},
    Room {93, ("Assault x8")},
    Room {94, ("Chimera x3, Chimera Seed x6, Bianco Angelo x2")},
    Room {95, ("Basilisk x16, Chimera Seed x4")},
    Room {96, ("Gladius x8, Cutlass x3, Bianco Angelo x3")},
    Room {97, ("Mephisto x6, Mega Scarecrow x2")},
    Room {98, ("Chimera x4, Chimera Seed x3")},
    Room {99, ("Alto Angelo x2, Bianco Angelo x44")},
    Room {100, ("Angelo Agnus")},
    Room {101, ("Dante")}
};
#endif

#if 0
struct EnemyIDAndName {
    int id;
    const char* name;
};
std::array<EnemyIDAndName, 29> enemy_names{
    EnemyIDAndName{0, __("Scarecrow Leg")},
    EnemyIDAndName{1, __("Scarecrow Arm")},
    EnemyIDAndName{3, __("Mega Scarecrow")},
    EnemyIDAndName{5, __("Bianco Angelo")},
    EnemyIDAndName{6, __("Alto Angelo")},
    EnemyIDAndName{8, __("Mephisto")},
    EnemyIDAndName{9, __("Faust")},
    EnemyIDAndName{10, __("Frost")},
    EnemyIDAndName{11, __("Assault")},
    EnemyIDAndName{12, __("Blitz")},
    EnemyIDAndName{13, __("Chimera Seed")},
    EnemyIDAndName{15, __("Cutlass")},
    EnemyIDAndName{16, __("Gladius")},
    EnemyIDAndName{17, __("Basilisk")},
    EnemyIDAndName{18, __("Berial")},
    EnemyIDAndName{19, __("Bael")},
    EnemyIDAndName{21, __("Echidna")},
    EnemyIDAndName{22, __("Angelo Credo")},
    EnemyIDAndName{23, __("Angelo Agnus")},
    EnemyIDAndName{25, __("Savior")},
    EnemyIDAndName{26, __("Savior2")},
    EnemyIDAndName{27, __("Credo human form")},
    EnemyIDAndName{29, __("Sanctus")},
    EnemyIDAndName{30, __("Sanctus2")},
    EnemyIDAndName{31, __("Credo human form")},
    EnemyIDAndName{33, __("Savior3")},
    EnemyIDAndName{35, __("Style checker")},
    EnemyIDAndName{36, __("Kyrie, pl022")},
    EnemyIDAndName{37, __("Dante")},
};
#endif