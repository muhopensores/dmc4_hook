#include "AreaJump.hpp"
#include <array>
#include <algorithm>
#include "RoomRespawn.hpp"

uintptr_t  AreaJump::jmp_return{ NULL };
cAreaJump* AreaJump::c_area_jump_ptr{ NULL };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

struct Room {
  const char* name;
  int id;
};

static constexpr std::array room_items = {
    Room {"Berial", 503},                    // DevilMayCry4_DX9.exe+A56768
    Room {"Bael", 504},                      // DevilMayCry4_DX9.exe+A56528
    Room {"Echidna", 505},                   // DevilMayCry4_DX9.exe+A55FE8
    Room {"Agnus", 506},                     // DevilMayCry4_DX9.exe+A55F88
    Room {"Credo", 507},                     // DevilMayCry4_DX9.exe+A56728
    Room {"Dante", 700},                     // DevilMayCry4_DX9.exe+A56508
    Room {"Opera House", 0},
    Room {"Opera House Plaza", 1},
    Room {"Storehouse", 2},
    Room {"Cathedral", 3},
    Room {"Terrace / Business District", 4},
    Room {"Residential District", 5},
    Room {"Port Caerula", 6},
    Room {"Customs House", 7},
    Room {"First Mining Area", 8},
    Room {"Ferrum Hills", 9},
    Room {"M17 Opera House", 10},
    Room {"M17 Opera House Plaza", 11},
    Room {"Business District / Terrace", 12},
    Room {"M20 Opera House Plaza", 13},
    Room {"Second Mining Area", 100},
    Room {"Fortuna Castle Gate", 105},
    Room {"Grand Hall - Fortuna Castle", 200},
    Room {"Large Hall", 201},
    Room {"Dining Room", 202},
    Room {"Torture Chamber", 203},
    Room {"Central Courtyard", 204},
    Room {"Foris Falls - Bridge Area", 205},
    Room {"Gallery", 206},
    Room {"Library", 207},
    Room {"Soldier's Graveyard", 209},
    Room {"Master's Chamber", 210},
    Room {"Spiral Well", 211},
    Room {"Underground Laboratory", 212},
    Room {"R&D Access", 213},
    Room {"Game Room", 214},
    Room {"Containment Room", 215},
    Room {"Angel Creation", 216},
    Room {"Foris Falls - Detour Area", 217},
    Room {"Forest Entrance", 300},
    Room {"Windswept Valley", 301},
    Room {"Ruined Church", 302},
    Room {"Ruined Valley", 303},
    Room {"Ancient Training Ground", 304},
    Room {"Lapis River", 305},
    Room {"Ancient Plaza", 306},
    Room {"Den of the She-Viper", 307},
    Room {"Forgotten Ruins", 308},
    Room {"Hidden Pit", 309},
    Room {"Ruined Lowlands", 310},
    Room {"Lost Woods", 311},
    Room {"Gran Album Bridge", 400},
    Room {"Grand Hall - Order of the Sword HQ", 401},
    Room {"Key Chamber", 402},
    Room {"The Gauntlet", 403},
    Room {"Agnus' Room", 404},
    Room {"Security Corridor", 405},
    Room {"Experiment Disposal", 406},
    Room {"Meeting Room", 407},
    Room {"Ascension Chamber", 408},
    Room {"Advent Chamber", 409},
    Room {"Machina Ex Deus", 500},
    Room {"Stairway to Heaven", 501},
    Room {"Sacred Heart", 502},
    Room {"M18", 510},
    Room {"Sky Above Fortuna", 512},
    Room {"Secret Mission 1", 800},
    Room {"Secret Mission 2", 801},
    Room {"Secret Mission 3", 802},
    Room {"Secret Mission 4", 803},
    Room {"Secret Mission 5", 804},
    Room {"Secret Mission 6", 805},
    Room {"Secret Mission 7", 806},
    Room {"Secret Mission 8", 807},
    Room {"Secret Mission 9", 808},
    Room {"Secret Mission 10", 809},
    Room {"Secret Mission 11", 810},
    Room {"Secret Mission 12", 811},
    Room {"Bloody Palace 1-19", 705},
    Room {"Bloody Palace 21-39", 704},
    Room {"Bloody Palace 41-59", 703},
    Room {"Bloody Palace 61-79", 701},
    Room {"Bloody Palace 81-99", 702}
};

auto is_valid_room_id = [](int id) {
    return std::any_of(room_items.begin(), room_items.end(), [id](const Room& room) { 
        return room.id == id; 
    });
};

naked void detour() {
	// steam   DevilMayCry4_DX9.exe+E1F6   - 8B 92 30380000        - mov edx, [edx+00003830]
	// nosteam DevilMayCry4_DX9.exe+546E76 - 8B 92 30380000        - mov edx, [edx+00003830]
	__asm {
		mov edx, [edx+0x3830]
		mov DWORD PTR [AreaJump::c_area_jump_ptr], edx
		jmp DWORD PTR [AreaJump::jmp_return]
	}
}

