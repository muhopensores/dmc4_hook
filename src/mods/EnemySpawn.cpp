#include "EnemySpawn.hpp"
#include "../sdk/Devil4.hpp"

static uintptr_t some_struct = 0x00E552CC;
static uintptr_t fptr_update_actor_list = 0x008DC540; // Spawns shit
// DevilMayCry4_DX9.exe+338AA2 // 00738AA2 // calls most enemy spawns

static SMediator* s_med_ptr             = nullptr;
static uPlayer* u_local_plr             = nullptr;
static int enemy_spawning               = 0;
static std::mutex g_mutex;

enum {
    HOTKEY_SPAWN_SCARECROW_LEG,
    HOTKEY_SPAWN_SCARECROW_ARM,
    HOTKEY_SPAWN_MEGA,
    HOTKEY_SPAWN_BIANCO,
    HOTKEY_SPAWN_ALTO,
    HOTKEY_SPAWN_MEPHISTO,
    HOTKEY_SPAWN_FAUST,
    HOTKEY_SPAWN_FROST,
    HOTKEY_SPAWN_ASSAULT,
    HOTKEY_SPAWN_BLITZ,
    HOTKEY_SPAWN_CHIMERA,
    HOTKEY_SPAWN_GLADIUS,
    HOTKEY_SPAWN_CUTLASS,
    HOTKEY_SPAWN_BASILISK,
    HOTKEY_MAX
};

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
// em027    // Credo human form, pl004, dh says agnus anims // 
// em029    // Sanctus, pl002, pl0023            // 62D0876D
// em030    // Sanctus2, pl023                   //
// em031    // Credo human again, pl004          //
// em033    // Savior3                           //
// em035    // style checker, em027, orb005      //
// em036    // Kyrie, pl022                      //
// em_dante // Boss Dante                        // 3D355100

constexpr std::array<uintptr_t, 23> fptr_em_factories{
    0x0053F810, // 00 Leg Scarecrow
    0x0055E710, // 01 Arm Scarecrow
    0x0055F7E0, // 02 Mega Scarecrow
    0x00561A10, // 03 Bianco Angelo
    0x00576C80, // 04 Alto Angelo
    0x0057F1E0, // 05 Mephisto
    0x00595810, // 06 Faust
    0x005A3F60, // 07 Frost
    0x005B3170, // 08 Assault
    0x005D1760, // 09 Blitz
    0x005DC160, // 10 Chimera Seed
    0x005F37F0, // 11 Cutlass
    0x0060AFC0, // 12 Gladius
    0x0061A7B0, // 13 Basilisk
    0x00630AC0, // 14 Berial
    0x00649CB0, // 15 Bael
    0x00685340, // 16 Echidna
    0x006AA2C0, // 17 Angelo Credo
    0x006BDE60, // 18 Angelo Agnus
    0x006F81E0, // 19 Sanctus
    0x007022F0, // 20 Sanctus Diabolica
    0x00723C00, // 21 Kyrie
    0x007BF980, // 22 Dante
    // Fault
    // The Savior
};

constexpr std::array<const char*, 23> enemy_names{
    __("Scarecrow (Leg)"),   // 00 Leg Scarecrow
    __("Scarecrow (Arm)"),   // 01 Arm Scarecrow
    __("Mega Scarecrow"),    // 02 Mega Scarecrow
    __("Bianco Angelo"),     // 03 Bianco Angelo
    __("Alto Angelo"),       // 04 Alto Angelo
    __("Mephisto"),          // 05 Mephisto
    __("Faust"),             // 06 Faust
    __("Frost"),             // 07 Frost
    __("Assault"),           // 08 Assault
    __("Blitz"),             // 09 Blitz
    __("Chimera Seed"),      // 10 Chimera Seed
    __("Cutlass"),		     // 11 Cutlass
    __("Gladius"), 		     // 12 Gladius
    __("Basilisk"),          // 13 Basilisk
    __("Berial"),            // 14 Berial
    __("Bael"),              // 15 Bael
    __("Echidna"),           // 16 Echidna
    __("Angelo Credo"),      // 17 Angelo Credo
    __("Angelo Agnus"),      // 18 Angelo Agnus
    __("Sanctus"),           // 19 Sanctus
    __("Sanctus Diabolica"), // 20 Sanctus Diabolica
    __("Kyrie"),             // 21 Kyrie
    __("Dante"),             // 22 Dante
                             // Fault
                             // The Savior
                             // The False Savior
};

