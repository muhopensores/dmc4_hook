#include "../mods.h"
//#include "../sdk/ReClass_Internal.hpp"

#include "modEnemySpawn.hpp"
#include "modAreaJump.hpp" // for cAreaJumpPtr

// 00738AA2 calls spawns
// scarecrow arm can be spawned via twitch chat with "\SpawnScarecrowArm"

// TODO(): move this somewhere maybe
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
static sMediator* sMedPtr = nullptr;
static uPlayer* uLocalPlr = nullptr;
static int enemySpawning = 0;
static bool enableTwitchSpecialSpawns = false;

int EnemySpawn::hotkeySpawnModifier{ NULL };

int EnemySpawn::hotkeySpawnScarecrowArm{ NULL };
int EnemySpawn::hotkeySpawnScarecrowLeg{ NULL };
int EnemySpawn::hotkeySpawnMega{ NULL };
int EnemySpawn::hotkeySpawnFrost{ NULL };
int EnemySpawn::hotkeySpawnAssault{ NULL };
int EnemySpawn::hotkeySpawnBlitz{ NULL };
int EnemySpawn::hotkeySpawnBasilisk{ NULL };
int EnemySpawn::hotkeySpawnChimera{ NULL };
int EnemySpawn::hotkeySpawnMephisto{ NULL };
int EnemySpawn::hotkeySpawnFaust{ NULL };
int EnemySpawn::hotkeySpawnBianco{ NULL };
int EnemySpawn::hotkeySpawnAlto{ NULL };

constexpr std::array<uintptr_t, 21> fptrEmFactories{
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
    0x007022F0, 		// Sanctus Diabolica bo11
    0x007BF980,         // Dante bo12                   
    0x00723C00          // Kyrie
};

constexpr std::array<const char*, 21> enemyNames{
	"Scarecrow (Arm)",	 // em01 //  0
	"Scarecrow (Leg)",	 // em02 //  1
	"Mega Scarecrow",	 // em03 //  2
	"Frost",			 // em04 //  3
	"Assault",			 // em05 //  4
	"Blitz",			 // em06 //  5
	// "Gladius",		 // em07
	// "Cutlass",		 // em08
	"Basilisk",			 // em09 //  6
	"Chimera Seed",		 // em10 //  7
	// Chimera			 // em11
    "Mephisto",			 // em12 //  8
    "Faust",			 // em13 //  9
	"Bianco Angelo",	 // em14 // 10
	"Alto Angelo",		 // em15 // 11
	// Fault			 // em16
//------------------------------------
	"Berial",			 // bo01 // 12
	"Bael",				 // bo02 // 13
	// Dagon			 // bo03
	"Echidna",			 // bo04 // 14
	// Agnus			 // bo05
	"Angelo Agnus",		 // bo06 // 15
	"Angelo Credo",		 // bo07 // 16
	// The Savior		 // bo08
	// The False Savior	 // bo09
	"Sanctus",			 // bo10 // 17
	"Sanctus Diabolica", // bo11 // 18
    "Dante",             // bo12 // 19
    "Kyrie"              // kyrie// 20
};

