#include "../mods.h"
//#include "../sdk/ReClass_Internal.hpp"

#include "modEnemySpawn.hpp"
#include "modAreaJump.hpp" // for cAreaJumpPtr


// TODO(): move this somewhere maybe
constexpr uintptr_t staticMediatorPtr = 0x00E558B8;
static uintptr_t fptrUpdateActorList{ 0x008DC540 }; // Spawns shit
static uintptr_t someStruct{ 0x00E552CC };
// DevilMayCry4_DX9.exe+338AA2 // 00738AA2 // calls most enemy spawns

static sMediator* sMedPtr = nullptr;
static uPlayer* uLocalPlr = nullptr;
static int enemySpawning = 0;
static bool enableTwitchSpecialSpawns = false;

int EnemySpawn::hotkeySpawnModifier{ NULL };
int EnemySpawn::hotkeySpawnScarecrowLeg{ NULL };
int EnemySpawn::hotkeySpawnScarecrowArm{ NULL };
int EnemySpawn::hotkeySpawnMega{ NULL };
int EnemySpawn::hotkeySpawnBianco{ NULL };
int EnemySpawn::hotkeySpawnAlto{ NULL };
int EnemySpawn::hotkeySpawnMephisto{ NULL };
int EnemySpawn::hotkeySpawnFaust{ NULL };
int EnemySpawn::hotkeySpawnFrost{ NULL };
int EnemySpawn::hotkeySpawnAssault{ NULL };
int EnemySpawn::hotkeySpawnBlitz{ NULL };
int EnemySpawn::hotkeySpawnChimera{ NULL };
int EnemySpawn::hotkeySpawnBasilisk{ NULL };

// id:         name & notes:                        file spawn bytes
// em000    // Scarecrow (Leg Type)              // B3D5241C
// em001    // Scarecrow (Arm Type)              // 10B4A56C
// em003    // Mega Scarecrow                    // C3D37215
// em005    // Bianco Angelo (White Armor)       // F20A5D0C
// em006    // Alto Angelo (Gold Armor)          // DADC9B79
// em008    // Mephisto                          // 8C051F65
// em009    // Faust                             // 2F7D022E
// em010    // Frost                             // A3218863
// em011    // Assault                           // 3F0CA327
// em012    // Blitz                             // 231B4C1B
// em013    // Chimera                           // E284A500
// em015    // Cutlass                           // 4601B405
// em016    // Gladius                           // 59C0DF2E
// em017    // Basillisk                         // 73B20614
// em018    // Berial                            // 839FE038
// em019    // Bael                              // 1AA82843
// em021    // Echidna                           // 98FCAC35
// em022    // Angelo Credo                      // AD130C4A
// em023    // Angelo Agnus                      // 2AA83A7F
// em025    // Savior                            // 5C0C385E
// em026    // Savior2                           //
// em027    // Credo human form, pl004           //
// em029    // Sanctus, pl002, pl0023            // 62D0876D
// em030    // Sanctus2, pl023                   //
// em031    // Credo human again, pl004          //
// em033    // Savior3                           //
// em035    // style checker, em027, orb005      //
// em036    // Kyrie, pl022                      //
// em_dante // Boss Dante                        // 3D355100

constexpr std::array<uintptr_t, 21> fptrEmFactories {
	0x0053F810,	               // 00 Leg Scarecrow
	0x0055E710,	               // 01 Arm Scarecrow
    0x0055F7E0,	               // 02 Mega Scarecrow
	0x00561A10,	               // 03 Bianco Angelo
    0x00576C80,	               // 04 Alto Angelo
    0x0057F1E0,	               // 05 Mephisto
	0x00595810,	               // 06 Faust
	0x005A3F60,	               // 07 Frost
    0x005B3170,	               // 08 Assault
    0x005D1760,	               // 09 Blitz
	0x005DC160,	               // 10 Chimera Seed
	0x0061A7B0,	               // 11 Basilisk
	0x00630AC0,	               // 12 Berial
	0x00649CB0,	               // 13 Bael
	0x00685340,	               // 14 Echidna
	0x006AA2C0,	               // 16 Angelo Credo
	0x006BDE60,	               // 15 Angelo Agnus
    0x006F81E0,	               // 17 Sanctus
    0x007022F0,                // 18 Sanctus Diabolica
    0x00723C00,                // 19 Kyrie
    0x007BF980                 // 20 Dante                   
    // 0x00609C20,             // // Cutlass // Broken
    // 0x00618460,             // // Gladius // Broken
	//                         // // Fault
	//                         // // The Savior
	//                         // // The False Savior
};

