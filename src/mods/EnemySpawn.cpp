
//#include "../sdk/ReClass_Internal.hpp"

#include "EnemySpawn.hpp"
#include "AreaJump.hpp" // for cAreaJumpPtr
// TODO(): move this somewhere maybe
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
static uintptr_t fptr_update_actor_list{0x008DC540}; // Spawns shit
static uintptr_t some_struct{0x00E552CC};
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
// em027    // Credo human form, pl004           //
// em029    // Sanctus, pl002, pl0023            // 62D0876D
// em030    // Sanctus2, pl023                   //
// em031    // Credo human again, pl004          //
// em033    // Savior3                           //
// em035    // style checker, em027, orb005      //
// em036    // Kyrie, pl022                      //
// em_dante // Boss Dante                        // 3D355100

constexpr std::array<uintptr_t, 21> fptr_em_factories{
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
    0x0061A7B0, // 11 Basilisk
    0x00630AC0, // 12 Berial
    0x00649CB0, // 13 Bael
    0x00685340, // 14 Echidna
    0x006AA2C0, // 16 Angelo Credo
    0x006BDE60, // 15 Angelo Agnus
    0x006F81E0, // 17 Sanctus
    0x007022F0, // 18 Sanctus Diabolica
    0x00723C00, // 19 Kyrie
    0x007BF980  // 20 Dante
    // 0x00609C20,             // // Cutlass // Broken
    // 0x00618460,             // // Gladius // Broken
    //                         // // Fault
    //                         // // The Savior
    //                         // // The False Savior
};

constexpr std::array<const char*, 21> enemy_names{
    "Scarecrow (Leg)",   // 00 Leg Scarecrow
    "Scarecrow (Arm)",   // 01 Arm Scarecrow
    "Mega Scarecrow",    // 02 Mega Scarecrow
    "Bianco Angelo",     // 03 Bianco Angelo
    "Alto Angelo",       // 04 Alto Angelo
    "Mephisto",          // 05 Mephisto
    "Faust",             // 06 Faust
    "Frost",             // 07 Frost
    "Assault",           // 08 Assault
    "Blitz",             // 09 Blitz
    "Chimera Seed",      // 10 Chimera Seed
    "Basilisk",          // 11 Basilisk
    "Berial",            // 12 Berial
    "Bael",              // 13 Bael
    "Echidna",           // 14 Echidna
    "Angelo Credo",      // 16 Angelo Credo
    "Angelo Agnus",      // 15 Angelo Agnus
    "Sanctus",           // 17 Sanctus
    "Sanctus Diabolica", // 18 Sanctus Diabolica
    "Kyrie",             // 19 Kyrie
    "Dante"              // 20 Dante
                         // "Cutlass",		       // // Cutlass // Broken
                         // "Gladius",		       // // Gladius // Broken
                         // Fault			       // // Fault
                         // The Savior		       // // The Savior
                         // The False Savior	       // // The False Savior
};

constexpr std::array<int, 21> enemy_spawn_type{
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
    3, // 11 Basilisk
    4, // 12 Berial
    0, // 13 Bael
    4, // 14 Echidna
    4, // 16 Angelo Credo
    4, // 15 Angelo Agnus
    4, // 17 Sanctus
    4, // 18 Sanctus Diabolica
    4, // 19 Kyrie
    4  // 20 Dante
    // 4                       // // Cutlass // Broken
    // 4                       // // Gladius // Broken
    // 4                       // // Fault
    // 4                       // // The Savior
    // 4                       // // The False Savior
};

/*
spawn animations:
if not listed, 4 was used
5 is broken on most, T POSE BABYYYYY

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
*/

glm::vec3 get_player_position() {
    // TODO(): move this into somewhere general since
    // this player ptr might be useful in other places
    s_med_ptr   = (SMediator*)*(uintptr_t*)static_mediator_ptr;
    u_local_plr = s_med_ptr->player_ptr;
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

void spawn_em00x(int index) {
    std::lock_guard<std::mutex> lk(g_mutex);
    uintptr_t em_function_pointer = fptr_em_factories.at(index);
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

        mov ecx, 0Fh
        mov eax, [some_struct] // static
		mov eax, [eax]
        push 0Fh
        call fptr_update_actor_list
		popfd
		popad
    }
}

void spawn_random_enemy() {
    auto now = std::chrono::system_clock::now();
    srand((uint32_t)now.time_since_epoch().count());
    // I thought this would stop crashes but no
    // if (devil4_sdk::get_sMediator()->uBoss1)
    //    spawn_em00x(rand() % 11); // do not spawn a boss if a boss already exists
    // else
    spawn_em00x(rand() % 21);
}

