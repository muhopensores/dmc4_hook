#include "AreaJump.hpp"
#include <array>
#include <algorithm>
#include "RoomRespawn.hpp"
#include <Config.hpp>

static int savedBPFloor = 0;
static float savedBPTimer = 0.0f;
static int savedOrbs = 0;
static float savedHP = 0.0f;
static float savedDT = 0.0f;

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

static bool is_valid_room_id(int id) {
    return std::any_of(room_items.begin(), room_items.end(), [id](const Room& room) { 
        return room.id == id; 
    });
};

static const Room* find_room_by_id(int id) {
    auto it = std::find_if(room_items.begin(), room_items.end(), [id](const Room& room) { return room.id == id; });
    IM_ASSERT(it != room_items.end()); // crash if we passed wrong index
    size_t index = std::distance(room_items.begin(), it);
    //IM_ASSERT(index > 0); // this made tp to berial crash 
    return &room_items[index];
};

// cba to add hash lookups at this moment, sorry che
static const Room* find_room_by_name(const csys::String& name) {
    auto it = std::find_if(room_items.begin(), room_items.end(), [name](const Room& room) {
        if(_stricmp(room.name, name.m_String.c_str()) == 0) {
            return true;
        }
        return false;
    });
    if (it != room_items.end()) {
        size_t index = std::distance(room_items.begin(), it);
        return &room_items[index];
    }
    return nullptr;
}

static const Room* bp_stage(int floor) {
	auto in_range = [](int value, int low, int high) {return (value >= low) && (value <= high); };
	
	if (in_range(floor, 1, 19))  { return find_room_by_id(705); }
	if (in_range(floor, 21, 39)) { return find_room_by_id(704); }
	if (in_range(floor, 41, 59)) { return find_room_by_id(703); }
	if (in_range(floor, 61, 79)) { return find_room_by_id(701); }
	if (in_range(floor, 81, 99)) { return find_room_by_id(702); }
	switch (floor) {
	case 20:
		return find_room_by_id(503);
	case 40:
		return find_room_by_id(504);
	case 60:
		return find_room_by_id(505);
	case 80:
		return find_room_by_id(507);
	case 100:
		return find_room_by_id(506);
	case 101:
		return find_room_by_id(700);
	default:
		break;
	}
	return nullptr;
}

void AreaJump::jump_to_stage(const Room* stage) {
    sArea* s_area_ptr = devil4_sdk::get_sArea();
    s_area_ptr->aGamePtr->room_id = stage->id;
	s_area_ptr->aGamePtr->init_jump = 1;
}


// RANDOM BP START/////////////////////////////////////////////////////////////////////////////////////////////////////
#include "random"
#include "numeric"