constexpr std::array<int, 23> enemy_spawn_type{
    4, // 00 Leg Scarecrow
    4, // 01 Arm Scarecrow
    4, // 02 Mega Scarecrow
    4, // 03 Bianco Angelo
    4, // 04 Alto Angelo
    4, // 05 Mephisto
    4, // 06 Faust
    4, // 07 Frost
    0, // 08 Assault
    4, // 09 Blitz
    2, // 10 Chimera Seed
    2, // 11 Cutlass
    8, // 12 Gladius
    3, // 13 Basilisk
    4, // 14 Berial
    0, // 15 Bael
    4, // 16 Echidna
    4, // 17 Angelo Credo
    4, // 18 Angelo Agnus
    4, // 19 Sanctus
    4, // 20 Sanctus Diabolica
    4, // 21 Kyrie
    4, // 22 Dante
    // 4 // Fault
    // 4 // The Savior
};

/*
spawn animations:
if not listed, 4 was used
5 is broken on most, T POSE BABYYYYY

berial:
0 spawns doing horizontal swipe
1 same thing

scarecrow:
2 built by ants, looks bad coming from the air
3 fall out of horizontal portal while lying down
4 fall out of horizontal portal

armour:
1 horizontal portal
4 vertical portal

mephisto/faust:
2 down, usually through the floor
3 goes up
4 goes up

frost:
2 frost spawns in ice
3 frost jump far horizontally
4 frost falls, looks good

assault:
0 under the player
1 long jump
2 through wall portal
3 do tiny jump towards camera
4 long jump

blitz:
1 under player explosion
2 on player explosion
4 teleporting around

chimera:
2 in seed, falls from much higher up
3 in seed, falls

basilisk:
2 hops forward from air
3 horizontal portal ground
4 attacking random spot

cutlass:
2 spawns on the ground

gladius:
1 makes sound and spawn visual
2 spawns shooting forward but kinda sucks
3 spawns charging vertical spin
4 spawns charging horizontally spin
5 spawns charing upwards attack
6 spawns with instant attack
7 spawns being stabbed into the ground
8 spawns slowly, comes out facing up, v clean

*/

glm::vec3 get_player_position() {
    u_local_plr = devil4_sdk::get_local_player();
    // not sure if this check is needed
    if (u_local_plr) {
        return u_local_plr->m_pos;
    } 
    return {0.0f, 0.0f, 0.0f};
}

void set_enemy_position(UEnemySomething* em) {
    em->m_spawn_coords               = get_player_position() + glm::vec3{0.0f, 300.0f, 0.0f};
    em->m_enemy_spawn_effect_something = enemy_spawn_type[(enemy_spawning)];
}

void EnemySpawn::spawn_em00x(EnemyType index) {
    std::lock_guard<std::mutex> lk(g_mutex);
    uintptr_t em_function_pointer = fptr_em_factories.at((int)index);
    if (!devil4_sdk::get_local_player()) return; // only work while character is loaded
    __asm {
		pushad
		pushfd
		call em_function_pointer // make actor
        mov esi, eax

        push edx
        mov edx, index
        mov [enemy_spawning], edx
        pop edx

		pushad
		push esi
		call set_enemy_position
		pop esi
		popad

        mov ecx, 0x0F
        mov eax, [some_struct] // static
		mov eax, [eax]
        push 0x0F
        call fptr_update_actor_list
		popfd
		popad
    }
}

void spawn_custom(uintptr_t spawnAddr) {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!devil4_sdk::get_local_player()) return; // only work while character is loaded
    __asm {
		pushad
		pushfd
		call spawnAddr // make actor
        mov esi, eax

        push edx
        mov edx, 0
        mov [enemy_spawning], edx
        pop edx

		// pushad
		// push esi
		// call set_enemy_position
		// pop esi
		// popad

        mov ecx, 0x0F
        mov eax, [some_struct] // static
		mov eax, [eax]
        push 0x0F
        call fptr_update_actor_list
		popfd
		popad
    }
}

void EnemySpawn::spawn_random_enemy() {
    auto now = std::chrono::system_clock::now();
    srand((uint32_t)now.time_since_epoch().count());
    // I thought this would stop crashes but no
    // if (devil4_sdk::get_sMediator()->uBoss1)
    //    spawn_em00x(rand() % 11); // do not spawn a boss if a boss already exists
    // else
    spawn_em00x((EnemyType)(rand() % 23));
}