std::optional<std::string> EnemySpawn::on_initialize() {
    //srand((uint32_t)this);
    MutatorRegistry::define("SpawnScarecrowLeg").weight(8)
        .description("Spawns ScarecrowLeg").alias("Leg")
        .on_init([&]() { spawn_em00x(0); });

    MutatorRegistry::define("SpawnScarecrowArm").weight(7)
        .description("Spawns ScarecrowArm").alias("Arm")
        .on_init([&]() { spawn_em00x(1); });

    MutatorRegistry::define("SpawnMega").weight(6)
        .description("Spawns a Mega").alias("Mega")
        .on_init([&]() { spawn_em00x(2); });

    MutatorRegistry::define("SpawnBianco").weight(10)
        .description("Spawns Bianco Angelo").alias("Bianco").alias("BA")
        .on_init([&]() { spawn_em00x(3); });

    MutatorRegistry::define("SpawnAlto").weight(10)
        .description("Spawns Alto Angelo").alias("Alto").alias("AA")
        .on_init([&]() { spawn_em00x(4); });

    MutatorRegistry::define("SpawnMephisto").weight(10)
        .description("Spawns Mephisto").alias("Mephisto").alias("FlyingRat")
        .on_init([&]() { spawn_em00x(5); });

    MutatorRegistry::define("SpawnFaust").weight(10)
        .description("Spawns Faust").alias("FlyingRatDaddy").alias("EdwardScissorhands")
        .on_init([&]() { spawn_em00x(6); });

    MutatorRegistry::define("SpawnFrost").weight(10)
        .description("Spawns Frost").alias("Frost").alias("frot").alias("iWantIcecream")
        .alias("Icecube")
        .on_init([&]() { spawn_em00x(7); });

    MutatorRegistry::define("SpawnAssault").weight(10)
        .description("Spawns Assault").alias("Assault").alias("Lizard")
        .on_init([&]() { spawn_em00x(8); });

    MutatorRegistry::define("SpawnBlitz")
        .description("Spawns Blitz").weight(1)
        .alias("Blitz").alias("Mark").alias("Zuck")
        .on_init([&]() { spawn_em00x(9); });

    MutatorRegistry::define("SpawnChimera")
        .description("Spawns Chimera Seed").weight(2)/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Chimera").alias("seed").alias("eg").alias("egg").alias("gregg")
        .alias("cum")
        .on_init([&]() { spawn_em00x(10); });
        
    MutatorRegistry::define("SummonThePunishment")
        .description("Rain Down The God's Wrath")/*.special_arg(&g_enable_twitch_special_spawns)*/
        /*.cumrain_arg(&g_forbid_cumrain)*/
        .alias("CUMRAIN").weight(4)
        .on_init([]() { 
            /*g_show_cum = true;*/
            for(int i = 0; i < 50; i++) 
                spawn_em00x(10);
        });

    MutatorRegistry::define("SpawnBasilisk").weight(10)
        .description("Spawns Basilisk").alias("Basilisk").alias("dog").alias("doggo")
        .alias("doge").alias("dogg").alias("puppy")
        .on_init([&]() { spawn_em00x(11); });

    MutatorRegistry::define("SpawnBerial").weight(1)
        .description("Spawns Berial")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Berial").alias("FirePony")
        .on_init([&]() { spawn_em00x(12); });

    MutatorRegistry::define("SpawnBael").weight(1)
        .description("Spawns Bael")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Bael").alias("frog").alias("froggy").alias("frogeh")
        .alias("froggeh").alias("toad").alias("runkiller")
        .alias("widepeepoHappy").alias("жаба")
        .on_init([&]() { spawn_em00x(13); });

    MutatorRegistry::define("SpawnEchidna").weight(1)
        .description("Spawns Echidna")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Echidna").alias("HentaiWaifu")
        .on_init([&]() { spawn_em00x(14); });

    MutatorRegistry::define("SpawnCredo").weight(2)
        .description("Spawns Angelo Credo")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Credo").alias("NoCavIsNotAGoodBoss").alias("BiggusDickus")
        .on_init([&]() { spawn_em00x(15); });

    MutatorRegistry::define("SpawnAgnus").weight(1)
        .description("Spawns Angelo Agnus")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Agnus").alias("Agnis").alias("Ragnis").alias("moth").alias("mothman")
        .on_init([&]() { spawn_em00x(16); });

    MutatorRegistry::define("SpawnSanctus").weight(0) // broken iirc
        .description("Spawns Sanctus Diabolica")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Sanctus").alias("OldFart").alias("pope").alias("Sanctussy")
        .on_init([&]() { spawn_em00x(18); });

    MutatorRegistry::define("SpawnKyrie").weight(10)
        .description("Spawns Kyrie").alias("Kyrie")
        .on_init([&]() { spawn_em00x(19); });

    MutatorRegistry::define("SpawnDante").weight(0) // broken
        .description("Spawns Dante")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Dante").alias("Donny").alias("Donte").alias("DonterKebab")
        .on_init([&]() { spawn_em00x(20); });

    MutatorRegistry::define("SpawnRandom").weight(8)
        .description("Spawns random enemy")/*.special_arg(&g_enable_twitch_special_spawns)*/
        .alias("Random")
        .on_init([]() {
            spawn_random_enemy();
        });

    using v_key = std::vector<uint32_t>;
    m_hotkeys.reserve(HOTKEY_MAX);
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F1 }, "Spawn Scarecrow Leg", "spawn_scarecrow_leg_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F2 }, "Spawn Scarecrow Arm", "spawn_scarecrow_arm_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F3 }, "Spawn Mega", "spawn_mega_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F4 }, "Spawn Bianco", "spawn_bianco_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F5 }, "Spawn Alto", "spawn_alto_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F6 }, "Spawn Mephisto", "spawn_mephisto_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F7 }, "Spawn Faust", "spawn_faust_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F8 }, "Spawn Frost", "spawn_frost_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F9 }, "Spawn Assault", "spawn_assault_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F10 }, "Spawn Blitz", "spawn_blitz_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F11 }, "Spawn Chimera", "spawn_chimera_key"));
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{ VK_CONTROL, VK_F12 }, "Spawn Basilisk", "spawn_basilisk_key"));

    console->system().RegisterCommand("spawnscarecrowleg", "Spawn a Scarecrow Leg", [/*this*/]() {
        spawn_em00x(0);
    });
    console->system().RegisterCommand("spawnscarecrowarm", "Spawn a Scarecrow Arm", [/*this*/]() {
        spawn_em00x(1);
    });
    console->system().RegisterCommand("spawnmega", "Spawn a Mega Scarecrow", [/*this*/]() {
        spawn_em00x(2);
    });
    console->system().RegisterCommand("spawnbianco", "Spawn a Bianco Angelo", [/*this*/]() {
        spawn_em00x(3);
    });
    console->system().RegisterCommand("spawnalto", "Spawn an Alto Angelo", [/*this*/]() {
        spawn_em00x(4);
    });
    console->system().RegisterCommand("spawnmephisto", "Spawn a Mephisto", [/*this*/]() {
        spawn_em00x(5);
    });
    console->system().RegisterCommand("spawnfaust", "Spawn a Faust", [/*this*/]() {
        spawn_em00x(6);
    });
    console->system().RegisterCommand("spawnfrost", "Spawn a Frost", [/*this*/]() {
        spawn_em00x(7);
    });
    console->system().RegisterCommand("spawnassault", "Spawn an Assault", [/*this*/]() {
        spawn_em00x(8);
    });
    console->system().RegisterCommand("spawnblitz", "Spawn a Blitz", [/*this*/]() {
        spawn_em00x(9);
    });
    console->system().RegisterCommand("spawnseed", "Spawn a Chimera Seed", [/*this*/]() {
        spawn_em00x(10);
    });
    console->system().RegisterCommand("spawnbasilisk", "Spawn a Basilisk", [/*this*/]() {
        spawn_em00x(11);
    });
    console->system().RegisterCommand("spawnberial", "Spawn a Berial", [/*this*/]() {
        spawn_em00x(12);
    });
    console->system().RegisterCommand("spawnbael", "Spawn a Bael", [/*this*/]() {
        spawn_em00x(13);
    });
    console->system().RegisterCommand("spawnechidna", "Spawn an Echidna", [/*this*/]() {
        spawn_em00x(14);
    });
    console->system().RegisterCommand("spawnagnus", "Spawn an Agnus", [/*this*/]() {
        spawn_em00x(15);
    });
    console->system().RegisterCommand("spawncredo", "Spawn a Credo", [/*this*/]() {
        spawn_em00x(16);
    });
    console->system().RegisterCommand("spawnsanctus", "Spawn a Sanctus", [/*this*/]() {
        spawn_em00x(17);
    });
    console->system().RegisterCommand("spawnsanctus2", "Spawn a Sanctus Diabolica", [/*this*/]() {
        spawn_em00x(18);
    });
    console->system().RegisterCommand("spawnkyrie", "Spawn a Kyrie", [/*this*/]() {
        spawn_em00x(19);
    });
    console->system().RegisterCommand("spawndante", "Spawn a Dante", [/*this*/]() {
        spawn_em00x(20);
    });
    console->system().RegisterCommand("spawnrandom", "Spawn a random enemy", [/*this*/]() {
        spawn_random_enemy();
    });

    return Mod::on_initialize();
}

