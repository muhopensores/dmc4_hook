
#include "../mods.h"
#include "modEnemySpawn.hpp"
// 00738AA2 calls spawns
constexpr std::array<uintptr_t, 19> fptrEmFactories{
	0x0055E710,			// Arm Scarecrow em01
	0x0053F810,			// Leg Scarecrow em02
    0x0055F7E0,			// Mega Scarecrow em03
	0x005A3F60,			// Frost em04
    0x005B3170,			// Assault em05
    0x005D1760,			// Blitz em06
    // 0x00618460,		// Gladius em07 // Broken
    // 0x00609C20,		// Cutlass em08 // Broken
	0x0061A7B0,			// Basilisk em09
	0x005DC160,			// Chimera Seed em10
						// Chimera
    0x0057F1E0,			// Mephisto em12
	0x00595810,			// Faust em13
	0x00561A10,			// Bianco Angelo em14
    0x00576C80,			// Alto Angelo em15
						// Fault em16
//--------------------------------
	0x00630AC0,			// Berial bo01
	0x00649CB0,			// BP Frog bo02
						// The Other Frog bo03
	0x00685340,			// Echidna bo04
						// Agnus bo05
	0x006BDE60,			// Angelo Agnus bo06
	0x006AA2C0,			// Angelo Credo bo07
						// The Savior bo08
						// The False Savior bo09
    0x006F81E0,			// Sanctus bo10
    0x007022F0			// Sanctus Diabolica bo11
						// Dante bo12
};



constexpr std::array<const char*, 19> enemyNames{
	"Scarecrow (Arm)",	// em01
	"Scarecrow (Leg)",	// em02
	"Mega Scarecrow",	// em03
	"Frost",			// em04
	"Assault",			// em05
	"Blitz",			// em06
	// "Gladius",		// em07
	// "Cutlass",		// em08
	"Basilisk",			// em09
	"Chimera Seed",		// em10
	// Chimera			// em11
    "Mephisto",			// em12
    "Faust",			// em13
	"Bianco Angelo",	// em14
	"Alto Angelo",		// em15
	// Fault			// em16
//--------------------------------
	"Berial",			// bo01
	"Bael",				// bo02
	// Dagon			// bo03
	"Echidna",			// bo04
	// Agnus			// bo05
	"Angelo Agnus",		// bo06
	"Angelo Credo",		// bo07
	// The Savior		// bo08
	// The False Savior	// bo09
	"Sanctus",			// bo10
	"Sanctus Diabolica"	// bo11
	// Dante			// bo12
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

