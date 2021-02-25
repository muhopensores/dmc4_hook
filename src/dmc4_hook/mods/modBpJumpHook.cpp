#include "../mods.h"
#include "modBpJumpHook.hpp"
#include "modAreaJump.hpp"
#include "modBpBossRush.hpp"
#include "random"
#include "numeric"
// #include "algorithm"
// #include "array"

bool BpJumpHook::modEnabled{ false };
uintptr_t BpJumpHook::_bpJumpHookContinue{ NULL };
uintptr_t BpJumpHook::_bpJumpHook2Continue{ NULL };
int bpFloor = 0;
int areaJumpID = 0;
int numberOfCompleteFloors = 0;
std::array<uint32_t, 100> bpArray;

BpJumpHook::BpJumpHook()
{
    // onInitialize();
}

int bpStageArea(int floor) 
{
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

int jumpToStage(int stage)
{
    switch (stage)
    {
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

void randomize_bp_floors()
{
    numberOfCompleteFloors = 0;
    std::iota(bpArray.begin(), bpArray.end(), 2); // fill the list starting from 2
    std::random_device rd;                        // random device for shuffle
    std::mt19937 g(rd());
    std::shuffle(bpArray.begin(), bpArray.end(), g);
}

void bp_start() // first bp detour
{
    // randomize bp floors
    randomize_bp_floors();

    // apply floor
    bpFloor = 1;

    // set area jump
    areaJumpID = 705;
}

void bp_continue() // other bp detours
{
    if (numberOfCompleteFloors == 100) // + 1
    {
        bpFloor = 101;
        areaJumpID = -1;
        return;
    }

    // apply floor
    bpFloor = bpArray[numberOfCompleteFloors];

    // set area jump
    areaJumpID = jumpToStage(bpStageArea(bpFloor));

    // inc number of complete floors
    numberOfCompleteFloors++;
}

naked void bpJumpHook_proc(void) // Initial load of BP
{
    _asm {
        cmp [BpJumpHook::modEnabled], 0
        je code

        // create randomized numbers
        pushad
        pushfd
        call bp_start
        popfd
        popad

        // write floor and area ID
        push edx
        mov edx, [bpFloor]
        mov [eax+74h], edx
        mov edx, [areaJumpID]
        mov [eax+6Ch], edx
        pop edx
        jmp retcode

    code:
        mov [eax+74h], 00000001
    retcode:
		jmp dword ptr [BpJumpHook::_bpJumpHookContinue]
    }
}

naked void bpJumpHook2_proc(void) // called every time you enter a teleporter
{
    _asm {
        cmp [BpJumpHook::modEnabled], 0
        je code

        // update floor and area ID values
        pushad
        pushfd
        call bp_continue
        popfd
        popad

        // write floor and area ID
        push edx
        mov edx, [bpFloor]
        mov [ecx+74h], edx
        mov edx, [areaJumpID]
        mov [ecx+6Ch], edx
        pop edx
        jmp retcode

    code:
        mov [ecx+6Ch], edx
    retcode:
        mov edx, [ebx+04h]
		jmp dword ptr [BpJumpHook::_bpJumpHook2Continue]
    }
}

void BpJumpHook::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x04B808, patch, "\x90\x90\x90\x90", 4);
        install_patch_offset(0x04C200, patch2, "\x90\x90\x90\x90\x90", 5);
    }
    else
    {
        patch.revert();
        patch2.revert();
    }
}

std::optional<std::string> BpJumpHook::onInitialize()
{
    if (!install_hook_offset(0x4AB8FA, hook, &bpJumpHook_proc, &BpJumpHook::_bpJumpHookContinue, 7))
    {
        HL_LOG_ERR("Failed to init BpJumpHook mod\n");
        return "Failed to init BpJumpHook mod";
    }
    if (!install_hook_offset(0x04A974, hook2, &bpJumpHook2_proc, &BpJumpHook::_bpJumpHook2Continue, 6))
    {
        HL_LOG_ERR("Failed to init BpJumpHook mod\n");
        return "Failed to init BpJumpHook2 mod";
    }
    return Mod::onInitialize();
}

void BpJumpHook::onGUIframe()
{
    if (ImGui::Checkbox("Randomize BP", &modEnabled))
    {
        toggle(modEnabled);
        bp_start();
        BpBossRush::modEnabled = 0;
    }
    ImGui::SameLine(0, 1);
    HelpMarker("Enable before starting BP");

    if (ImGui::CollapsingHeader("[spoilers] What stages did you roll?"))
    {
        ImGui::Text("Stage Number");
        ImGui::SameLine(205);
        ImGui::Text("Random Stage");

        ImGui::Text("1");
        ImGui::SameLine(205);
        ImGui::Text("1");

        int stageCount = 2; 
        for (int i : bpArray)
        {
            ImGui::Text("%i",stageCount);
            stageCount++;
            ImGui::SameLine(205);
            ImGui::Text("%i", i);
        }
        ImGui::Separator();
    }
}

void BpJumpHook::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("randomize_bp").value_or(false);
    toggle(modEnabled);
    if (modEnabled)
    {
        BpBossRush::modEnabled = false;
    }
}

void BpJumpHook::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("randomize_bp", modEnabled);
}