std::optional<std::string> EnemySpawn::on_initialize() {
    //srand((uint32_t)this);
    MutatorRegistry::define("SpawnScarecrowLeg").weight(8)
        .description("Spawns ScarecrowLeg").alias("Leg")
        .on_init([&]() { spawn_em00x(EnemyType::SCARECROW_LEG); });

    MutatorRegistry::define("SpawnScarecrowArm").weight(7)
        .description("Spawns ScarecrowArm").alias("Arm")
        .on_init([&]() { spawn_em00x(EnemyType::SCARECROW_ARM); });

    MutatorRegistry::define("SpawnMega").weight(6)
        .description("Spawns a Mega").alias("Mega")
        .on_init([&]() { spawn_em00x(EnemyType::SCARECROW_MEGA); });

    MutatorRegistry::define("SpawnBianco").weight(10)
        .description("Spawns Bianco Angelo").alias("Bianco").alias("BA")
        .on_init([&]() { spawn_em00x(EnemyType::ANGELO_BIANCO); });

    MutatorRegistry::define("SpawnAlto").weight(10)
        .description("Spawns Alto Angelo").alias("Alto").alias("AA")
        .on_init([&]() { spawn_em00x(EnemyType::ANGELO_ALTO); });

    MutatorRegistry::define("SpawnMephisto").weight(10)
        .description("Spawns Mephisto").alias("Mephisto").alias("FlyingRat")
        .on_init([&]() { spawn_em00x(EnemyType::MEPHISTO); });

    MutatorRegistry::define("SpawnFaust").weight(10)
        .description("Spawns Faust").alias("FlyingRatDaddy").alias("EdwardScissorhands")
        .on_init([&]() { spawn_em00x(EnemyType::FAUST); });

    MutatorRegistry::define("SpawnFrost").weight(10)
        .description("Spawns Frost").alias("Frost").alias("frot").alias("iWantIcecream")
        .alias("Icecube")
        .on_init([&]() { spawn_em00x(EnemyType::FROST); });

    MutatorRegistry::define("SpawnAssault").weight(10)
        .description("Spawns Assault").alias("Assault").alias("Lizard")
        .on_init([&]() { spawn_em00x(EnemyType::ASSAULT); });

    MutatorRegistry::define("SpawnBlitz")
        .description("Spawns Blitz").weight(1)
        .alias("Blitz").alias("Mark").alias("Zuck")
        .on_init([&]() { spawn_em00x(EnemyType::BLITZ); });

    MutatorRegistry::define("SpawnChimera")
        .description("Spawns Chimera Seed").weight(2)/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Chimera").alias("seed").alias("eg").alias("egg").alias("gregg")
        .alias("cum")
        .on_init([&]() { spawn_em00x(EnemyType::CHIMERA_SEED); });
        
    MutatorRegistry::define("SummonThePunishment")
        .description("Rain Down The God's Wrath")/*.special_arg(&g_enable_twitch_special_spawns)*/
        /*.cumrain_arg(&g_forbid_cumrain)*/
        .alias("CUMRAIN").weight(4)
        .on_init([]() { 
            /*g_show_cum = true;*/
            for(int i = 0; i < 50; i++) 
                spawn_em00x(EnemyType::CHIMERA_SEED);
        });

    MutatorRegistry::define("SpawnCutlass").weight(0) // broken
        .description("Spawns Cutlass")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Cutlass").alias("Fish")
        .on_init([&]() { spawn_em00x(EnemyType::CUTLASS); });

    MutatorRegistry::define("SpawnGladius").weight(0) // broken
        .description("Spawns Gladius")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Gladius").alias("Swordfish")
        .on_init([&]() { spawn_em00x(EnemyType::GLADIUS); });

    MutatorRegistry::define("SpawnBasilisk").weight(10)
        .description("Spawns Basilisk").alias("Basilisk").alias("dog").alias("doggo")
        .alias("doge").alias("dogg").alias("puppy")
        .on_init([&]() { spawn_em00x(EnemyType::BASILISK); });

    MutatorRegistry::define("SpawnBerial").weight(1)
        .description("Spawns Berial")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Berial").alias("FirePony")
        .on_init([&]() { spawn_em00x(EnemyType::BERIAL); });

    MutatorRegistry::define("SpawnBael").weight(1)
        .description("Spawns Bael")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Bael").alias("frog").alias("froggy").alias("frogeh")
        .alias("froggeh").alias("toad").alias("runkiller")
        .alias("widepeepoHappy").alias("жаба")
        .on_init([&]() { spawn_em00x(EnemyType::BAEL); });

    MutatorRegistry::define("SpawnEchidna").weight(1)
        .description("Spawns Echidna")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Echidna").alias("HentaiWaifu")
        .on_init([&]() { spawn_em00x(EnemyType::ECHIDNA); });

    MutatorRegistry::define("SpawnCredo").weight(2)
        .description("Spawns Angelo Credo")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Credo").alias("NoCavIsNotAGoodBoss").alias("BiggusDickus")
        .on_init([&]() { spawn_em00x(EnemyType::CREDO); });

    MutatorRegistry::define("SpawnAgnus").weight(1)
        .description("Spawns Angelo Agnus")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Agnus").alias("Agnis").alias("Ragnis").alias("moth").alias("mothman")
        .on_init([&]() { spawn_em00x(EnemyType::AGNUS); });

    MutatorRegistry::define("SpawnSanctus").weight(0) // broken iirc
        .description("Spawns Sanctus")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Sanctus").alias("OldFart").alias("pope").alias("Sanctussy")
        .on_init([&]() { spawn_em00x(EnemyType::SANCTUS); });

    MutatorRegistry::define("SpawnSanctusDia").weight(0) // broken iirc
        .description("Spawns Sanctus Diabolica")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Sanctus2").alias("OldFart2").alias("pope2").alias("Sanctussy2")
        .on_init([&]() { spawn_em00x(EnemyType::SANCTUS_DIABOLICA); });

    MutatorRegistry::define("SpawnKyrie").weight(10)
        .description("Spawns Kyrie").alias("Kyrie")
        .on_init([&]() { spawn_em00x(EnemyType::KYRIE); });

    MutatorRegistry::define("SpawnDante").weight(0) // broken
        .description("Spawns Dante")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Dante").alias("Donny").alias("Donte").alias("DonterKebab")
        .on_init([&]() { spawn_em00x(EnemyType::DANTE); });

    MutatorRegistry::define("SpawnRandom").weight(8)
        .description("Spawns random enemy")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Random")
        .on_init([]() {
            spawn_random_enemy();
        });

    EnemySpawn::m_hotkeys.reserve(HOTKEY_MAX);
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F1 }, __("Spawn Scarecrow Leg"), "spawn_scarecrow_leg_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F2 }, __("Spawn Scarecrow Arm"), "spawn_scarecrow_arm_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F3 }, __("Spawn Mega"), "spawn_mega_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F4 }, __("Spawn Bianco"), "spawn_bianco_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F5 }, __("Spawn Alto"), "spawn_alto_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F6 }, __("Spawn Mephisto"), "spawn_mephisto_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F7 }, __("Spawn Faust"), "spawn_faust_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F8 }, __("Spawn Frost"), "spawn_frost_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F9 }, __("Spawn Assault"), "spawn_assault_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F10 }, __("Spawn Blitz"), "spawn_blitz_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F11 }, __("Spawn Chimera"), "spawn_chimera_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_CONTROL, VK_F12 }, __("Spawn Gladius"), "spawn_gladius_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_SHIFT,   VK_F1  }, __("Spawn Cutlass"), "spawn_cutlass_key");
    utility::create_keyboard_hotkey(EnemySpawn::m_hotkeys, { VK_SHIFT,   VK_F2  }, __("Spawn Basilisk"), "spawn_basilisk_key");

    console->system().RegisterCommand("spawnscarecrowleg", "Spawn a Scarecrow Leg", [/*this*/]() {
        spawn_em00x(EnemyType::SCARECROW_LEG);
    });
    console->system().RegisterCommand("spawnscarecrowarm", "Spawn a Scarecrow Arm", [/*this*/]() {
        spawn_em00x(EnemyType::SCARECROW_ARM);
    });
    console->system().RegisterCommand("spawnmega", "Spawn a Mega Scarecrow", [/*this*/]() {
        spawn_em00x(EnemyType::SCARECROW_MEGA);
    });
    console->system().RegisterCommand("spawnbianco", "Spawn a Bianco Angelo", [/*this*/]() {
        spawn_em00x(EnemyType::ANGELO_BIANCO);
    });
    console->system().RegisterCommand("spawnalto", "Spawn an Alto Angelo", [/*this*/]() {
        spawn_em00x(EnemyType::ANGELO_ALTO);
    });
    console->system().RegisterCommand("spawnmephisto", "Spawn a Mephisto", [/*this*/]() {
        spawn_em00x(EnemyType::MEPHISTO);
    });
    console->system().RegisterCommand("spawnfaust", "Spawn a Faust", [/*this*/]() {
        spawn_em00x(EnemyType::FAUST);
    });
    console->system().RegisterCommand("spawnfrost", "Spawn a Frost", [/*this*/]() {
        spawn_em00x(EnemyType::FROST);
    });
    console->system().RegisterCommand("spawnassault", "Spawn an Assault", [/*this*/]() {
        spawn_em00x(EnemyType::ASSAULT);
    });
    console->system().RegisterCommand("spawnblitz", "Spawn a Blitz", [/*this*/]() {
        spawn_em00x(EnemyType::BLITZ);
    });
    console->system().RegisterCommand("spawnseed", "Spawn a Chimera Seed", [/*this*/]() {
        spawn_em00x(EnemyType::CHIMERA_SEED);
    });
    console->system().RegisterCommand("spawncutlass", "Spawn a Cutlass", [/*this*/]() {
        spawn_em00x(EnemyType::CUTLASS);
    });
    console->system().RegisterCommand("spawngladius", "Spawn a Gladius", [/*this*/]() {
        spawn_em00x(EnemyType::GLADIUS);
    });
    console->system().RegisterCommand("spawnbasilisk", "Spawn a Basilisk", [/*this*/]() {
        spawn_em00x(EnemyType::BASILISK);
    });
    console->system().RegisterCommand("spawnberial", "Spawn a Berial", [/*this*/]() {
        spawn_em00x(EnemyType::BERIAL);
    });
    console->system().RegisterCommand("spawnbael", "Spawn a Bael", [/*this*/]() {
        spawn_em00x(EnemyType::BAEL);
    });
    console->system().RegisterCommand("spawnechidna", "Spawn an Echidna", [/*this*/]() {
        spawn_em00x(EnemyType::ECHIDNA);
    });
    console->system().RegisterCommand("spawnagnus", "Spawn an Agnus", [/*this*/]() {
        spawn_em00x(EnemyType::AGNUS);
    });
    console->system().RegisterCommand("spawncredo", "Spawn a Credo", [/*this*/]() {
        spawn_em00x(EnemyType::CREDO);
    });
    console->system().RegisterCommand("spawnsanctus", "Spawn a Sanctus", [/*this*/]() {
        spawn_em00x(EnemyType::SANCTUS);
    });
    console->system().RegisterCommand("spawnsanctus2", "Spawn a Sanctus Diabolica", [/*this*/]() {
        spawn_em00x(EnemyType::SANCTUS_DIABOLICA);
    });
    console->system().RegisterCommand("spawnkyrie", "Spawn a Kyrie", [/*this*/]() {
        spawn_em00x(EnemyType::KYRIE);
    });
    console->system().RegisterCommand("spawndante", "Spawn a Dante", [/*this*/]() {
        spawn_em00x(EnemyType::DANTE);
    });
    console->system().RegisterCommand("spawnrandom", "Spawn a random enemy", [/*this*/]() {
        spawn_random_enemy();
    });

    return Mod::on_initialize();
}