constexpr std::array<int, 21> EnemySpawnType{
    4,  // "Scarecrow Arm"     // em01 //  0
    4,  // "Scarecrow Leg"     // em02 //  1
    4,  // "Mega Scarecrow",   // em03 //  2
    4,  // "Frost",            // em04 //  3
    0,  // "Assault",          // em05 //  4
    4,  // "Blitz",            // em06 //  5
        // "Gladius",		   // em07
        // "Cutlass",		   // em08
    3,  // "Basilisk",         // em09 //  6
    3,  // "Chimera Seed",     // em10 //  7 // requires different spawn
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
    4,  // "Sanctus Diabolica" // bo11 // 18
    4,  // "Dante"			   // bo12
    4   // "Kyrie"
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

void setEnemyPosition(uEnemySomething* em) {
	em->mSpawnCoords = getPlayerPosition() + glm::vec3{ 0.0f, 300.0f, 0.0f };
	//em->mEnemySpawnEffectSomething = 4;
    em->mEnemySpawnEffectSomething = EnemySpawnType[(enemySpawning)];
}

/*
scarecrow:
2 scarecrow built by ants (also for mega), looks weird coming from the air
3 scarecrows fall out of horizontal portal while lying down
4 scarecrows fall out of horizontal portal
5 broken on most, T POSE BABYYYYY

frost:
2 frost spawns in ice
3 frost jump far horizontally
4 frost falls, looks good

blitz:
1 blitz spawns under player explosion
2 blitz spawns on player explosion
4 blitz spawns teleporting around

mephisto/faust:
2 makes mephisto+faust spawn down, usually through the floor
3 mephisto faust spawns and goes up
4 mephisto faust spawns and goes up

assault:
0 assualt spawns under the player
2 assault spawns through wall portal
3 assault do tiny jump towards camera
4 assault long jump

armour:
1 bianco spawns through horizontal portal
4 bianco spawns through vertical portal

basilisk:
2 basilisk hops forward from air
3 basilisk horizontal portal ground
4 basilisk spawns attacking random spot

seed:
2 chimera seed spawns in seed and falls from much higher up
3 chimera seed spawns in seed and falls
*/

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
    m_spawnBlitzCommand = std::hash<std::string>{}("\\SpawnBlitz");
    m_spawnBasiliskCommand = std::hash<std::string>{}("\\SpawnBasilisk");
    m_spawnChimeraCommand = std::hash<std::string>{}("\\SpawnChimera");
	m_spawnMephistoCommand = std::hash<std::string>{}("\\SpawnMephisto");
    m_spawnFaustCommand = std::hash<std::string>{}("\\SpawnFaust");
    m_spawnBiancoCommand = std::hash<std::string>{}("\\SpawnBianco");
    m_spawnAltoCommand = std::hash<std::string>{}("\\SpawnAlto");
    m_spawnBerialCommand = std::hash<std::string>{}("\\SpawnBerial");
    m_spawnBaelCommand = std::hash<std::string>{}("\\SpawnBael");
    m_spawnEchidnaCommand = std::hash<std::string>{}("\\SpawnEchidna");
    m_spawnAgnusCommand = std::hash<std::string>{}("\\SpawnAgnus");
    m_spawnCredoCommand = std::hash<std::string>{}("\\SpawnCredo");
    m_spawnSanctusDiaCommand = std::hash<std::string>{}("\\SpawnSanctus");
    m_spawnDanteCommand = std::hash<std::string>{}("\\SpawnDante");
    m_spawnKyrieCommand = std::hash<std::string>{}("\\SpawnKyrie");

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
	if (ImGui::ListBox("##Enemy Spawn Listbox", &enemyNames_current, enemyNames.data(), enemyNames.size(), 21))
    {
            spawnEm00x(enemyNames_current);
    }

    ImGui::Checkbox("Twitch Can Spawn Special Enemies", &enableTwitchSpecialSpawns);
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
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBlitzCommand);
    if (hash == m_spawnBlitzCommand)
    {
        spawnEm00x(5);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBasiliskCommand);
    if (hash == m_spawnBasiliskCommand)
    {
        spawnEm00x(6);
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
    if (enableTwitchSpecialSpawns) {
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBerialCommand);
        if (hash == m_spawnBerialCommand)
        {
            spawnEm00x(12);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBaelCommand);
        if (hash == m_spawnBaelCommand)
        {
            spawnEm00x(13);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnEchidnaCommand);
        if (hash == m_spawnEchidnaCommand)
        {
            spawnEm00x(14);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAgnusCommand);
        if (hash == m_spawnAgnusCommand)
        {
            spawnEm00x(15);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnCredoCommand);
        if (hash == m_spawnCredoCommand)
        {
            spawnEm00x(16);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnSanctusDiaCommand);
        if (hash == m_spawnSanctusDiaCommand)
        {
            spawnEm00x(18); // skipping non diabolica sanctus
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnDanteCommand);
        if (hash == m_spawnDanteCommand)
        {
            spawnEm00x(19);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnKyrieCommand);
        if (hash == m_spawnKyrieCommand)
        {
            spawnEm00x(20);
        }
    }
}

