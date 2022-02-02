#include "../mods.h"
//#include "../sdk/ReClass_Internal.hpp"

#include "modEnemySpawn.hpp"
#include "modAreaJump.hpp" // for cAreaJumpPtr

// 00738AA2 calls spawns
// scarecrow arm can be spawned via twitch chat with "\SpawnScarecrowArm"


// TODO(): move this somewhere maybe
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
sMediator* sMedPtr = nullptr;
uPlayer* uLocalPlr = nullptr;

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
	"Scarecrow (Arm)",	// em01 //  0
	"Scarecrow (Leg)",	// em02 //  1
	"Mega Scarecrow",	// em03 //  2
	"Frost",			// em04 //  3
	"Assault",			// em05 //  4
	"Blitz",			// em06 //  5
	// "Gladius",		// em07
	// "Cutlass",		// em08
	"Basilisk",			// em09 //  6
	"Chimera Seed",		// em10 //  7
	// Chimera			// em11
    "Mephisto",			// em12 //  8
    "Faust",			// em13 //  9
	"Bianco Angelo",	// em14 // 10
	"Alto Angelo",		// em15 // 11
	// Fault			// em16
//--------------------------------
	"Berial",			// bo01 // 12
	"Bael",				// bo02 // 13
	// Dagon			// bo03
	"Echidna",			// bo04 // 14
	// Agnus			// bo05
	"Angelo Agnus",		// bo06 // 15
	"Angelo Credo",		// bo07 // 16
	// The Savior		// bo08
	// The False Savior	// bo09
	"Sanctus",			// bo10 // 17
	"Sanctus Diabolica"	// bo11 // 18
	// Dante			// bo12
};

constexpr std::array<int, 19> EnemyAndSpawnType{
    4,  // "Scarecrow Arm"     // em01 //  0
    4,  // "Scarecrow Leg"     // em02 //  1
    4,  // "Mega Scarecrow",   // em03 //  2
    4,  // "Frost",            // em04 //  3
    4,  // "Assault",          // em05 //  4
    4,  // "Blitz",            // em06 //  5
        // "Gladius",		   // em07
        // "Cutlass",		   // em08
    4,  // "Basilisk",         // em09 //  6
    1,  // "Chimera Seed",     // em10 //  7 // requires different spawn
        // "Chimera",		   // em11
    4,  // "Mephisto",         // em12 //  8
    4,  // "Faust",            // em13 //  9
    4,  // "Bianco Angelo",    // em14 // 10
    4,  // "Alto Angelo",      // em15 // 11
        // "Fault",		       // em16
    4,  // "Berial",           // bo01 // 12
    4,  // "Bael",             // bo02 // 13
        // "Dagon",	           // bo03
    4,  // "Echidna",          // bo04 // 14
        // "Agnus",	           // bo05
    4,  // "Angelo Agnus",     // bo06 // 15
    4,  // "Angelo Credo",     // bo07 // 16
        // "The Savior",	   // bo08
        // "The False Savior", // bo09
    4,  // "Sanctus",          // bo10 // 17
    4   // "Sanctus Diabolica" // bo11 // 18
        // "Dante"			   // bo12
};

static uintptr_t fptrUpdateActorList{ 0x008DC540 }; // Spawns shit
static uintptr_t someStruct{ 0x00E552CC };

glm::vec3 getPlayerPosition() {
	// TODO(): move this into somewhere general since
	// this player ptr might be useful in other places
	sMedPtr = (sMediator*)*(uintptr_t*)staticMediatorPtr;
	uLocalPlr = sMedPtr->playerPtr;
	// not sure if this check is needed
	if (uLocalPlr) {
		return uLocalPlr->mPos;
	}
	else {
		return { 0.0f, 0.0f, 0.0f };
	}
}

int enemySpawning = 0;

void setEnemyPosition(uEnemySomething* em) {
	em->mSpawnCoords = getPlayerPosition() + glm::vec3{ 0.0f, 300.0f, 0.0f };
	//em->mEnemySpawnEffectSomething = 4;
    em->mEnemySpawnEffectSomething = EnemyAndSpawnType[(enemySpawning)];
}

void spawnEm00x(int index) {
	uintptr_t emFunctionPointer = fptrEmFactories.at(index);
	__asm {
		pusha
		pushf
		call emFunctionPointer // make actor
        mov esi, eax

        push edx
        mov edx, index
        mov [enemySpawning], edx
        pop edx

		pusha
		push esi
		call setEnemyPosition
		pop esi
		popa
        mov ecx, 0Fh
        mov eax, [someStruct] // static
		mov eax, [eax]
        push 0Fh
        call fptrUpdateActorList
		popf
		popa
	}
}