void EnemySpawn::on_gui_frame() {
    if (IsBadWritePtr(AreaJump::c_area_jump_ptr, sizeof(uint32_t)) || IsBadReadPtr(AreaJump::c_area_jump_ptr, sizeof(uint32_t))) {
        ImGui::TextWrapped(_("Enemy Spawner is not initialized.\nLoad into a stage to access it."));
        return;
    }
    ImGui::Text(_("Enemy Spawner"));
    ImGui::SameLine();
    help_marker(_("Any clicked enemy will spawn above player coords"));
    int enemy_names_current = 0;
    if (ImGui::ListBox("##Enemy Spawn Listbox", &enemy_names_current, enemy_names.data(), enemy_names.size(), 21)) {
        spawn_em00x(enemy_names_current);
    }
    if (ImGui::Button(_("Random"))) {
        spawn_random_enemy();
    }
}

#if 0
void EnemySpawn::on_twitch_command(std::size_t hash) {
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_scarecrow_leg_command);
    if (hash == m_spawn_scarecrow_leg_command) {
        spawn_em00x(0);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_scarecrow_arm_command);
    if (hash == m_spawn_scarecrow_arm_command) {
        spawn_em00x(1);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_mega_command);
    if (hash == m_spawn_mega_command) {
        spawn_em00x(2);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_bianco_command);
    if (hash == m_spawn_bianco_command) {
        spawn_em00x(3);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_alto_command);
    if (hash == m_spawn_alto_command) {
        spawn_em00x(4);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_mephisto_command);
    if (hash == m_spawn_mephisto_command) {
        spawn_em00x(5);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_faust_command);
    if (hash == m_spawn_faust_command) {
        spawn_em00x(6);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_frost_command);
    if (hash == m_spawn_frost_command) {
        spawn_em00x(7);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_assault_command);
    if (hash == m_spawn_assault_command) {
        spawn_em00x(8);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_blitz_command);
    if (hash == m_spawn_blitz_command) {
        spawn_em00x(9);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_chimera_command);
    if (hash == m_spawn_chimera_command) {
        spawn_em00x(10);
    }
    spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_basilisk_command);
    if (hash == m_spawn_basilisk_command) {
        spawn_em00x(11);
    }
    if (enable_twitch_special_spawns) {
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_berial_command);
        if (hash == m_spawn_berial_command) {
            spawn_em00x(12);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_bael_command);
        if (hash == m_spawn_bael_command) {
            spawn_em00x(13);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_echidna_command);
        if (hash == m_spawn_echidna_command) {
            spawn_em00x(14);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_credo_command);
        if (hash == m_spawn_credo_command) {
            spawn_em00x(15);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_agnus_command);
        if (hash == m_spawn_agnus_command) {
            spawn_em00x(16);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_sanctus_dia_command);
        if (hash == m_spawn_sanctus_dia_command) {
            spawn_em00x(18); // skipping non diabolica sanctus
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_kyrie_command);
        if (hash == m_spawn_kyrie_command) {
            spawn_em00x(19);
        }
        spdlog::debug("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_spawn_dante_command);
        if (hash == m_spawn_dante_command) {
            spawn_em00x(20);
        }
    }
}
#endif

void EnemySpawn::on_config_load(const utility::Config& cfg) {
    //g_enable_twitch_special_spawns = cfg.get<bool>("enable_twitch_special_spawns").value_or(true);
    //g_forbid_cumrain = cfg.get<bool>("forbid_cumrain").value_or(false);
};

void EnemySpawn::on_config_save(utility::Config& cfg) {
    //cfg.set<bool>("enable_twitch_special_spawns", g_enable_twitch_special_spawns);
    //cfg.set<bool>("forbid_cumrain", g_forbid_cumrain);
}

void EnemySpawn::on_update_input(utility::Input& input) {

    for (size_t i = 0; i < m_hotkeys.size(); i++) {
        if (m_hotkeys[i]->check(input)) {
            spawn_em00x(i);
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