constexpr std::array<const char*, 21> enemyNames {
	"Scarecrow (Leg)",	       // 00 Leg Scarecrow
	"Scarecrow (Arm)",	       // 01 Arm Scarecrow
	"Mega Scarecrow",	       // 02 Mega Scarecrow
	"Bianco Angelo",	       // 03 Bianco Angelo
	"Alto Angelo",		       // 04 Alto Angelo
    "Mephisto",			       // 05 Mephisto
    "Faust",			       // 06 Faust
	"Frost",			       // 07 Frost
	"Assault",			       // 08 Assault
	"Blitz",			       // 09 Blitz
	"Chimera Seed",		       // 10 Chimera Seed
	"Basilisk",			       // 11 Basilisk
	"Berial",			       // 12 Berial
	"Bael",				       // 13 Bael
	"Echidna",			       // 14 Echidna
	"Angelo Credo",		       // 16 Angelo Credo
	"Angelo Agnus",		       // 15 Angelo Agnus
	"Sanctus",			       // 17 Sanctus
	"Sanctus Diabolica",       // 18 Sanctus Diabolica
    "Kyrie",                   // 19 Kyrie
    "Dante"                    // 20 Dante            
	// "Cutlass",		       // // Cutlass // Broken
	// "Gladius",		       // // Gladius // Broken
	// Fault			       // // Fault
	// The Savior		       // // The Savior
	// The False Savior	       // // The False Savior
};

constexpr std::array<int, 21> EnemySpawnType {
    4,                         // 00 Leg Scarecrow
    4,                         // 01 Arm Scarecrow
    4,                         // 02 Mega Scarecrow
    4,                         // 03 Bianco Angelo
    4,                         // 04 Alto Angelo
    4,                         // 05 Mephisto
    4,                         // 06 Faust
    4,                         // 07 Frost
    0,                         // 08 Assault
    4,                         // 09 Blitz
    3,                         // 10 Chimera Seed
    3,                         // 11 Basilisk
    4,                         // 12 Berial
    4,                         // 13 Bael
    4,                         // 14 Echidna
    4,                         // 16 Angelo Credo
    4,                         // 15 Angelo Agnus
    4,                         // 17 Sanctus
    4,                         // 18 Sanctus Diabolica
    4,                         // 19 Kyrie
    4                          // 20 Dante            
    // 4                       // // Cutlass // Broken
    // 4                       // // Gladius // Broken
    // 4                       // // Fault
    // 4                       // // The Savior
    // 4                       // // The False Savior
};

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
    em->mEnemySpawnEffectSomething = EnemySpawnType[(enemySpawning)];
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

std::optional<std::string> EnemySpawn::onInitialize()  { 
    m_spawnScarecrowLegCommand = std::hash<std::string>{}("\\SpawnScarecrowLeg");
	m_spawnScarecrowArmCommand = std::hash<std::string>{}("\\SpawnScarecrowArm");
	m_spawnMegaCommand = std::hash<std::string>{}("\\SpawnMega");
    m_spawnBiancoCommand = std::hash<std::string>{}("\\SpawnBianco");
    m_spawnAltoCommand = std::hash<std::string>{}("\\SpawnAlto");
	m_spawnMephistoCommand = std::hash<std::string>{}("\\SpawnMephisto");
    m_spawnFaustCommand = std::hash<std::string>{}("\\SpawnFaust");
    m_spawnFrostCommand = std::hash<std::string>{}("\\SpawnFrost");
	m_spawnAssaultCommand = std::hash<std::string>{}("\\SpawnAssault");
    m_spawnBlitzCommand = std::hash<std::string>{}("\\SpawnBlitz");
    m_spawnChimeraCommand = std::hash<std::string>{}("\\SpawnChimera");
    m_spawnBasiliskCommand = std::hash<std::string>{}("\\SpawnBasilisk");
    m_spawnBerialCommand = std::hash<std::string>{}("\\SpawnBerial");
    m_spawnBaelCommand = std::hash<std::string>{}("\\SpawnBael");
    m_spawnEchidnaCommand = std::hash<std::string>{}("\\SpawnEchidna");
    m_spawnCredoCommand = std::hash<std::string>{}("\\SpawnCredo");
    m_spawnAgnusCommand = std::hash<std::string>{}("\\SpawnAgnus");
    m_spawnSanctusDiaCommand = std::hash<std::string>{}("\\SpawnSanctus");
    m_spawnKyrieCommand = std::hash<std::string>{}("\\SpawnKyrie");
    m_spawnDanteCommand = std::hash<std::string>{}("\\SpawnDante");
	return Mod::onInitialize(); 
}