int bp_stage(int floor) {
	auto in_range = [](int value, int low, int high) {return (value >= low) && (value <= high); };
	
	if (in_range(floor, 1, 19)) { return 78; }
	if (in_range(floor, 21, 39)) { return 79; }
	if (in_range(floor, 41, 59)) { return 80; }
	if (in_range(floor, 61, 79)) { return 81; }
	if (in_range(floor, 81, 99)) { return 82; }
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

void AreaJump::jump_to_stage(int stage) {
    switch (stage) {
    case 0:
        c_area_jump_ptr->room_id = 503; // "Bloody Palace 20"
        break;
    case 1:
        c_area_jump_ptr->room_id = 504; // "Bloody Palace 40"
        break;
    case 2:
        c_area_jump_ptr->room_id = 505; // "Bloody Palace 60"
        break;
    case 3:
        c_area_jump_ptr->room_id = 507; // "Bloody Palace 80"
        break;
    case 4:
        c_area_jump_ptr->room_id = 506; // "Bloody Palace 100"
        break;
    case 5:
        c_area_jump_ptr->room_id = 700; // "Bloody Palace 101"
        break;
    case 6:
        c_area_jump_ptr->room_id = 0; // "Opera House"
        break;
    case 7:
        c_area_jump_ptr->room_id = 1; // "Opera House Plaza"
        break;
    case 8:
        c_area_jump_ptr->room_id = 2; // "Storehouse"
        break;
    case 9:
        c_area_jump_ptr->room_id = 3; // "Cathedral"
        break;
    case 10:
        c_area_jump_ptr->room_id = 4; // "Terrace / Business District"
        break;
    case 11:
        c_area_jump_ptr->room_id = 5; // "Residential District"
        break;
    case 12:
        c_area_jump_ptr->room_id = 6; // "Port Caerula"
        break;
    case 13:
        c_area_jump_ptr->room_id = 7; // "Customs House"
        break;
    case 14:
        c_area_jump_ptr->room_id = 8; // "First Mining Area"
        break;
    case 15:
        c_area_jump_ptr->room_id = 9; // "Ferrum Hills"
        break;
    case 16:
        c_area_jump_ptr->room_id = 10; // "M17 Opera House"
        break;
    case 17:
        c_area_jump_ptr->room_id = 11; // "M17 Opera House Plaza"
        break;
    case 18:
        c_area_jump_ptr->room_id = 12; // "Business District / Terrace"
        break;
    case 19:
        c_area_jump_ptr->room_id = 13; // "M20 Opera House Plaza"
        break;
    case 20:
        c_area_jump_ptr->room_id = 100; // "Second Mining Area"
        break;
    case 21:
        c_area_jump_ptr->room_id = 105; // "Fortuna Castle Gate"
        break;
    case 22:
        c_area_jump_ptr->room_id = 200; // "Grand Hall (Fortuna Castle)"
        break;
    case 23:
        c_area_jump_ptr->room_id = 201; // "Large Hall"
        break;
    case 24:
        c_area_jump_ptr->room_id = 202; // "Dining Room"
        break;
    case 25:
        c_area_jump_ptr->room_id = 203; // "Torture Chamber"
        break;
    case 26:
        c_area_jump_ptr->room_id = 204; // "Central Courtyard"
        break;
    case 27:
        c_area_jump_ptr->room_id = 205; // "Foris Falls (Bridge Area)
        break;
    case 28:
        c_area_jump_ptr->room_id = 206; // "Gallery"
        break;
    case 29:
        c_area_jump_ptr->room_id = 207; // "Library"
        break;
    case 30:
        c_area_jump_ptr->room_id = 209; // "Soldier's Graveyard"
        break;
    case 31:
        c_area_jump_ptr->room_id = 210; // "Master's Chamber"
        break;
    case 32:
        c_area_jump_ptr->room_id = 211; // "Spiral Wall"
        break;
    case 33:
        c_area_jump_ptr->room_id = 212; // "Underground Labratory"
        break;
    case 34:
        c_area_jump_ptr->room_id = 213; // "R&D Access"
        break;
    case 35:
        c_area_jump_ptr->room_id = 214; // "Game Room"
        break;
    case 36:
        c_area_jump_ptr->room_id = 215; // "Containment Room"
        break;
    case 37:
        c_area_jump_ptr->room_id = 216; // "Angel Creation"
        break;
    case 38:
        c_area_jump_ptr->room_id = 217; // "Foris Falls (Detour Area)"
        break;
    case 39:
        c_area_jump_ptr->room_id = 300; // "Forest Entrance"
        break;
    case 40:
        c_area_jump_ptr->room_id = 301; // "Windswept Valley"
        break;
    case 41:
        c_area_jump_ptr->room_id = 302; // "Ruined Church"
        break;
    case 42:
        c_area_jump_ptr->room_id = 303; // "Ruined Valley"
        break;
    case 43:
        c_area_jump_ptr->room_id = 304; // "Ancient Training Ground"
        break;
    case 44:
        c_area_jump_ptr->room_id = 305; // "Lapis River"
        break;
    case 45:
        c_area_jump_ptr->room_id = 306; // "Ancient Plaza"
        break;
    case 46:
        c_area_jump_ptr->room_id = 307; // "Den of the She-Viper"
        break;
    case 47:
        c_area_jump_ptr->room_id = 308; // "Forgotten Ruins"
        break;
    case 48:
        c_area_jump_ptr->room_id = 309; // "Hidden Pit"
        break;
    case 49:
        c_area_jump_ptr->room_id = 310; // "Ruined Lowlands"
        break;
    case 50:
        c_area_jump_ptr->room_id = 311; // "Lost Woods"
        break;
    case 51:
        c_area_jump_ptr->room_id = 400; // "Gran Album Bridge"
        break;
    case 52:
        c_area_jump_ptr->room_id = 401; // "Grand Hall (Order of the Sword HQ)"
        break;
    case 53:
        c_area_jump_ptr->room_id = 402; // "Key Chamber"
        break;
    case 54:
        c_area_jump_ptr->room_id = 403; // "The Gauntlet"
        break;
    case 55:
        c_area_jump_ptr->room_id = 404; // "Agnus' Room"
        break;
    case 56:
        c_area_jump_ptr->room_id = 405; // "Security Corridor"
        break;
    case 57:
        c_area_jump_ptr->room_id = 406; // "Experiment Disposal"
        break;
    case 58:
        c_area_jump_ptr->room_id = 407; // "Meeting Room"
        break;
    case 59:
        c_area_jump_ptr->room_id = 408; // "Ascension Chamber"
        break;
    case 60:
        c_area_jump_ptr->room_id = 409; // "Advent Chamber"
        break;
    case 61:
        c_area_jump_ptr->room_id = 500; // "Machina Ex Deus"
        break;
    case 62:
        c_area_jump_ptr->room_id = 501; // "Stairway to Heaven"
        break;
    case 63:
        c_area_jump_ptr->room_id = 502; // "Sacred Heart"
        break;
    case 64:
        c_area_jump_ptr->room_id = 510; // "M18"
        break;
    case 65:
        c_area_jump_ptr->room_id = 512; // "Sky Above Fortuna"
        break;
    case 66:
        c_area_jump_ptr->room_id = 800; // "Secret Mission 1"
        break;
    case 67:
        c_area_jump_ptr->room_id = 801; // "Secret Mission 2"
        break;
    case 68:
        c_area_jump_ptr->room_id = 802; // "Secret Mission 3"
        break;
    case 69:
        c_area_jump_ptr->room_id = 803; // "Secret Mission 4"
        break;
    case 70:
        c_area_jump_ptr->room_id = 804; // "Secret Mission 5"
        break;
    case 71:
        c_area_jump_ptr->room_id = 805; // "Secret Mission 6"
        break;
    case 72:
        c_area_jump_ptr->room_id = 806; // "Secret Mission 7"
        break;
    case 73:
        c_area_jump_ptr->room_id = 807; // "Secret Mission 8"
        break;
    case 74:
        c_area_jump_ptr->room_id = 808; // "Secret Mission 9"
        break;
    case 75:
        c_area_jump_ptr->room_id = 809; // "Secret Mission 10"
        break;
    case 76:
        c_area_jump_ptr->room_id = 810; // "Secret Mission 11"
        break;
    case 77:
        c_area_jump_ptr->room_id = 811; // "Secret Mission 12"
        break;
    case 78:
        c_area_jump_ptr->room_id = 705; // "Bloody Palace 1-19"
        break;
    case 79:
        c_area_jump_ptr->room_id = 704; // "Bloody Palace 21-39"
        break;
    case 80:
        c_area_jump_ptr->room_id = 703; // "Bloody Palace 41-59"
        break;
    case 81:
        c_area_jump_ptr->room_id = 701; // "Bloody Palace 61-79"
        break;
    case 82:
        c_area_jump_ptr->room_id = 702; // "Bloody Palace 81-99"
        break;
    }
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

    console->system().RegisterCommand("room", "Jump to room ID",
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

	int room_item_current = 0;
	/*if (ImGui::ListBox(_("##Room Codes Listbox"), &room_item_current, room_items.data(), room_items.size(), 10)) {
		jump_to_stage(room_item_current);
	}*/
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
