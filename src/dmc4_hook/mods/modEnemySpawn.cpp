
#include "../mods.h"
#include "modEnemySpawn.hpp"

constexpr std::array<uintptr_t, 5> fptrEmFactories{
	0x0055E710, // arm crow call 0055E710
	0x0053F810, // leg crow call 0053F810
	0x005A3F60, // frost call 005A3F60
	0x00561A10, // white armours call 00561A10
	0x0057F1E0, // ghosties call 0057F1E0
};

constexpr std::array<const char*, 5> enemyNames{
	"Spawn em001",
	"Spawn em002",
	"Spawn em003",
	"Spawn em004",
	"Spawn em005"
};

static uintptr_t fptrUpdateActorList{ 0x008DC540 }; // Spawns shit
static uintptr_t someStruct{ 0x00E552CC };

void spawnEm00x(int index) {
	uintptr_t emFunctionPointer = fptrEmFactories.at(index);
	__asm {
		pusha
		pushf
		call emFunctionPointer // make actor
        mov esi, eax
        mov ecx, 0Fh
        mov eax, [someStruct] // static
		mov eax, [eax]
        push 0Fh
        call fptrUpdateActorList
		popf
		popa
	}
}
#if 0
naked void enemySpawn_proc(void)
{
    _asm {
		cmp byte ptr [EnemySpawn::modEnabled], 0
		je code
        ret // ?

    code:
        call 0055E710h //Scarecrow Arm
        //does jackshit
        //pushad
        //fld dword ptr [float1]
        //fld dword ptr [float2]
        //fld dword ptr [float3]
        //popad
        //pushad
        //lea eax, [eax+1898]
        //fstp dword ptr [eax]
        //lea eax, [eax-04]
        //fstp dword ptr [eax]
        //lea eax, [eax-04]
        //fstp dword ptr [eax]
        //popad
        mov esi, eax
        mov ecx, 0Fh
        //mov ecx, 126
        mov eax, [00E552CCh]
        push 0Fh
        call 008DC540h
        ret
    }
}
#endif


std::optional<std::string> EnemySpawn::onInitialize() 
{ 
	return Mod::onInitialize(); 
}

void EnemySpawn::onGUIframe()
{
	/*auto lambda = [](int i) {
		if (ImGui::Button(enemyNames[i])) {
			spawnEm00x(i);
		}
	};
	std::for_each(enemyNames.begin(), enemyNames.end(), lambda);*/

	ImGui::Text("Enemy Spawn");
	// will mr.compiler unroll this? probably not check the .ASM listings im not gonna
		for (int i = 0, e = enemyNames.size(); i != e; i++) {
			if (ImGui::Button(enemyNames[i])) {
				spawnEm00x(i);
			}
			if (i % 2 == 0) {
				ImGui::SameLine();
			}
		}
	ImGui::NewLine();
	ImGui::Separator();
}