void EnemySpawn::onGUIframe() {
	if (IsBadWritePtr(AreaJump::cAreaJumpPtr, sizeof(uint32_t)) || IsBadReadPtr(AreaJump::cAreaJumpPtr, sizeof(uint32_t))) {
        ImGui::TextWrapped("Enemy Spawner is not initialized.\nLoad into a stage to access it.");
        return;
    }
	ImGui::Text("Enemy Spawner");
	int enemyNames_current = 0;
	if (ImGui::ListBox("##Enemy Spawn Listbox", &enemyNames_current, enemyNames.data(), enemyNames.size(), 21)) {
            spawnEm00x(enemyNames_current);
    }
    ImGui::Checkbox("Twitch Can Spawn Special Enemies", &enableTwitchSpecialSpawns);
}

void EnemySpawn::onTwitchCommand(std::size_t hash) {
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnScarecrowLegCommand);
    if (hash == m_spawnScarecrowLegCommand) {
        spawnEm00x(0);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnScarecrowArmCommand);
    if (hash == m_spawnScarecrowArmCommand) {
        spawnEm00x(1);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnMegaCommand);
    if (hash == m_spawnMegaCommand) {
        spawnEm00x(2);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBiancoCommand);
    if (hash == m_spawnBiancoCommand) {
        spawnEm00x(3);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAltoCommand);
    if (hash == m_spawnAltoCommand) {
        spawnEm00x(4);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnMephistoCommand);
    if (hash == m_spawnMephistoCommand) {
        spawnEm00x(5);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnFaustCommand);
    if (hash == m_spawnFaustCommand) {
        spawnEm00x(6);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnFrostCommand);
    if (hash == m_spawnFrostCommand) {
        spawnEm00x(7);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAssaultCommand);
    if (hash == m_spawnAssaultCommand) {
        spawnEm00x(8);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBlitzCommand);
    if (hash == m_spawnBlitzCommand) {
        spawnEm00x(9);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnChimeraCommand);
    if (hash == m_spawnChimeraCommand) {
        spawnEm00x(10);
    }
    HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBasiliskCommand);
    if (hash == m_spawnBasiliskCommand) {
        spawnEm00x(11);
    }
    if (enableTwitchSpecialSpawns) {
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBerialCommand);
        if (hash == m_spawnBerialCommand) {
            spawnEm00x(12);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnBaelCommand);
        if (hash == m_spawnBaelCommand) {
            spawnEm00x(13);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnEchidnaCommand);
        if (hash == m_spawnEchidnaCommand) {
            spawnEm00x(14);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnCredoCommand);
        if (hash == m_spawnCredoCommand) {
            spawnEm00x(15);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnAgnusCommand);
        if (hash == m_spawnAgnusCommand) {
            spawnEm00x(16);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnSanctusDiaCommand);
        if (hash == m_spawnSanctusDiaCommand) {
            spawnEm00x(18); // skipping non diabolica sanctus
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnKyrieCommand);
        if (hash == m_spawnKyrieCommand) {
            spawnEm00x(19);
        }
        HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawnDanteCommand);
        if (hash == m_spawnDanteCommand) {
            spawnEm00x(20);
        }
    }
}