void EnemySpawn::on_gui_frame(int display) {
    uPlayer* player = devil4_sdk::get_local_player();
    int enemy_names_current = 0;
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);

    utility::ImGooListboxTranslated translated_names(enemy_names.data(), enemy_names.size());
    if (ImGui::ListBox("##Enemy Spawn Listbox", &enemy_names_current, translated_names.data(), translated_names.size(), 23)) {
        if (player)
            spawn_em00x((EnemyType)enemy_names_current);
    }
    if (ImGui::Button(_("Random"))) {
        if (player)
            spawn_random_enemy();
    }
    ImGui::Spacing();
    static intptr_t custom_spawn_addr = NULL;
    ImGui::Text(_("[DEBUG] Custom Spawn Addr"));
    ImGui::InputInt("##[DEBUG] Custom Spawn Addr InputInt", &custom_spawn_addr, 0, 0, ImGuiInputTextFlags_CharsHexadecimal);
    if (ImGui::Button(_("Spawn"))) {
        if (player)
            spawn_custom(custom_spawn_addr);
    }
    ImGui::PopItemWidth();
}

// void EnemySpawn::on_config_load(const utility::Config& cfg) {};

// void EnemySpawn::on_config_save(utility::Config& cfg) {}

void EnemySpawn::on_update_input(utility::Input& input) {
    for (size_t i = 0; i < EnemySpawn::m_hotkeys.size(); i++) {
        if (EnemySpawn::m_hotkeys[i]->check(input)) {
            spawn_em00x((EnemyType)i);
        };
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
