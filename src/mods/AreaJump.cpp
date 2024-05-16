#include "AreaJump.hpp"
#include <array>
#include <algorithm>
#include "RoomRespawn.hpp"

uintptr_t  AreaJump::jmp_return{ NULL };
cAreaJump* AreaJump::c_area_jump_ptr{ NULL };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

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
    IM_ASSERT(index > 0);
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

naked void detour() {
	// steam   DevilMayCry4_DX9.exe+E1F6   - 8B 92 30380000        - mov edx, [edx+00003830]
	// nosteam DevilMayCry4_DX9.exe+546E76 - 8B 92 30380000        - mov edx, [edx+00003830]
	__asm {
		mov edx, [edx+0x3830]
		mov DWORD PTR [AreaJump::c_area_jump_ptr], edx
		jmp DWORD PTR [AreaJump::jmp_return]
	}
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
    c_area_jump_ptr->room_id = stage->id;
	c_area_jump_ptr->init_jump = 1;
}

std::optional<std::string> AreaJump::on_initialize() {
	// uintptr_t address = hl::FindPattern("8B 92 30 38 00 00", "DevilMayCry4_DX9.exe"); // DevilMayCry4_DX9.exe+E1F6 
    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_OEM_4 }, "Restart BP stage", "bp_restart_stage_hotkey"));

    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_OEM_6 }, "Next BP stage", "bp_next_stage_hotkey"));

    if (!install_hook_offset(0x00E1F6, hook, &detour, &AreaJump::jmp_return, 6)){
            spdlog::error("Failed to init AreaJump mod\n");
            return "Failed to init AreaJump mod";
    }

    console->system().RegisterCommand("skip", "Skip current BP stage", [this]() {
        if (!IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr,sizeof(uint32_t))) {
            static SMediator* s_mediator_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
            if (s_mediator_ptr->missionID == 50){ // always shows 50 for BP
                jump_to_stage(bp_stage(++(c_area_jump_ptr->bp_floor_stage)));
            }
	    }
    });

    console->system().RegisterCommand("bp", "Jump to BP stage",
        [this](int value) {
        static SMediator* s_mediator_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
        if (!IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr, sizeof(uint32_t))) {
            if (s_mediator_ptr->missionID == 50) { // always shows 50 for BP
                if (value <= 101 && value >= 1){
                    jump_to_stage(bp_stage(c_area_jump_ptr->bp_floor_stage = value));
                }
                else {
                spdlog::error("Invalid Stage ID");
                }
            }
        }
        }, 
        csys::Arg<int>("0-101"));

    // damn cant overload commands distingueshed by arguments alone 
    console->system().RegisterCommand("roomi", "Jump to room ID",
        [this](int value) {
        static SMediator* s_mediator_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
        if (!IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr, sizeof(uint32_t))) {
            if (is_valid_room_id(value)) {
                c_area_jump_ptr->room_id = value;
                c_area_jump_ptr->init_jump = 1;
            }
            else {
                spdlog::error("Invalid Room ID");
            }
        }
        }, 
        csys::Arg<int>("0-811"));

    console->system().RegisterCommand("rooma", "Jump to room name", 
        [this](csys::String value) {
        static SMediator* s_mediator_ptr = (SMediator*)*(uintptr_t*)static_mediator_ptr;
        if (!IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr, sizeof(uint32_t))) {
            if (const Room* proom = find_room_by_name(value)) {
                c_area_jump_ptr->room_id = proom->id;
                c_area_jump_ptr->init_jump = 1;
            }
            else {
                spdlog::error("Invalid Room Name");
            }
        }
        }, csys::Arg<csys::String>("ASCI room name, case insensitive"));

	return Mod::on_initialize();
}

void AreaJump::on_gui_frame() 
{
	if (IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr,sizeof(uint32_t))) {
		ImGui::TextWrapped(_("Area Jump is not initialized.\nLoad into a stage to access it."));
		return;
	}

    ImGui::Text(_("Bloody Palace Floor Teleports"));
    ImGui::SameLine();
    help_marker(_("Type in which BP floor you want to teleport to then hit Go to be teleported to that stage"));
    ImGui::Spacing();

	if (c_area_jump_ptr->bp_floor_stage) {
		if (ImGui::InputInt(_("##BP Floor "), &c_area_jump_ptr->bp_floor_stage, 1, 10, ImGuiInputTextFlags_AllowTabInput)) {
			c_area_jump_ptr->bp_floor_stage = std::clamp(c_area_jump_ptr->bp_floor_stage, 1, 101);
		}

        if (ImGui::Button(_("Go"), ImVec2(290, 20))) {
			jump_to_stage(bp_stage(c_area_jump_ptr->bp_floor_stage));
		}
	}
    else {
        ImGui::TextWrapped(_("BP Floor Jump is not initialized.\nLoad into BP to access it."));
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text(_("Mission Area Teleports"));
    ImGui::SameLine();
    help_marker(_("Teleport to any area by clicking its name"));

    ImGui::Spacing();

	int item_current_idx = 0;
    if (ImGui::BeginListBox("##Room Codes Listbox")) {
        for (size_t n = 0; n < room_items.size(); n++) {
            const bool is_selected = (item_current_idx == n);

            // sigh
            char buffer[MAX_PATH];
            int result = snprintf(buffer, sizeof(buffer), "%s - %d", room_items[n].name, room_items[n].id);
            IM_ASSERT(result > 0); // encoding error
            IM_ASSERT(result < MAX_PATH); // output was truncated or null terminator didnt fit in

            if (ImGui::Selectable(buffer, is_selected)) {
                item_current_idx = n;
                jump_to_stage(&room_items[n]);
            }

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected) {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndListBox();
    }
}

void AreaJump::on_update_input(utility::Input & input) {
    if (m_hotkeys[0]->check(input)) {
        if (IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr, sizeof(uint32_t))) {
            return;
        }
        RoomRespawn::g_reset_manager = true;
        jump_to_stage(bp_stage(c_area_jump_ptr->bp_floor_stage));
    }

    if (m_hotkeys[1]->check(input)) {
        if (IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr,sizeof(uint32_t))) {
            return;
        }
        jump_to_stage(bp_stage(++(c_area_jump_ptr->bp_floor_stage)));
    }
}
