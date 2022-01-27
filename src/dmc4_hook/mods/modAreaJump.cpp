#include "modAreaJump.hpp"
#include <array>
#include <algorithm>

uintptr_t  AreaJump::jmp_return{ NULL };
cAreaJump* AreaJump::cAreaJumpPtr{ NULL };

constexpr std::array room_items {
	"Berial",                             // 503
	"Bael",                               // 504
	"Echidna",                            // 505
	"Credo",                              // 507
	"Agnus",                              // 506
	"Dante",                              // 700
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
	// steam   DevilMayCry4_DX9.exe+E1F6   - 8B 92 30380000        - mov edx,[edx+00003830]
	// nosteam DevilMayCry4_DX9.exe+546E76 - 8B 92 30380000        - mov edx,[edx+00003830]
	__asm {
		mov edx, [edx+3830h]
		mov DWORD PTR [AreaJump::cAreaJumpPtr], edx
		jmp DWORD PTR [AreaJump::jmp_return]
	}
}

AreaJump::AreaJump() {
	//m_exeType = i;
	//onInitialize();
}

std::optional<std::string> AreaJump::onInitialize() {

	uintptr_t address = hl::FindPattern("8B 92 30 38 00 00", "DevilMayCry4_DX9.exe"); // DevilMayCry4_DX9.exe+E1F6 
    if (!install_hook_offset(0x00E1F6, hook, &detour, &AreaJump::jmp_return, 6))
        {
            HL_LOG_ERR("Failed to init AreaJump mod\n");
            return "Failed to init AreaJump mod";
        }

	return Mod::onInitialize();
}