std::optional<std::string> EnemySpawn::onInitialize() 
{ 
	m_spawnScarecrowArmCommand = std::hash<std::string>{}("\\SpawnScarecrowArm");
    m_spawnScarecrowLegCommand = std::hash<std::string>{}("\\SpawnScarecrowLeg");
	m_spawnMegaCommand = std::hash<std::string>{}("\\SpawnMega");
    m_spawnFrostCommand = std::hash<std::string>{}("\\SpawnFrost");
	m_spawnAssaultCommand = std::hash<std::string>{}("\\SpawnAssault");
    m_spawnBasiliskCommand = std::hash<std::string>{}("\\SpawnBasilisk");
    m_spawnChimeraCommand = std::hash<std::string>{}("\\SpawnChimera");
	m_spawnMephistoCommand = std::hash<std::string>{}("\\SpawnMephisto");
    m_spawnFaustCommand = std::hash<std::string>{}("\\SpawnFaust");
    m_spawnBiancoCommand = std::hash<std::string>{}("\\SpawnBianco");
    m_spawnAltoCommand = std::hash<std::string>{}("\\SpawnAlto");

	return Mod::onInitialize(); 
}

void EnemySpawn::onGUIframe()
{
	if (IsBadWritePtr(AreaJump::cAreaJumpPtr, sizeof(uint32_t)) || IsBadReadPtr(AreaJump::cAreaJumpPtr, sizeof(uint32_t)))
    {
        ImGui::TextWrapped("Enemy Spawner is not initialized.\nLoad into a stage to access it.");
        return;
    }

	ImGui::Text("Enemy Spawner");

	int enemyNames_current = 0;
	if (ImGui::ListBox("##Enemy Spawn Listbox", &enemyNames_current, enemyNames.data(), enemyNames.size(), 19))
    {
            spawnEm00x(enemyNames_current);
    }
}

void EnemySpawn::onTwitchCommand(std::size_t hash)
{
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnScarecrowArmCommand);
    if (hash == m_spawnScarecrowArmCommand)
    {
        spawnEm00x(0);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnScarecrowLegCommand);
    if (hash == m_spawnScarecrowLegCommand)
    {
        spawnEm00x(1);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnMegaCommand);
    if (hash == m_spawnMegaCommand)
    {
        spawnEm00x(2);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnFrostCommand);
    if (hash == m_spawnFrostCommand)
    {
        spawnEm00x(3);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAssaultCommand);
    if (hash == m_spawnAssaultCommand)
    {
        spawnEm00x(4);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBasiliskCommand);
    if (hash == m_spawnBasiliskCommand)
    {
        spawnEm00x(6); // Skipping Blitz
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnChimeraCommand);
    if (hash == m_spawnChimeraCommand)
    {
        spawnEm00x(7);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnMephistoCommand);
    if (hash == m_spawnMephistoCommand)
    {
        spawnEm00x(8);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnFaustCommand);
    if (hash == m_spawnFaustCommand)
    {
        spawnEm00x(9);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBiancoCommand);
    if (hash == m_spawnBiancoCommand)
    {
        spawnEm00x(10);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAltoCommand);
    if (hash == m_spawnAltoCommand)
    {
        spawnEm00x(11);
    }
}

#if 0
[ENABLE]
alloc(emspawn,64,DevilMayCry4_DX9.exe)
createthread(emspawn)
emspawn:
call 0055E710
//does jackshit
/*pushad
fld dword ptr [float1]
fld dword ptr [float2]
fld dword ptr [float3]
popad
pushad
lea eax, [eax+1898]
fstp dword ptr [eax]
lea eax, [eax-04]
fstp dword ptr [eax]
lea eax, [eax-04]
fstp dword ptr [eax]
popad*/
mov esi, eax
mov ecx, 0F
//mov ecx, 126
mov eax,[00E552CC]
push 0F
call 008DC540
ret

[DISABLE]
dealloc(emspawn)
unregistersymbol(emspawn)

[ENABLE]
alloc(emspawn,64,devilmaycry4specialedition.exe)
alloc(emspawndata,64,devilmaycry4specialedition.exe)
emspawndata:
float1:
dd (float)0
float2:
dd (float)0
float3:
dd (float)0

createthread(emspawn)
emspawn:
mov eax,DevilMayCry4SpecialEdition.exe+26E590 // 1
call eax
pushad
fld dword ptr [float1]
fld dword ptr [float2]
fld dword ptr [float3]
popad
pushad
lea eax,[eax+00001898]
fstp dword ptr [eax]
lea eax,[eax-04]
fstp dword ptr [eax]
lea eax,[eax-04]
fstp dword ptr [eax]
popad
mov esi,eax
mov ecx,0000000F
push 00
push 00
push esi
push ecx
mov ecx,[DevilMayCry4SpecialEdition.exe+F240A4] // 2
mov eax,DevilMayCry4SpecialEdition.exe+6C32E0 // 3
call eax
ret

[DISABLE]
dealloc(emspawn)
unregistersymbol(emspawn)
dealloc(emspawndata)
unregistersymbol(emspawndata)
#endif