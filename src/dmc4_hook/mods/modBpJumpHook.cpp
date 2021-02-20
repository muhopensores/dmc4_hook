#include "../mods.h"
#include "modBpJumpHook.hpp"
#include "ctime"

int bpArray[100];
int bpFloor = 0;
bool didBpFloorAlreadyHappen = false;
int numberOfCompleteFloors = 0;
int attemptedBpFloor = 0;
uintptr_t* areaJumpPtr = NULL;

int bpStageArea(int floor) {
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

void jumpToStage(int stage)
{
    switch (stage)
    {
    case 0:
        *areaJumpPtr = 503; // "Bloody Palace 20"
        break;
    case 1:
        *areaJumpPtr = 504; // "Bloody Palace 40"
        break;
    case 2:
        *areaJumpPtr = 505; // "Bloody Palace 60"
        break;
    case 3:
        *areaJumpPtr = 507; // "Bloody Palace 80"
        break;
    case 4:
        *areaJumpPtr = 506; // "Bloody Palace 100"
        break;
    case 5:
        *areaJumpPtr = 700; // "Bloody Palace 101"
        break;
    case 6:
        *areaJumpPtr = 705; // "Bloody Palace 1-19"
        break;
    case 7:
        *areaJumpPtr = 704; // "Bloody Palace 21-39"
        break;
    case 8:
        *areaJumpPtr = 703; // "Bloody Palace 41-59"
        break;
    case 9:
        *areaJumpPtr = 701; // "Bloody Palace 61-79"
        break;
    case 10:
        *areaJumpPtr = 702; // "Bloody Palace 81-99"
        // AreaJump::cAreaJumpPtr->roomId
        break;
    }
}

int random_num(int range_start, int range_end)
{
    std::srand((unsigned int)time(0));
    return range_start + (std::rand() % (range_end - range_start + 1));
}

bool check_complete_floors()
{
    for (int i = 0; i < numberOfCompleteFloors; i++)
    {
        if (bpArray[i] == attemptedBpFloor)
        {
            return true;
        }
    }
    return false;
}

void reset_random_bp()
{
    std::fill_n(bpArray, 100, 0); // fill array with 0
    numberOfCompleteFloors = 0;
    return;
}

void get_random_bp_floor()
{
    do
    {
        attemptedBpFloor = random_num(1, 101);             // put a random number in attemptedBpFloor
        didBpFloorAlreadyHappen = check_complete_floors(); // see if it's already been played
    } 
    while (didBpFloorAlreadyHappen == true);             // if it has been played, get another number

    // save bp floor completion
    bpFloor = attemptedBpFloor;
    bpArray[numberOfCompleteFloors] = bpFloor;

    // inc number of complete floors
    numberOfCompleteFloors++;

    // set area jump pointer
    jumpToStage(bpStageArea(bpFloor));
    return;
}

//___________________________________________________________________________________

bool BpJumpHook::modEnabled{ false };
uintptr_t BpJumpHook::_bpJumpHookContinue{ NULL };
uintptr_t BpJumpHook::_bpJumpHook2Continue{ NULL };
uintptr_t BpJumpHook::_bpJumpHook3Continue{ NULL };

BpJumpHook::BpJumpHook()
{
    // onInitialize();
}

void functiontest()
{
    BpJumpHook::modEnabled = 0;
    return;
}

naked void bpJumpHook_proc(void) // Initial load of BP
{
    _asm {
        cmp [BpJumpHook::modEnabled], 0
        je code

        // reset random BP
        pushad
        pushfd
        call reset_random_bp
        popfd
        popad

        // get area jump pointer
        push edx
        lea edx, [eax+6Ch]
        mov [areaJumpPtr], edx
        pop edx

        // get randomized bp floor
        pushad
        pushfd
        call get_random_bp_floor
        popfd
        popad

        // write new floor
        push edx
        mov edx, [bpFloor]
        mov [eax+74h], edx
        pop edx
        jmp retcode

    code:
        mov [eax+74h], 00000001
    retcode:
		jmp dword ptr [BpJumpHook::_bpJumpHookContinue]
    }
}

naked void bpJumpHook2_proc(void) // called on most floors
{
    _asm {
        cmp [BpJumpHook::modEnabled], 0
        je code

        // get  randomized bp floor
        pushad
        pushfd
        call get_random_bp_floor
        popfd
        popad

        //write new floor
        push edx
        mov edx, [bpFloor]
        mov [eax+ecx*4h+74h], edx
        pop edx
        jmp retcode

    code:
        add dword ptr [eax+ecx*4h+74h], 01
    retcode:
		jmp dword ptr [BpJumpHook::_bpJumpHook2Continue]
    }
}

naked void bpJumpHook3_proc(void) // called after boss stages
{
    _asm {
        cmp [BpJumpHook::modEnabled], 0
        je code

        // get  randomized bp floor
        pushad
        pushfd
        call get_random_bp_floor
        popfd
        popad

        // write new floor
        push edx
        mov edx, [bpFloor]
        mov [eax+ecx*4h+74h], edx
        pop edx
        jmp retcode

    code:
        mov [eax+ecx*4h+74h], edx
    retcode:
        add dword ptr [ebx+04h], 03
		jmp dword ptr [BpJumpHook::_bpJumpHook3Continue]
    }
}

void BpJumpHook::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x004A974, patch, "\x90\x90\x90", 3);
    }
    else
    {
        patch.revert();
    }
}

std::optional<std::string> BpJumpHook::onInitialize()
{
    if (!install_hook_offset(0x4AB8FA, hook, &bpJumpHook_proc, &BpJumpHook::_bpJumpHookContinue, 7))
    {
        HL_LOG_ERR("Failed to init BpJumpHook mod\n");
        return "Failed to init BpJumpHook mod";
    }
    if (!install_hook_offset(0x04C200, hook2, &bpJumpHook2_proc, &BpJumpHook::_bpJumpHook2Continue, 5))
    {
        HL_LOG_ERR("Failed to init BpJumpHook mod\n");
        return "Failed to init BpJumpHook2 mod";
    }
    if (!install_hook_offset(0x04B808, hook3, &bpJumpHook3_proc, &BpJumpHook::_bpJumpHook3Continue, 8))
    {
        HL_LOG_ERR("Failed to init BpJumpHook mod\n");
        return "Failed to init BpJumpHook3 mod";
    }

    return Mod::onInitialize();
}

void BpJumpHook::onGUIframe()
{
    if (ImGui::Checkbox("Randomize BP", &modEnabled))
    {
        reset_random_bp();
        toggle(modEnabled);
    }
}

void BpJumpHook::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("randomize_bp").value_or(false);
    toggle(modEnabled);
}

void BpJumpHook::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("randomize_bp", modEnabled);
}