int bpStage(int floor) {
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

void AreaJump::jumpToStage(int stage) {
    switch (stage)
    {
    case 0:
        cAreaJumpPtr->roomId = 503; // "Bloody Palace 20"
        break;
    case 1:
        cAreaJumpPtr->roomId = 504; // "Bloody Palace 40"
        break;
    case 2:
        cAreaJumpPtr->roomId = 505; // "Bloody Palace 60"
        break;
    case 3:
        cAreaJumpPtr->roomId = 507; // "Bloody Palace 80"
        break;
    case 4:
        cAreaJumpPtr->roomId = 506; // "Bloody Palace 100"
        break;
    case 5:
        cAreaJumpPtr->roomId = 700; // "Bloody Palace 101"
        break;
    case 6:
        cAreaJumpPtr->roomId = 0; // "Opera House"
        break;
    case 7:
        cAreaJumpPtr->roomId = 1; // "Opera House Plaza"
        break;
    case 8:
        cAreaJumpPtr->roomId = 2; // "Storehouse"
        break;
    case 9:
        cAreaJumpPtr->roomId = 3; // "Cathedral"
        break;
    case 10:
        cAreaJumpPtr->roomId = 4; // "Terrace / Business District"
        break;
    case 11:
        cAreaJumpPtr->roomId = 5; // "Residential District"
        break;
    case 12:
        cAreaJumpPtr->roomId = 6; // "Port Caerula"
        break;
    case 13:
        cAreaJumpPtr->roomId = 7; // "Customs House"
        break;
    case 14:
        cAreaJumpPtr->roomId = 8; // "First Mining Area"
        break;
    case 15:
        cAreaJumpPtr->roomId = 9; // "Ferrum Hills"
        break;
    case 16:
        cAreaJumpPtr->roomId = 10; // "M17 Opera House"
        break;
    case 17:
        cAreaJumpPtr->roomId = 11; // "M17 Opera House Plaza"
        break;
    case 18:
        cAreaJumpPtr->roomId = 12; // "Business District / Terrace"
        break;
    case 19:
        cAreaJumpPtr->roomId = 13; // "M20 Opera House Plaza"
        break;
    case 20:
        cAreaJumpPtr->roomId = 100; // "Second Mining Area"
        break;
    case 21:
        cAreaJumpPtr->roomId = 105; // "Fortuna Castle Gate"
        break;
    case 22:
        cAreaJumpPtr->roomId = 200; // "Grand Hall (Fortuna Castle)"
        break;
    case 23:
        cAreaJumpPtr->roomId = 201; // "Large Hall"
        break;
    case 24:
        cAreaJumpPtr->roomId = 202; // "Dining Room"
        break;
    case 25:
        cAreaJumpPtr->roomId = 203; // "Torture Chamber"
        break;
    case 26:
        cAreaJumpPtr->roomId = 204; // "Central Courtyard"
        break;
    case 27:
        cAreaJumpPtr->roomId = 205; // "Foris Falls (Bridge Area)
        break;
    case 28:
        cAreaJumpPtr->roomId = 206; // "Gallery"
        break;
    case 29:
        cAreaJumpPtr->roomId = 207; // "Library"
        break;
    case 30:
        cAreaJumpPtr->roomId = 209; // "Soldier's Graveyard"
        break;
    case 31:
        cAreaJumpPtr->roomId = 210; // "Master's Chamber"
        break;
    case 32:
        cAreaJumpPtr->roomId = 211; // "Spiral Wall"
        break;
    case 33:
        cAreaJumpPtr->roomId = 212; // "Underground Labratory"
        break;
    case 34:
        cAreaJumpPtr->roomId = 213; // "R&D Access"
        break;
    case 35:
        cAreaJumpPtr->roomId = 214; // "Game Room"
        break;
    case 36:
        cAreaJumpPtr->roomId = 215; // "Containment Room"
        break;
    case 37:
        cAreaJumpPtr->roomId = 216; // "Angel Creation"
        break;
    case 38:
        cAreaJumpPtr->roomId = 217; // "Foris Falls (Detour Area)"
        break;
    case 39:
        cAreaJumpPtr->roomId = 300; // "Forest Entrance"
        break;
    case 40:
        cAreaJumpPtr->roomId = 301; // "Windswept Valley"
        break;
    case 41:
        cAreaJumpPtr->roomId = 302; // "Ruined Church"
        break;
    case 42:
        cAreaJumpPtr->roomId = 303; // "Ruined Valley"
        break;
    case 43:
        cAreaJumpPtr->roomId = 304; // "Ancient Training Ground"
        break;
    case 44:
        cAreaJumpPtr->roomId = 305; // "Lapis River"
        break;
    case 45:
        cAreaJumpPtr->roomId = 306; // "Ancient Plaza"
        break;
    case 46:
        cAreaJumpPtr->roomId = 307; // "Den of the She-Viper"
        break;
    case 47:
        cAreaJumpPtr->roomId = 308; // "Forgotten Ruins"
        break;
    case 48:
        cAreaJumpPtr->roomId = 309; // "Hidden Pit"
        break;
    case 49:
        cAreaJumpPtr->roomId = 310; // "Ruined Lowlands"
        break;
    case 50:
        cAreaJumpPtr->roomId = 311; // "Lost Woods"
        break;
    case 51:
        cAreaJumpPtr->roomId = 400; // "Gran Album Bridge"
        break;
    case 52:
        cAreaJumpPtr->roomId = 401; // "Grand Hall (Order of the Sword HQ)"
        break;
    case 53:
        cAreaJumpPtr->roomId = 402; // "Key Chamber"
        break;
    case 54:
        cAreaJumpPtr->roomId = 403; // "The Gauntlet"
        break;
    case 55:
        cAreaJumpPtr->roomId = 404; // "Agnus' Room"
        break;
    case 56:
        cAreaJumpPtr->roomId = 405; // "Security Corridor"
        break;
    case 57:
        cAreaJumpPtr->roomId = 406; // "Experiment Disposal"
        break;
    case 58:
        cAreaJumpPtr->roomId = 407; // "Meeting Room"
        break;
    case 59:
        cAreaJumpPtr->roomId = 408; // "Ascension Chamber"
        break;
    case 60:
        cAreaJumpPtr->roomId = 409; // "Advent Chamber"
        break;
    case 61:
        cAreaJumpPtr->roomId = 500; // "Machina Ex Deus"
        break;
    case 62:
        cAreaJumpPtr->roomId = 501; // "Stairway to Heaven"
        break;
    case 63:
        cAreaJumpPtr->roomId = 502; // "Sacred Heart"
        break;
    case 64:
        cAreaJumpPtr->roomId = 510; // "M18"
        break;
    case 65:
        cAreaJumpPtr->roomId = 512; // "Sky Above Fortuna"
        break;
    case 66:
        cAreaJumpPtr->roomId = 800; // "Secret Mission 1"
        break;
    case 67:
        cAreaJumpPtr->roomId = 801; // "Secret Mission 2"
        break;
    case 68:
        cAreaJumpPtr->roomId = 802; // "Secret Mission 3"
        break;
    case 69:
        cAreaJumpPtr->roomId = 803; // "Secret Mission 4"
        break;
    case 70:
        cAreaJumpPtr->roomId = 804; // "Secret Mission 5"
        break;
    case 71:
        cAreaJumpPtr->roomId = 805; // "Secret Mission 6"
        break;
    case 72:
        cAreaJumpPtr->roomId = 806; // "Secret Mission 7"
        break;
    case 73:
        cAreaJumpPtr->roomId = 807; // "Secret Mission 8"
        break;
    case 74:
        cAreaJumpPtr->roomId = 808; // "Secret Mission 9"
        break;
    case 75:
        cAreaJumpPtr->roomId = 809; // "Secret Mission 10"
        break;
    case 76:
        cAreaJumpPtr->roomId = 810; // "Secret Mission 11"
        break;
    case 77:
        cAreaJumpPtr->roomId = 811; // "Secret Mission 12"
        break;
    case 78:
        cAreaJumpPtr->roomId = 705; // "Bloody Palace 1-19"
        break;
    case 79:
        cAreaJumpPtr->roomId = 704; // "Bloody Palace 21-39"
        break;
    case 80:
        cAreaJumpPtr->roomId = 703; // "Bloody Palace 41-59"
        break;
    case 81:
        cAreaJumpPtr->roomId = 701; // "Bloody Palace 61-79"
        break;
    case 82:
        cAreaJumpPtr->roomId = 702; // "Bloody Palace 81-99"
        break;
    }
	cAreaJumpPtr->initJump = 1;
}

void AreaJump::onGUIframe() 
{
	if (IsBadWritePtr(cAreaJumpPtr, sizeof(uint32_t)) || IsBadReadPtr(cAreaJumpPtr,sizeof(uint32_t)))
    {
		ImGui::TextWrapped("Area Jump is not initialized.\nLoad into a stage to access it.");
		return;
	}

    ImGui::Text("Bloody Palace Floor Teleports");

    ImGui::Spacing();

	if (cAreaJumpPtr->bpFloorStage)
    {
		if (ImGui::InputInt("##BP Floor ", &cAreaJumpPtr->bpFloorStage, 1, 10, ImGuiInputTextFlags_AllowTabInput)) {
			cAreaJumpPtr->bpFloorStage = std::clamp(cAreaJumpPtr->bpFloorStage, 1, 101);
		}

        if (ImGui::Button("Go", ImVec2(290, 20)))
        {
			jumpToStage(bpStage(cAreaJumpPtr->bpFloorStage));
		}
	}
    else
    {
        ImGui::TextWrapped("BP Floor Jump is not initialized.\nLoad into BP to access it.");
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Mission Area Teleports");

    ImGui::Spacing();

	int room_item_current = 0;
	if (ImGui::ListBox("##Room Codes Listbox", &room_item_current, room_items.data(), room_items.size(), 10))
	{
		jumpToStage(room_item_current);
	}
}
