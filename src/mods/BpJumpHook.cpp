
#include "BpJumpHook.hpp"
// #include "AreaJump.hpp"
#include "random"
#include "numeric"

bool BpJumpHook::mod_enabled{ false };
static bool bp_boss_rush{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
uintptr_t BpJumpHook::bp_jump_hook1_continue{ NULL };
uintptr_t BpJumpHook::bp_jump_hook2_continue{ NULL };
uintptr_t BpJumpHook::bp_jump_hook3_continue{ NULL };

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
    // randomize_bp_floors(); // this now happens on boot or when ticking the checkbox

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
        cmp byte ptr [BpJumpHook::mod_enabled], 1
        je InitRandomStages
        cmp byte ptr [bp_boss_rush], 0
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
        cmp byte ptr [bp_boss_rush], 1
        je bossrush
        push [stage_start_id]
        jmp retcode

    bossrush:
        cmp byte ptr [BpJumpHook::mod_enabled], 1
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
		jmp dword ptr [BpJumpHook::bp_jump_hook1_continue]
    }
}

naked void bp_jump_hook2_proc(void) { // Initial load of BP
    _asm {
        cmp byte ptr [bp_boss_rush], 1
        je bossrush

        cmp byte ptr [BpJumpHook::mod_enabled], 0
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
        cmp byte ptr [BpJumpHook::mod_enabled], 1 // should randomize?
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
		jmp dword ptr [BpJumpHook::bp_jump_hook2_continue]
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
        cmp byte ptr [bp_boss_rush], 1
        je bossrush

        cmp byte ptr [BpJumpHook::mod_enabled], 0
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
        cmp byte ptr [BpJumpHook::mod_enabled], 1 // should randomize?
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
		jmp dword ptr [BpJumpHook::bp_jump_hook3_continue]
    }
}

void BpJumpHook::toggle(bool enable) { // randomized bp
    if (enable) {
        install_patch_offset(0x04B808, patch1, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x04C200, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else {
        patch1.reset();
        patch2.reset();
    }
}

std::optional<std::string> BpJumpHook::on_initialize() {
    if (!install_hook_offset(0x4AB8E1, hook1, &bp_jump_hook1_proc, &BpJumpHook::bp_jump_hook1_continue, 5)) {
        spdlog::error("Failed to init bpJumpHook1 mod\n");
        return "Failed to init bpJumpHook1 mod";
    }
    if (!install_hook_offset(0x4AB8FA, hook2, &bp_jump_hook2_proc, &BpJumpHook::bp_jump_hook2_continue, 7)) {
        spdlog::error("Failed to init bpJumpHook2 mod\n");
        return "Failed to init bpJumpHook2 mod";
    }
    if (!install_hook_offset(0x04A974, hook3, &bp_jump_hook3_proc, &BpJumpHook::bp_jump_hook3_continue, 6)) {
        spdlog::error("Failed to init bpJumpHook3 mod\n");
        return "Failed to init bpJumpHook3 mod";
    }
    return Mod::on_initialize();
}

void BpJumpHook::on_gui_frame() {
    if (ImGui::Checkbox(_("Randomize BP"), &mod_enabled)) {
        toggle(mod_enabled);
        randomize_bp_floors();
    }
    ImGui::SameLine();
    help_marker(_("All stages will be randomized. Enable before selecting BP"));
    ImGui::SameLine(sameLineWidth);
    ImGui::Checkbox(_("Boss Rush"), &bp_boss_rush);
    ImGui::SameLine();
    help_marker(_("Only boss stages will be played. Enable before selecting BP\nCan be used with Randomize BP"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        if (ImGui::CollapsingHeader(_("View Randomized Stages"))) {
                ImGui::Text(_("Stage Number"));
                ImGui::SameLine(sameLineWidth);
                ImGui::Text(_("Random Stage"));
            if (!bp_boss_rush) {
                int stage_count = 1;
                for (int i : bp_array) {
                    ImGui::Text("%i", stage_count);
                    stage_count++;
                    ImGui::SameLine(sameLineWidth);
                    ImGui::Text("%i", i);
                }
            }
            else {
                int stage_count = 1;
                for (int i : boss_array) {
                    ImGui::Text("%i", stage_count);
                    stage_count++;
                    ImGui::SameLine(sameLineWidth);
                    ImGui::Text("%i", i);
                }
            }
            ImGui::Separator();
        }
        ImGui::Unindent(lineIndent);
    }
}

void BpJumpHook::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("randomize_bp").value_or(false);
    if (mod_enabled) {
        randomize_bp_floors();
        toggle(mod_enabled);
    }
    bp_boss_rush = cfg.get<bool>("bp_boss_rush").value_or(false);
}

void BpJumpHook::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("randomize_bp", mod_enabled);
    cfg.set<bool>("bp_boss_rush", bp_boss_rush);
}