void EnemySpawn::onConfigLoad(const utils::Config& cfg) {
    hotkeySpawnModifier = cfg.get<int>("hotkey_spawn_modifier").value_or(0x11); // Ctrl
    hotkeySpawnScarecrowLeg = cfg.get<int>("hotkey_spawn_scarecrow_leg").value_or(0x70); // F 1
    hotkeySpawnScarecrowArm = cfg.get<int>("hotkey_spawn_scarecrow_arm").value_or(0x71); // F 2
    hotkeySpawnMega = cfg.get<int>("hotkey_spawn_mega").value_or(0x72);                  // F 3
    hotkeySpawnBianco = cfg.get<int>("hotkey_spawn_bianco").value_or(0x73);              // F 4
    hotkeySpawnAlto = cfg.get<int>("hotkey_spawn_alto").value_or(0x74);                  // F 5
    hotkeySpawnMephisto = cfg.get<int>("hotkey_spawn_mephisto").value_or(0x75);          // F 6
    hotkeySpawnFaust = cfg.get<int>("hotkey_spawn_faust").value_or(0x76);                // F 7
    hotkeySpawnFrost = cfg.get<int>("hotkey_spawn_frost").value_or(0x77);                // F 8
    hotkeySpawnAssault = cfg.get<int>("hotkey_spawn_assault").value_or(0x78);            // F 9
    hotkeySpawnBlitz = cfg.get<int>("hotkey_spawn_blitz").value_or(0x79);                // F10
    hotkeySpawnChimera = cfg.get<int>("hotkey_spawn_chimera").value_or(0x7A);            // F11
    hotkeySpawnBasilisk = cfg.get<int>("hotkey_spawn_basilisk").value_or(0x7B);          // F12
    enableTwitchSpecialSpawns = cfg.get<bool>("enable_twitch_special_spawns").value_or(true);
};

void EnemySpawn::onConfigSave(utils::Config& cfg) {
    cfg.set<int>("hotkey_spawn_modifier", hotkeySpawnModifier);
    cfg.set<int>("hotkey_spawn_scarecrow_leg", hotkeySpawnScarecrowLeg);
    cfg.set<int>("hotkey_spawn_scarecrow_arm", hotkeySpawnScarecrowArm);
    cfg.set<int>("hotkey_spawn_mega", hotkeySpawnMega);
    cfg.set<int>("hotkey_spawn_bianco", hotkeySpawnBianco);
    cfg.set<int>("hotkey_spawn_alto", hotkeySpawnAlto);
    cfg.set<int>("hotkey_spawn_mephisto", hotkeySpawnMephisto);
    cfg.set<int>("hotkey_spawn_faust", hotkeySpawnFaust);
    cfg.set<int>("hotkey_spawn_frost", hotkeySpawnFrost);
    cfg.set<int>("hotkey_spawn_assault", hotkeySpawnAssault);
    cfg.set<int>("hotkey_spawn_blitz", hotkeySpawnBlitz);
    cfg.set<int>("hotkey_spawn_chimera", hotkeySpawnChimera);
    cfg.set<int>("hotkey_spawn_basilisk", hotkeySpawnBasilisk);
    cfg.set<bool>("enable_twitch_special_spawns", enableTwitchSpecialSpawns);
}

void EnemySpawn::onUpdateInput(hl::Input& input) {
    if (input.isDown(hotkeySpawnModifier)) {
        if (input.wentDown(hotkeySpawnScarecrowLeg)) {
            spawnEm00x(0);
        }
        if (input.wentDown(hotkeySpawnScarecrowArm)) {
            spawnEm00x(1);
        }
        if (input.wentDown(hotkeySpawnMega)) {
            spawnEm00x(2);
        }
        if (input.wentDown(hotkeySpawnBianco)) {
            spawnEm00x(3);
        }
        if (input.wentDown(hotkeySpawnAlto)) {
            spawnEm00x(4);
        }
        if (input.wentDown(hotkeySpawnMephisto)) {
            spawnEm00x(5);
        }
        if (input.wentDown(hotkeySpawnFaust)) {
            spawnEm00x(6);
        }
        if (input.wentDown(hotkeySpawnFrost)) {
            spawnEm00x(7);
        }
        if (input.wentDown(hotkeySpawnAssault)) {
            spawnEm00x(8);
        }
        if (input.wentDown(hotkeySpawnBlitz)) {
            spawnEm00x(9);
        }
        if (input.wentDown(hotkeySpawnChimera)) {
            spawnEm00x(10);
        }
        if (input.wentDown(hotkeySpawnBasilisk)) {
            spawnEm00x(11);
        }
    }
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
