#include "../mods.h"
#include "modBpJumpHook.hpp"
// #include "modAreaJump.hpp"
#include "random"
#include "numeric"

bool BpJumpHook::modEnabled{ false };
static bool bpBossRush{ false };
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
uintptr_t BpJumpHook::_bpJumpHook1Continue{ NULL };
uintptr_t BpJumpHook::_bpJumpHook2Continue{ NULL };
uintptr_t BpJumpHook::_bpJumpHook3Continue{ NULL };

static int bpFloor = 0;
static int bpAreaID = 0;
static int numberOfCompleteFloors = 0;
static std::array<uint32_t, 100> bpArray;
static char* stageStartID;

static int bossFloor = 0;
static int bossAreaID = 0;
static int numberOfCompleteBosses = 0;
static std::array<uint32_t, 5> bossArray;
static char* bossStartID;

// DevilMayCry4_DX9.exe+4AB8E1 - push []
char BossStartIDs[11][5] {
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
    numberOfCompleteFloors = 0;
    std::iota(bpArray.begin(), bpArray.end(), 1); // fill the list starting from 1
    std::random_device rd;                        // random device for shuffle
    std::mt19937 g(rd());
    std::shuffle(bpArray.begin(), bpArray.end(), g);
}

void randomize_bp_bosses() {
    numberOfCompleteBosses = 0;
    bossArray[0] = 20;
    bossArray[1] = 40;
    bossArray[2] = 60;
    bossArray[3] = 80;
    bossArray[4] = 100;
    std::random_device rd; // random device for shuffle
    std::mt19937 g(rd());
    std::shuffle(bossArray.begin(), bossArray.end(), g);
}

void random_boss_init(void) {
    // randomize bp bosses
    randomize_bp_bosses();

    // apply floor
    bossFloor = bossArray[numberOfCompleteBosses];

    // set area jump
    bossAreaID = convert_switch_to_area_id(convert_floor_to_switch(bossFloor));

    // set stage load
    bossStartID = *(char**)BossStartIDs[convert_floor_to_switch(bossFloor)];
}

void boss_continue() { // used every time you enter a teleporter during randomized boss rush
    // inc number of complete floors
    numberOfCompleteBosses++;

    if (numberOfCompleteBosses == 5) {
        bossFloor = 101;
        bossAreaID = 700;
        return;
    }

    // apply floor
    bossFloor = bossArray[numberOfCompleteBosses];

    // set area jump
    bossAreaID = convert_switch_to_area_id(convert_floor_to_switch(bossFloor));
}

void random_bp_init() { // used in first bp detour
    // randomize bp floors
    randomize_bp_floors();

    // apply floor
    bpFloor = bpArray[numberOfCompleteFloors];

    // set area jump
    bpAreaID = convert_switch_to_area_id(convert_floor_to_switch(bpFloor));

    // set stage load
    stageStartID = *(char**)BossStartIDs[convert_floor_to_switch(bpFloor)];
}

void bp_continue() { // used in other bp detours
    // inc number of complete floors
    numberOfCompleteFloors++;

    if (numberOfCompleteFloors == 100) {
        bpFloor = 101;
        bpAreaID = 700;
        return;
    }

    // apply floor
    bpFloor = bpArray[numberOfCompleteFloors];

    // set area jump
    bpAreaID = convert_switch_to_area_id(convert_floor_to_switch(bpFloor));
}

naked void bpJumpHook1_proc(void) { // Initial load of BP, picks stage to load
    _asm {
    // check both cheats are off before skipping randomizing
        cmp byte ptr [BpJumpHook::modEnabled], 1
        je InitRandomStages
        cmp byte ptr [bpBossRush], 0
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
        cmp byte ptr [bpBossRush], 1
        je bossrush
        push [stageStartID]
        jmp retcode

    bossrush:
        cmp byte ptr [BpJumpHook::modEnabled], 1
        je randombossrushstart
    // if boss rush isn't randomized
        push [0x00E56568] // berial
        jmp retcode

    // push appropriate stage for whichever area we got
    randombossrushstart:
        push [bossStartID]
        jmp retcode

    code:
        push [0x00E56768]
    retcode:
		jmp dword ptr [BpJumpHook::_bpJumpHook1Continue]
    }
}

naked void bpJumpHook2_proc(void) { // Initial load of BP
    _asm {
        cmp byte ptr [bpBossRush], 1
        je bossrush

        cmp byte ptr [BpJumpHook::modEnabled], 0
        je code

    // write randomized floor and area ID
        push edx
        mov edx, [bpFloor]
        mov [eax+74h], edx
        mov edx, [bpAreaID]
        mov [eax+6Ch], edx
        pop edx
        jmp retcode

    bossrush:
        cmp byte ptr [BpJumpHook::modEnabled], 1 // should randomize?
        je randombossrushstart
        mov dword ptr [eax+74h], 20 // room id
        mov dword ptr [eax+6Ch], 503 // area id
        jmp retcode

    // write randomized boss stage and area id
    randombossrushstart:
        push edx
        mov edx, [bossFloor]
        mov [eax+74h], edx
        mov edx, [bossAreaID]
        mov [eax+6Ch], edx
        pop edx
        jmp retcode

    code:
        mov dword ptr [eax+74h], 00000001
    retcode:
		jmp dword ptr [BpJumpHook::_bpJumpHook2Continue]
    }
}