void EnemySpawn::onConfigLoad(const utils::Config& cfg)
{
    hotkeySpawnModifier = cfg.get<int>("hotkey_spawn_modifier").value_or(0x11); // Ctrl

    hotkeySpawnScarecrowArm = cfg.get<int>("hotkey_spawn_scarecrow_arm").value_or(0x70); // F1
    hotkeySpawnScarecrowLeg = cfg.get<int>("hotkey_spawn_scarecrow_leg").value_or(0x71); // F2
    hotkeySpawnMega = cfg.get<int>("hotkey_spawn_mega").value_or(0x72);                  // F3
    hotkeySpawnFrost = cfg.get<int>("hotkey_spawn_frost").value_or(0x73);                // F4
    hotkeySpawnAssault = cfg.get<int>("hotkey_spawn_assault").value_or(0x74);            // F5
    hotkeySpawnBlitz = cfg.get<int>("hotkey_spawn_blitz").value_or(0x75);                // F6
    hotkeySpawnBasilisk = cfg.get<int>("hotkey_spawn_basilisk").value_or(0x76);          // F7
    hotkeySpawnChimera = cfg.get<int>("hotkey_spawn_chimera").value_or(0x77);            // F8
    hotkeySpawnMephisto = cfg.get<int>("hotkey_spawn_mephisto").value_or(0x78);          // F9
    hotkeySpawnFaust = cfg.get<int>("hotkey_spawn_faust").value_or(0x79);                // F10
    hotkeySpawnBianco = cfg.get<int>("hotkey_spawn_bianco").value_or(0x7A);              // F11
    hotkeySpawnAlto = cfg.get<int>("hotkey_spawn_alto").value_or(0x7B);                  // F12

    enableTwitchSpecialSpawns = cfg.get<bool>("enable_twitch_special_spawns").value_or(true);
};

void EnemySpawn::onConfigSave(utils::Config& cfg)
{
    cfg.set<int>("hotkey_spawn_modifier", hotkeySpawnModifier);

    cfg.set<int>("hotkey_spawn_scarecrow_arm", hotkeySpawnScarecrowArm);
    cfg.set<int>("hotkey_spawn_scarecrow_leg", hotkeySpawnScarecrowLeg);
    cfg.set<int>("hotkey_spawn_mega", hotkeySpawnMega);
    cfg.set<int>("hotkey_spawn_frost", hotkeySpawnFrost);
    cfg.set<int>("hotkey_spawn_assault", hotkeySpawnAssault);
    cfg.set<int>("hotkey_spawn_blitz", hotkeySpawnBlitz);
    cfg.set<int>("hotkey_spawn_basilisk", hotkeySpawnBasilisk);
    cfg.set<int>("hotkey_spawn_chimera", hotkeySpawnChimera);
    cfg.set<int>("hotkey_spawn_mephisto", hotkeySpawnMephisto);
    cfg.set<int>("hotkey_spawn_faust", hotkeySpawnFaust);
    cfg.set<int>("hotkey_spawn_bianco", hotkeySpawnBianco);
    cfg.set<int>("hotkey_spawn_alto", hotkeySpawnAlto);

    cfg.set<bool>("enable_twitch_special_spawns", enableTwitchSpecialSpawns);
};

void EnemySpawn::onUpdateInput(hl::Input& input)
{
    if (input.isDown(hotkeySpawnModifier)) {
        if (input.wentDown(hotkeySpawnScarecrowArm))
        {
            spawnEm00x(0);
        }
        if (input.wentDown(hotkeySpawnScarecrowLeg))
        {
            spawnEm00x(1);
        }
        if (input.wentDown(hotkeySpawnMega))
        {
            spawnEm00x(2);
        }
        if (input.wentDown(hotkeySpawnFrost))
        {
            spawnEm00x(3);
        }
        if (input.wentDown(hotkeySpawnAssault))
        {
            spawnEm00x(4);
        }
        if (input.wentDown(hotkeySpawnBlitz))
        {
            spawnEm00x(5);
        }
        if (input.wentDown(hotkeySpawnBasilisk))
        {
            spawnEm00x(6);
        }
        if (input.wentDown(hotkeySpawnChimera))
        {
            spawnEm00x(7);
        }
        if (input.wentDown(hotkeySpawnMephisto))
        {
            spawnEm00x(8);
        }
        if (input.wentDown(hotkeySpawnFaust))
        {
            spawnEm00x(9);
        }
        if (input.wentDown(hotkeySpawnBianco))
        {
            spawnEm00x(10);
        }
        if (input.wentDown(hotkeySpawnAlto))
        {
            spawnEm00x(11);
        }
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