bool AreaJump::randomize_bp_toggle{ false };
bool AreaJump::bp_boss_rush_toggle{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
uintptr_t AreaJump::bp_jump_hook1_continue{ NULL };
uintptr_t AreaJump::bp_jump_hook2_continue{ NULL };
uintptr_t AreaJump::bp_jump_hook3_continue{ NULL };

static int bp_floor = 0;
static int bp_area_id = 0;
static int number_of_complete_floors = 0;
static std::array<uint32_t, 100> bp_array;
static char* stage_start_id;

static int boss_floor = 0;
static int boss_area_id = 0;
static int number_of_complete_bosses = 0;
static std::array<uint32_t, 5> boss_array;
static char* boss_start_id;

// DevilMayCry4_DX9.exe+4AB8E1 - push []
char boss_start_i_ds[11][5] {
    "\x68\x65\xE5\x00", //                             //  0 // Berial  // 503
    "\x28\x65\xE5\x00", //                             //  1 // Bael    // 504
    "\xE8\x5F\xE5\x00", //                             //  2 // Echidna // 505
    "\x88\x5F\xE5\x00", //                             //  3 // Credo   // 507
    "\x28\x67\xE5\x00", //                             //  4 // Agnus   // 506
    "\x08\x65\xE5\x00", //                             //  5 // Dante   // 700
    "\x68\x67\xE5\x00", // DevilMayCry4_DX9.exe+A56768 //  6 // 1-19    // 705
    "\x88\x63\xE5\x00", // DevilMayCry4_DX9.exe+A56388 //  7 // 21-39   // 704
    "\xA8\x6A\xE5\x00", // DevilMayCry4_DX9.exe+A56AA8 //  8 // 41-59   // 703
    "\x68\x5F\xE5\x00", // DevilMayCry4_DX9.exe+A55F68 //  9 // 61-79   // 701
    "\x48\x64\xE5\x00"  // DevilMayCry4_DX9.exe+A56448 // 10 // 81-99   // 702
};

int convert_floor_to_switch(int floor) {
	auto in_range = [](int value, int low, int high) {return (value >= low) && (value <= high); };
	
	if (in_range(floor,  1, 19)) { return  6; }
	if (in_range(floor, 21, 39)) { return  7; }
	if (in_range(floor, 41, 59)) { return  8; }
	if (in_range(floor, 61, 79)) { return  9; }
	if (in_range(floor, 81, 99)) { return 10; }
	switch (floor) {
	case 20:
		return 0;
	case 40:
		return 1;
	case 60:
		return 2;
	case 80:
		return 3;
	case 100:
		return 4;
	case 101:
		return 5;
	default:
		break;
	}
	return -1;
}

int convert_switch_to_area_id(int stage) {
    switch (stage) {
    case 0:
        return 503; // "Bloody Palace 20"
    case 1:
        return 504; // "Bloody Palace 40"
    case 2:
        return 505; // "Bloody Palace 60"
    case 3:
        return 507; // "Bloody Palace 80"
    case 4:
        return 506; // "Bloody Palace 100"
    case 5:
        return 700; // "Bloody Palace 101"
    case 6:
        return 705; // "Bloody Palace 1-19"
    case 7:
        return 704; // "Bloody Palace 21-39"
    case 8:
        return 703; // "Bloody Palace 41-59"
    case 9:
        return 701; // "Bloody Palace 61-79"
    case 10:
        return 702; // "Bloody Palace 81-99"
    }
    return -1;
}

void randomize_bp_floors() {
    number_of_complete_floors = 0;
    std::iota(bp_array.begin(), bp_array.end(), 1); // fill the list starting from 1
    std::random_device rd;                        // random device for shuffle
    std::mt19937 g(rd());
    std::shuffle(bp_array.begin(), bp_array.end(), g);
}

void randomize_bp_bosses() {
    number_of_complete_bosses = 0;
    boss_array[0] = 20;
    boss_array[1] = 40;
    boss_array[2] = 60;
    boss_array[3] = 80;
    boss_array[4] = 100;
    std::random_device rd; // random device for shuffle
    std::mt19937 g(rd());
    std::shuffle(boss_array.begin(), boss_array.end(), g);
}

void random_boss_init(void) {
    // randomize bp bosses
    randomize_bp_bosses();

    // apply floor
    boss_floor = boss_array[number_of_complete_bosses];

    // set area jump
    boss_area_id = convert_switch_to_area_id(convert_floor_to_switch(boss_floor));

    // set stage load
    boss_start_id = *(char**)boss_start_i_ds[convert_floor_to_switch(boss_floor)];
}

void boss_continue() { // used every time you enter a teleporter during randomized boss rush
    // inc number of complete floors
    number_of_complete_bosses++;

    if (number_of_complete_bosses == 5) {
        boss_floor = 101;
        boss_area_id = 700;
        return;
    }

    // apply floor
    boss_floor = boss_array[number_of_complete_bosses];

    // set area jump
    boss_area_id = convert_switch_to_area_id(convert_floor_to_switch(boss_floor));
}

void random_bp_init() { // used in first bp detour
    // randomize bp floors
    randomize_bp_floors();

    // apply floor
    bp_floor = bp_array[number_of_complete_floors];

    // set area jump
    bp_area_id = convert_switch_to_area_id(convert_floor_to_switch(bp_floor));

    // set stage load
    stage_start_id = *(char**)boss_start_i_ds[convert_floor_to_switch(bp_floor)];
}

void bp_continue() { // used in other bp detours
    // inc number of complete floors
    number_of_complete_floors++;

    if (number_of_complete_floors == 100) {
        bp_floor = 101;
        bp_area_id = 700;
        return;
    }

    // apply floor
    bp_floor = bp_array[number_of_complete_floors];

    // set area jump
    bp_area_id = convert_switch_to_area_id(convert_floor_to_switch(bp_floor));
}

naked void bp_jump_hook1_proc(void) { // Initial load of BP, picks stage to load
    _asm {
    // check both cheats are off before skipping randomizing
        cmp byte ptr [AreaJump::randomize_bp_toggle], 1
        je InitRandomStages
        cmp byte ptr [AreaJump::bp_boss_rush_toggle], 0
        je code // both cheats are off, skip randomizing
    // create randomized numbers
    InitRandomStages:
        push eax
        push ecx
        push edx
        call random_bp_init
        call random_boss_init
        pop edx
        pop ecx
        pop eax
        cmp byte ptr [AreaJump::bp_boss_rush_toggle], 1
        je bossrush
        push [stage_start_id]
        jmp retcode

    bossrush:
        cmp byte ptr [AreaJump::randomize_bp_toggle], 1
        je randombossrushstart
    // if boss rush isn't randomized
        push [0x00E56568] // berial
        jmp retcode

    // push appropriate stage for whichever area we got
    randombossrushstart:
        push [boss_start_id]
        jmp retcode

    code:
        push [0x00E56768]
    retcode:
		jmp dword ptr [AreaJump::bp_jump_hook1_continue]
    }
}

naked void bp_jump_hook2_proc(void) { // Initial load of BP
    _asm {
        cmp byte ptr [AreaJump::bp_boss_rush_toggle], 1
        je bossrush

        cmp byte ptr [AreaJump::randomize_bp_toggle], 0
        je code

    // write randomized floor and area ID
        push edx
        mov edx, [bp_floor]
        mov [eax+0x74], edx
        mov edx, [bp_area_id]
        mov [eax+0x6C], edx
        pop edx
        jmp retcode

    bossrush:
        cmp byte ptr [AreaJump::randomize_bp_toggle], 1 // should randomize?
        je randombossrushstart
        mov dword ptr [eax+0x74], 20 // room id
        mov dword ptr [eax+0x6C], 503 // area id
        jmp retcode

    // write randomized boss stage and area id
    randombossrushstart:
        push edx
        mov edx, [boss_floor]
        mov [eax+0x74], edx
        mov edx, [boss_area_id]
        mov [eax+0x6C], edx
        pop edx
        jmp retcode

    code:
        mov dword ptr [eax+0x74], 00000001
    retcode:
		jmp dword ptr [AreaJump::bp_jump_hook2_continue]
    }
}

naked void bp_jump_hook3_proc(void) { // called every time you enter a teleporter
    _asm {
        push eax
        mov eax, [static_mediator_ptr]
        mov eax, [eax]
        cmp dword ptr [eax+0xE0], 50
        pop eax
        jne code
        cmp byte ptr [AreaJump::bp_boss_rush_toggle], 1
        je bossrush

        cmp byte ptr [AreaJump::randomize_bp_toggle], 0
        je code

    // update floor and area ID values
        push ecx
        push eax
        push edx
        call bp_continue
        pop edx
        pop eax
        pop ecx

    // write bp floor
        push edx
        mov edx, [bp_floor]
        mov [ecx+0x74], edx
        mov edx, [bp_area_id]
        mov [ecx+0x6C], edx
        pop edx
        jmp retcode

    bossrush:
        cmp byte ptr [AreaJump::randomize_bp_toggle], 1 // should randomize?
        je randombossrushcontinue
        cmp dword ptr [ecx+0x74], 21
        je stage40
        cmp dword ptr [ecx+0x74], 41
        je stage60
        cmp dword ptr [ecx+0x74], 61
        je stage80
        cmp dword ptr [ecx+0x74], 81
        je stage100
        jmp code

    stage40:
        mov dword ptr [ecx+0x74], 40
        mov dword ptr [ecx+0x6C], 504
        jmp retcode
    stage60:
        mov dword ptr [ecx+0x74], 60
        mov dword ptr [ecx+0x6C], 505
        jmp retcode
    stage80:
        mov dword ptr [ecx+0x74], 80
        mov dword ptr [ecx+0x6C], 507
        jmp retcode
    stage100:
        mov dword ptr [ecx+0x74], 100
        mov dword ptr [ecx+0x6C], 506
        jmp retcode

    // update floor and area ID values
    randombossrushcontinue:
        push ecx
        push eax
        push edx
        call boss_continue
        pop edx
        pop eax
        pop ecx

    // write boss floor
        push edx
        mov edx, [boss_floor]
        mov [ecx+0x74], edx
        mov edx, [boss_area_id]
        mov [ecx+0x6C], edx
        pop edx
        jmp retcode

    code:
        mov dword ptr [ecx+0x6C], edx
    retcode:
        mov edx, [ebx+04h]
		jmp dword ptr [AreaJump::bp_jump_hook3_continue]
    }
}

void AreaJump::toggle_randomized_bp(bool enable) { // randomized bp
    if (enable) {
        install_patch_offset(0x04B808, patch1, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x04C200, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

std::optional<std::string> AreaJump::on_initialize() {
    sArea* s_area_ptr = devil4_sdk::get_sArea();
	// uintptr_t address = hl::FindPattern("8B 92 30 38 00 00", "DevilMayCry4_DX9.exe"); // DevilMayCry4_DX9.exe+E1F6 
    utility::create_keyboard_hotkey(m_hotkeys, { VK_CONTROL, VK_OEM_4 }, "Restart BP stage", "bp_restart_stage_hotkey");
    utility::create_keyboard_hotkey(m_hotkeys, { VK_CONTROL, VK_OEM_6 }, "Next BP stage", "bp_next_stage_hotkey");

    console->system().RegisterCommand("skip", "Skip current BP stage", [this]() {
        if (devil4_sdk::get_local_player()) {
            if (devil4_sdk::get_sMediator()->missionID == 50){ // always shows 50 for BP
                jump_to_stage(bp_stage(++(devil4_sdk::get_sArea()->aGamePtr->bp_floor)));
            }
	    }
    });

    console->system().RegisterCommand("bp", "Jump to BP stage", [this](int value) {
        if (devil4_sdk::get_local_player()) {
            if (devil4_sdk::get_sMediator()->missionID == 50) { // always shows 50 for BP
                if (value <= 101 && value >= 1){
                    jump_to_stage(bp_stage(devil4_sdk::get_sArea()->aGamePtr->bp_floor = value));
                }
                else {
                    spdlog::error("Invalid Stage ID");
                }
            }
        }
    }, csys::Arg<int>("0-101"));

    // damn cant overload commands distingueshed by arguments alone 
    console->system().RegisterCommand("roomi", "Jump to room ID", [](int value) {
        if (devil4_sdk::get_local_player()) {
            if (is_valid_room_id(value)) {
                devil4_sdk::get_sArea()->aGamePtr->room_id = value;
                devil4_sdk::get_sArea()->aGamePtr->init_jump = 1;
            }
            else {
                spdlog::error("Invalid Room ID");
            }
        }
    }, csys::Arg<int>("0-811"));

    console->system().RegisterCommand("rooma", "Jump to room name", [](csys::String value) {
        if (devil4_sdk::get_local_player()) {
            if (const Room* proom = find_room_by_name(value)) {
                devil4_sdk::get_sArea()->aGamePtr->room_id = proom->id;
                devil4_sdk::get_sArea()->aGamePtr->init_jump = 1;
            }
            else {
                spdlog::error("Invalid Room Name");
            }
        }
    }, csys::Arg<csys::String>("ASCI room name, case insensitive"));

    // BP:
    if (!install_hook_offset(0x4AB8E1, hook1, &bp_jump_hook1_proc, &AreaJump::bp_jump_hook1_continue, 5)) {
        spdlog::error("Failed to init bpJumpHook1 mod\n");
        return "Failed to init bpJumpHook1 mod";
    }
    if (!install_hook_offset(0x4AB8FA, hook2, &bp_jump_hook2_proc, &AreaJump::bp_jump_hook2_continue, 7)) {
        spdlog::error("Failed to init bpJumpHook2 mod\n");
        return "Failed to init bpJumpHook2 mod";
    }
    if (!install_hook_offset(0x04A974, hook3, &bp_jump_hook3_proc, &AreaJump::bp_jump_hook3_continue, 6)) {
        spdlog::error("Failed to init bpJumpHook3 mod\n");
        return "Failed to init bpJumpHook3 mod";
    }

	return Mod::on_initialize();
}

void AreaJump::on_gui_frame(int display) {
    sArea* s_area_ptr = devil4_sdk::get_sArea();
    SMediator* s_med_ptr = devil4_sdk::get_sMediator();
    uPlayer* player = devil4_sdk::get_local_player();

    ImGui::SeparatorText(_("Bloody Palace"));

    ImGui::PushItemWidth(sameLineItemWidth);
	if (ImGui::InputInt(_("##BP Floor "), &s_area_ptr->aGamePtr->bp_floor, 1, 10, ImGuiInputTextFlags_AllowTabInput)) {
        if (player)
			s_area_ptr->aGamePtr->bp_floor = std::clamp(s_area_ptr->aGamePtr->bp_floor, 1, 101);
	}
    ImGui::SameLine();
    if (ImGui::Button(_("Save BP Progress"), ImVec2(sameLineItemWidth, NULL))) {
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        SMediator* s_med_ptr = devil4_sdk::get_sMediator();
        if (player) {
            savedBPFloor = s_area_ptr->aGamePtr->bp_floor;
            savedBPTimer = s_med_ptr->bpTimer;
            savedOrbs = s_med_ptr->orbMissionCurrent;
            savedHP = s_med_ptr->player_ptr->HP;
            savedDT = s_med_ptr->player_ptr->DT;
            ModFramework* framework = g_framework.get();
            utility::Config cfg{};
            cfg.load(CONFIG_FILENAME);
            cfg.set<int>("saved_bp_floor", savedBPFloor);
            cfg.set<float>("saved_bp_timer", savedBPTimer);
            cfg.set<int>("saved_orbs", savedOrbs);
            cfg.set<float>("saved_hp", savedHP);
            cfg.set<float>("saved_dt", savedDT);
            cfg.save(CONFIG_FILENAME);
        }
    }        
    ImGui::SameLine();
    help_marker(_("Saves floor, timer, orbs, hp, dt"));

    if (ImGui::Button(_("Teleport"), ImVec2(sameLineItemWidth, NULL))) {
        if (player)
			jump_to_stage(bp_stage(s_area_ptr->aGamePtr->bp_floor));
	}
    ImGui::SameLine();
    if (ImGui::Button(_("Load BP Progress"), ImVec2(sameLineItemWidth, NULL))) {
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        if (player) {
            s_area_ptr->aGamePtr->bp_floor = savedBPFloor;
            jump_to_stage(bp_stage(s_area_ptr->aGamePtr->bp_floor));
            s_med_ptr->bpTimer = savedBPTimer;
            s_med_ptr->orbMissionCurrent = savedOrbs;
            s_med_ptr->player_ptr->HP = savedHP;
            s_med_ptr->player_ptr->DT = savedDT;
            s_area_ptr->aGamePtr->init_jump = 1;
        }
    }
    ImGui::SameLine();
    help_marker(_("Press Load after loading into BP\nLoads saved floor, timer, orbs, hp, dt"));

    ImGui::PopItemWidth();

    if (ImGui::Checkbox(_("Randomize BP"), &randomize_bp_toggle)) {
        toggle_randomized_bp(randomize_bp_toggle);
        randomize_bp_floors();
        randomize_bp_bosses();
    }
    ImGui::SameLine();
    help_marker(_("All stages will be randomized"));
    ImGui::Checkbox(_("Boss Rush"), &bp_boss_rush_toggle);
    ImGui::SameLine();
    help_marker(_("Only boss stages will be played\nEnable before selecting BP"));
    if (randomize_bp_toggle) {
        ImGui::Indent(lineIndent);
        if (ImGui::CollapsingHeader(_("View Randomized Stages"))) {
                ImGui::Text(_("Stage Number"));
                ImGui::SameLine(sameLineWidth);
                ImGui::Text(_("Random Stage"));
            if (!bp_boss_rush_toggle) {
                int stage_count = 1;
                for (int i : bp_array) {
                    ImGui::Text(_("%i"), stage_count);
                    stage_count++;
                    ImGui::SameLine(sameLineWidth);
                    ImGui::Text(_("%i"), i);
                }
            }
            else {
                int stage_count = 1;
                for (int i : boss_array) {
                    ImGui::Text(_("%i"), stage_count);
                    stage_count++;
                    ImGui::SameLine(sameLineWidth);
                    ImGui::Text(_("%i"), i);
                }
            }
            ImGui::Separator();
        }
        ImGui::Unindent(lineIndent);
    }

    ImGui::SeparatorText(_("Mission Area Teleports"));

    ImGui::Spacing();
    ImGui::PushItemWidth(sameLineWidth);
	int item_current_idx = 0;
    float itemHeight = ImGui::GetTextLineHeightWithSpacing();
    int visibleItems = 25;
    ImVec2 listBoxSize = ImVec2(ImGui::GetContentRegionAvail().x, itemHeight * visibleItems);
    if (ImGui::BeginListBox("##Room Codes Listbox", listBoxSize)) {
        for (size_t n = 0; n < room_items.size(); n++) {
            const bool is_selected = (item_current_idx == n);
            // sigh
            char buffer[MAX_PATH];
            int result = snprintf(buffer, sizeof(buffer), "%s - %d", room_items[n].name, room_items[n].id);
            IM_ASSERT(result > 0); // encoding error
            IM_ASSERT(result < MAX_PATH); // output was truncated or null terminator didnt fit in

            if (ImGui::Selectable(buffer, is_selected)) {
                item_current_idx = n;
                if (player)
                    jump_to_stage(&room_items[n]);
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopItemWidth();
}

void AreaJump::on_update_input(utility::Input & input) {
    if (m_hotkeys[0]->check(input)) {
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        if (!devil4_sdk::get_local_player()) {
            return;
        }
        RoomRespawn::g_reset_manager = true;
        jump_to_stage(bp_stage(s_area_ptr->aGamePtr->bp_floor));
    }

    if (m_hotkeys[1]->check(input)) {
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        if (!devil4_sdk::get_local_player()) {
            return;
        }
        jump_to_stage(bp_stage(++(s_area_ptr->aGamePtr->bp_floor)));
    }
}

void AreaJump::on_config_save(utility::Config& cfg) {
    cfg.set<int>("saved_bp_floor", savedBPFloor);
    cfg.set<float>("saved_bp_timer", savedBPTimer);
    cfg.set<int>("saved_orbs", savedOrbs);
    cfg.set<float>("saved_hp", savedHP);
    cfg.set<float>("saved_dt", savedDT);

    // BP:
    cfg.set<bool>("randomize_bp_toggle", randomize_bp_toggle);
    cfg.set<bool>("bp_boss_rush_toggle", bp_boss_rush_toggle);
}

void AreaJump::on_config_load(const utility::Config& cfg) {
    savedBPFloor = cfg.get<int>("saved_bp_floor").value_or(0);
    savedBPTimer = cfg.get<float>("saved_bp_timer").value_or(0.0f);
    savedOrbs = cfg.get<int>("saved_orbs").value_or(0);
    savedHP = cfg.get<float>("saved_hp").value_or(0.0f);
    savedDT = cfg.get<float>("saved_dt").value_or(0.0f);

    // BP:
    randomize_bp_toggle = cfg.get<bool>("randomize_bp_toggle").value_or(false);
    if (randomize_bp_toggle) {
        randomize_bp_floors();
        randomize_bp_bosses();
        toggle_randomized_bp(randomize_bp_toggle);
    }
    bp_boss_rush_toggle = cfg.get<bool>("bp_boss_rush_toggle").value_or(false);
}