naked void bpJumpHook3_proc(void) { // called every time you enter a teleporter
    _asm {
        push eax
        mov eax, [staticMediatorPtr]
        mov eax, [eax]
        cmp dword ptr [eax+0xE0], 50
        pop eax
        jne code
        cmp byte ptr [bpBossRush], 1
        je bossrush

        cmp byte ptr [BpJumpHook::modEnabled], 0
        je code

    // update floor and area ID values
        push eax
        push ecx
        call bp_continue
        pop ecx
        pop eax

    // write bp floor
        push edx
        mov edx, [bpFloor]
        mov [ecx+74h], edx
        mov edx, [bpAreaID]
        mov [ecx+6Ch], edx
        pop edx
        jmp retcode

    bossrush:
        cmp byte ptr [BpJumpHook::modEnabled], 1 // should randomize?
        je randombossrushcontinue
        cmp dword ptr [ecx+74h], 21
        je stage40
        cmp dword ptr [ecx+74h], 41
        je stage60
        cmp dword ptr [ecx+74h], 61
        je stage80
        cmp dword ptr [ecx+74h], 81
        je stage100
        jmp code

    stage40:
        mov dword ptr [ecx+74h], 40
        mov dword ptr [ecx+6Ch], 504
        jmp retcode
    stage60:
        mov dword ptr [ecx+74h], 60
        mov dword ptr [ecx+6Ch], 505
        jmp retcode
    stage80:
        mov dword ptr [ecx+74h], 80
        mov dword ptr [ecx+6Ch], 507
        jmp retcode
    stage100:
        mov dword ptr [ecx+74h], 100
        mov dword ptr [ecx+6Ch], 506
        jmp retcode

    // update floor and area ID values
    randombossrushcontinue:
        push eax
        push ecx
        call boss_continue
        pop ecx
        pop eax

    // write boss floor
        push edx
        mov edx, [bossFloor]
        mov [ecx+74h], edx
        mov edx, [bossAreaID]
        mov [ecx+6Ch], edx
        pop edx
        jmp retcode

    code:
        mov dword ptr [ecx+6Ch], edx
    retcode:
        mov edx, [ebx+04h]
		jmp dword ptr [BpJumpHook::_bpJumpHook3Continue]
    }
}

void BpJumpHook::toggle(bool enable) { // randomized bp
    if (enable) {
        install_patch_offset(0x04B808, patch1, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x04C200, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.revert();
        patch2.revert();
    }
}

std::optional<std::string> BpJumpHook::onInitialize() {
    if (!install_hook_offset(0x4AB8E1, hook1, &bpJumpHook1_proc, &BpJumpHook::_bpJumpHook1Continue, 5)) {
        HL_LOG_ERR("Failed to init bpJumpHook1 mod\n");
        return "Failed to init bpJumpHook1 mod";
    }
    if (!install_hook_offset(0x4AB8FA, hook2, &bpJumpHook2_proc, &BpJumpHook::_bpJumpHook2Continue, 7)) {
        HL_LOG_ERR("Failed to init bpJumpHook2 mod\n");
        return "Failed to init bpJumpHook2 mod";
    }
    if (!install_hook_offset(0x04A974, hook3, &bpJumpHook3_proc, &BpJumpHook::_bpJumpHook3Continue, 6)) {
        HL_LOG_ERR("Failed to init bpJumpHook3 mod\n");
        return "Failed to init bpJumpHook3 mod";
    }
    return Mod::onInitialize();
}

void BpJumpHook::onGUIframe() {
    if (ImGui::Checkbox("Randomize BP", &modEnabled)) {
        toggle(modEnabled);
    }
    ImGui::SameLine();
    HelpMarker("Enable before starting BP");
    ImGui::SameLine(205);
    ImGui::Checkbox("Boss Rush", &bpBossRush);
    ImGui::SameLine();
    HelpMarker("Enable before starting BP");

    if (modEnabled) {
        if (ImGui::CollapsingHeader("View Randomized Stages")) {
                ImGui::Text("Stage Number");
                ImGui::SameLine(205);
                ImGui::Text("Random Stage");
            if (!bpBossRush) {
                int stageCount = 1;
                for (int i : bpArray) {
                    ImGui::Text("%i", stageCount);
                    stageCount++;
                    ImGui::SameLine(205);
                    ImGui::Text("%i", i);
                }
            }
            else {
                int stageCount = 1;
                for (int i : bossArray) {
                    ImGui::Text("%i", stageCount);
                    stageCount++;
                    ImGui::SameLine(205);
                    ImGui::Text("%i", i);
                }
            }
            ImGui::Separator();
        }
    }
}

void BpJumpHook::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("randomize_bp").value_or(false);
    bpBossRush = cfg.get<bool>("bp_boss_rush").value_or(false);
    toggle(modEnabled);
}

void BpJumpHook::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("randomize_bp", modEnabled);
    cfg.set<bool>("bp_boss_rush", bpBossRush);
}
