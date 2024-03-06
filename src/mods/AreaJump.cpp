#include "AreaJump.hpp"
#include <array>
#include <algorithm>
#include "RoomRespawn.hpp"

uintptr_t  AreaJump::jmp_return{ NULL };
cAreaJump* AreaJump::c_area_jump_ptr{ NULL };

constexpr std::array room_items {
	"Berial",                             // 503 // DevilMayCry4_DX9.exe+A56768
	"Bael",                               // 504 // DevilMayCry4_DX9.exe+A56528
	"Echidna",                            // 505 // DevilMayCry4_DX9.exe+A55FE8
	"Credo",                              // 507 // DevilMayCry4_DX9.exe+A55F88
	"Agnus",                              // 506 // DevilMayCry4_DX9.exe+A56728 
	"Dante",                              // 700 // DevilMayCry4_DX9.exe+A56508
	"Opera House",                        // 0
	"Opera House Plaza",                  // 1
	"Storehouse",                         // 2
	"Cathedral",                          // 3
	"Terrace / Business District",        // 4
	"Residential District",               // 5
	"Port Caerula",                       // 6
	"Customs House",                      // 7
	"First Mining Area",                  // 8
	"Ferrum Hills",                       // 9
	"M17 Opera House",                    // 10
	"M17 Opera House Plaza",              // 11
	"Business District / Terrace",        // 12
	"M20 Opera House Plaza",              // 13
	"Second Mining Area",                 // 100
	"Fortuna Castle Gate",                // 105
	"Grand Hall (Fortuna Castle)",        // 200
	"Large Hall",                         // 201
	"Dining Room",                        // 202
	"Torture Chamber",                    // 203
	"Central Courtyard",                  // 204
	"Foris Falls (Bridge Area)",          // 205
	"Gallery",                            // 206
	"Library",                            // 207
	"Soldier's Graveyard",                // 209
	"Master's Chamber",                   // 210
	"Spiral Well",                        // 211
	"Underground Laboratory",             // 212
	"R&D Access",                         // 213
	"Game Room",                          // 214
	"Containment Room",                   // 215
	"Angel Creation",                     // 216
	"Foris Falls (Detour Area)",          // 217
	"Forest Entrance",                    // 300
	"Windswept Valley",                   // 301
	"Ruined Church",                      // 302
	"Ruined Valley",                      // 303
	"Ancient Training Ground",            // 304
	"Lapis River",                        // 305
	"Ancient Plaza",                      // 306
	"Den of the She-Viper",               // 307
	"Forgotten Ruins",                    // 308
	"Hidden Pit",                         // 309
	"Ruined Lowlands",                    // 310
	"Lost Woods",                         // 311
	"Gran Album Bridge",                  // 400
	"Grand Hall (Order of the Sword HQ)", // 401
	"Key Chamber",                        // 402
	"The Gauntlet",                       // 403
	"Agnus' Room",                        // 404
	"Security Corridor",                  // 405
	"Experiment Disposal",                // 406
	"Meeting Room",                       // 407
	"Ascension Chamber",                  // 408
	"Advent Chamber",                     // 409
	"Machina Ex Deus",                    // 500
	"Stairway to Heaven",                 // 501
	"Sacred Heart",                       // 502
	"M18",                                // 510
	"Sky Above Fortuna",                  // 512
	"Secret Mission 1",                   // 800
	"Secret Mission 2",                   // 801
	"Secret Mission 3",                   // 802
	"Secret Mission 4",                   // 803
	"Secret Mission 5",                   // 804
	"Secret Mission 6",                   // 805
	"Secret Mission 7",                   // 806
	"Secret Mission 8",                   // 807
	"Secret Mission 9",                   // 808
	"Secret Mission 10",                  // 809
	"Secret Mission 11",                  // 810
	"Secret Mission 12",                  // 811
    "Bloody Palace 1-19",                 // 705
    "Bloody Palace 21-39",                // 704
    "Bloody Palace 41-59",                // 703
    "Bloody Palace 61-79",                // 701
    "Bloody Palace 81-99"                 // 702
};

naked void detour() {
	// steam   DevilMayCry4_DX9.exe+E1F6   - 8B 92 30380000        - mov edx, [edx+00003830]
	// nosteam DevilMayCry4_DX9.exe+546E76 - 8B 92 30380000        - mov edx, [edx+00003830]
	__asm {
		mov edx, [edx+3830h]
		mov DWORD PTR [AreaJump::c_area_jump_ptr], edx
		jmp DWORD PTR [AreaJump::jmp_return]
	}
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

	return Mod::on_initialize();
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

void AreaJump::on_gui_frame() 
{
	if (IsBadWritePtr(c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(c_area_jump_ptr,sizeof(uint32_t))) {
		ImGui::TextWrapped("Area Jump is not initialized.\nLoad into a stage to access it.");
		return;
	}

    ImGui::Text("Bloody Palace Floor Teleports");

    ImGui::Spacing();

	if (c_area_jump_ptr->bp_floor_stage) {
		if (ImGui::InputInt("##BP Floor ", &c_area_jump_ptr->bp_floor_stage, 1, 10, ImGuiInputTextFlags_AllowTabInput)) {
			c_area_jump_ptr->bp_floor_stage = std::clamp(c_area_jump_ptr->bp_floor_stage, 1, 101);
		}

        if (ImGui::Button("Go", ImVec2(290, 20))) {
			jump_to_stage(bp_stage(c_area_jump_ptr->bp_floor_stage));
		}
	}
    else {
        ImGui::TextWrapped("BP Floor Jump is not initialized.\nLoad into BP to access it.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Mission Area Teleports");

    ImGui::Spacing();

	int room_item_current = 0;
	if (ImGui::ListBox("##Room Codes Listbox", &room_item_current, room_items.data(), room_items.size(), 10)) {
		jump_to_stage(room_item_current);
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
