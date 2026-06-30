#include "Survival.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/cUnit.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/sMediator.hpp"
#include "sdk/sUnit.hpp"
#include "EnemySpawn.hpp"
#include "AreaJump.hpp"
#include "SpawnedEnemiesAttack.hpp"
#include "PowerUpSystem.hpp"
#include "EnemyTracker.hpp" // for enemy specific damage offset
#include "Quicksilver.hpp"
#include "MutatorHolyWater.hpp"
#include "BigHeadMode.hpp"
#include "HideHud.hpp"
#include "DarkSoulsStamina.hpp"
#include "imgui_internal.h"
#include "..\sdk\sArea.hpp"
#include "EnvironmentalHazards.hpp"
#include "RotatingLaser.hpp"
#include "CharSwitcher.hpp" // for external_spawn_requested, stops chars being registered to sMed
#include "GermanWord.hpp"

static constexpr uintptr_t some_struct            = 0x00E552CC;
static constexpr uintptr_t fptr_update_actor_list = 0x008DC540;

class WaveConfig {
public:
    int max_enemies_with_boss;
    int max_enemies_without_boss;
    int ldk_max_enemies_with_boss;
    int ldk_max_enemies_without_boss;
    int boss_waves_cooldown;
    int max_bosses;
    int max_side_enemies;
    int boss_spawn_chance;
    int side_enemy_spawn_chance;
    int powerup_spawn_chance;
    int laser_spawn_chance;
    std::vector<SpawnableEnemyType> standard_enemies;
    std::vector<SpawnableEnemyType> side_enemies;
    std::vector<SpawnableEnemyType> boss_enemies;
    WaveConfig(
        int _max_with_boss, int _max_without_boss,
        int _ldk_max_with_boss, int _ldk_max_without_boss,
        int _boss_cooldown, int _max_bosses,
        int _max_side_enemies,
        int _boss_chance, int _side_chance, int _powerup_chance, int _laser_chance,
        const std::vector<SpawnableEnemyType>& _standard_enemies = {},
        const std::vector<SpawnableEnemyType>& _side_enemies = {},
        const std::vector<SpawnableEnemyType>& _boss_enemies = {}
    ) :
        max_enemies_with_boss(_max_with_boss),
        max_enemies_without_boss(_max_without_boss),
        ldk_max_enemies_with_boss(_ldk_max_with_boss),
        ldk_max_enemies_without_boss(_ldk_max_without_boss),
        boss_waves_cooldown(_boss_cooldown),
        max_bosses(_max_bosses),
        max_side_enemies(_max_side_enemies),
        boss_spawn_chance(_boss_chance),
        side_enemy_spawn_chance(_side_chance),
        powerup_spawn_chance(_powerup_chance),
        laser_spawn_chance(_laser_chance),
        standard_enemies(_standard_enemies),
        side_enemies(_side_enemies),
        boss_enemies(_boss_enemies)
    {}
};

bool Survival::mod_enabled = false;
bool Survival::meme_effects = false;
bool Survival::survival_active = false; // Set dynamically, not a ui toggle
uintptr_t Survival::jmp_return_hp = NULL;
uintptr_t Survival::jmp_return_red_timer = NULL;

// ImVec2 Survival::window_pos{ 0.0f, 0.0f };
utility::Timer* Survival::timer{};
utility::Timer* Survival::meme_timer{};
float Survival::survivedTimer = 0.0f;
int Survival::wave = 0;
bool Survival::player_existed_last_frame = false;
bool Survival::meme_player_existed_last_frame = false;
std::random_device Survival::rd;
std::mt19937 Survival::rng(Survival::rd());
static std::unique_ptr<PowerUpSystem> basicPowerUpSystem = std::make_unique<PowerUpSystem>();
static std::unique_ptr<PowerUpSystem> memePowerUpSystem = std::make_unique<PowerUpSystem>();
static float accumulated_delta = 0.0f;
static const float teleport_delay = 50.0f;
static int waves_since_boss = 0;
static bool survival_active_last_frame = false;
static bool pending_arc_load = true;

// for stage select
struct SurvivalRoom {
    int roomID;
    int roomNumber;
};

const std::vector<SurvivalRoom> survivalRooms {
    {705,   1},
    {704,  21},
    {703,  41},
    {701,  61},
    {702,  81},
    {700, 101}
};
constexpr std::array<const char*, 6> survival_room_names {
    __("BP 1-19"),
    __("BP 21-39"), 
    __("BP 41-59"),
    __("BP 61-79"),
    __("BP 81-99"),
    __("BP 101")
};
int Survival::currentRoomIndex = 0;

// safe to be called with no enemy
Survival::EnemyInfo Survival::get_enemy_info(uEnemy_Old* enemy) {
    EnemyInfo enemy_info{ 0, 0 };
    while (enemy) {
        uDamage_Old* currentEnemyDamage = (uDamage_Old*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
        if (currentEnemyDamage->HP > 0.0f) {
            enemy_info.enemies_alive++;
            if (enemy->ID >= BERIAL || enemy->ID == CREDO || enemy->ID == AGNUS || enemy->ID == BLITZ) {
                enemy_info.bosses_alive++;
            }
            if (enemy->ID == CHIMERA || enemy->ID == CUTLASS || enemy->ID == GLADIUS) {
                enemy_info.side_enemies_alive++;
            }
        }
        enemy = enemy->nextEnemy;
    }
    return enemy_info;
}

static const std::map<int, WaveConfig> WAVE_CONFIGS = {
    {5, WaveConfig( // 0-5
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        0,  // boss_waves_cooldown
        0,  // max_bosses
        0,  // max_side_enemies
        0,  // boss_spawn_chance
        0,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        0, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA}, // tier 1 enemies with duplicates for less chance of mega
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // Side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {10, WaveConfig( // 5-10 // introduce tier 2 enemies
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        0,  // boss_waves_cooldown
        0,  // max_bosses
        0,  // max_side_enemies
        0,  // boss_spawn_chance
        0,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        4, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT}, // tier 2 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {20, WaveConfig( // 20-30 // introduce tier 3 enemies
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        0,  // boss_waves_cooldown
        0,  // max_bosses
        0,  // max_side_enemies
        0,  // boss_spawn_chance
        0,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        3, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK}, // tier 3 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {30, WaveConfig( // 30-40 // introduce side enemies
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        0,  // boss_waves_cooldown
        0,  // max_bosses
        1,  // max_side_enemies
        0,  // boss_spawn_chance
        8,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        3, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK}, // tier 3 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {40, WaveConfig( // 40-50 // introduce tier 4 enemies
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        0,  // boss_waves_cooldown
        0,  // max_bosses
        1,  // max_side_enemies
        0,  // boss_spawn_chance
        8,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        2, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK, // tier 3 enemies
         SpawnableEnemyType::FAUST}, // tier 4 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {50, WaveConfig( // 40-50 // introduce bosses
        1,  // max_enemies_with_boss
        3,  // max_enemies_without_boss
        3,  // ldk_max_enemies_with_boss
        5,  // ldk_max_enemies_without_boss
        30, // boss_waves_cooldown
        1,  // max_bosses
        1,  // max_side_enemies
        8,  // boss_spawn_chance
        8,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        2, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK, // tier 3 enemies
         SpawnableEnemyType::FAUST}, // tier 4 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},
    
    {100, WaveConfig( // 50-100 // up enemy count + side enemy count
        3,  // max_enemies_with_boss
        6,  // max_enemies_without_boss
        5,  // ldk_max_enemies_with_boss
        7,  // ldk_max_enemies_without_boss
        20, // boss_waves_cooldown
        1,  // max_bosses
        2,  // max_side_enemies
        8,  // boss_spawn_chance
        8,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        1, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK, // tier 3 enemies
         SpawnableEnemyType::FAUST}, // tier 4 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )},

    {9999, WaveConfig( // 100+ // up enemy count + boss count + side enemy count, probably crashy with multiple bosses
        4,  // max_enemies_with_boss
        7,  // max_enemies_without_boss
        6,  // ldk_max_enemies_with_boss
        8,  // ldk_max_enemies_without_boss
        10, // boss_waves_cooldown
        2,  // max_bosses
        3,  // max_side_enemies
        8,  // boss_spawn_chance
        8,  // side_enemy_spawn_chance
        3,  // powerup_spawn_chance
        1, // laser_spawn_chance
        {SpawnableEnemyType::SCARECROW_LEG, SpawnableEnemyType::SCARECROW_ARM, SpawnableEnemyType::SCARECROW_MEGA, // tier 1 enemies
         SpawnableEnemyType::ANGELO_BIANCO, SpawnableEnemyType::MEPHISTO, SpawnableEnemyType::ASSAULT, // tier 2 enemies
         SpawnableEnemyType::FROST, SpawnableEnemyType::ANGELO_ALTO, SpawnableEnemyType::BASILISK, // tier 3 enemies
         SpawnableEnemyType::FAUST}, // tier 4 enemies
        {SpawnableEnemyType::CHIMERA_SEED, SpawnableEnemyType::CUTLASS, SpawnableEnemyType::GLADIUS}, // side enemies
        {SpawnableEnemyType::BLITZ, SpawnableEnemyType::CREDO, SpawnableEnemyType::BERIAL, SpawnableEnemyType::BAEL} // Boss enemies
    )}
};

static const WaveConfig& get_wave_config() {
    for (const auto& [wave_threshold, config] : WAVE_CONFIGS) {
        if (Survival::wave < wave_threshold) {
            return config;
        }
    }
    return WAVE_CONFIGS.rbegin()->second;
}

static bool can_spawn_standard_enemy(const Survival::EnemyInfo& enemy_info, sMediator* sMed, const WaveConfig& config) {
    bool is_ldk = (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT);
        
    int max_enemies = is_ldk
        ? (enemy_info.bosses_alive > 0 ? config.ldk_max_enemies_with_boss : config.ldk_max_enemies_without_boss)
        : (enemy_info.bosses_alive > 0 ? config.max_enemies_with_boss : config.max_enemies_without_boss);
        
    return enemy_info.enemies_alive < max_enemies;
}

static bool can_spawn_side_enemy(const Survival::EnemyInfo& enemy_info, const WaveConfig& config) {
    if (config.max_side_enemies <= 0 || config.side_enemy_spawn_chance <= 0) {
        return false;
    }
    
    return enemy_info.side_enemies_alive < config.max_side_enemies;
}

static bool can_spawn_boss(const Survival::EnemyInfo& enemy_info, const WaveConfig& config) {
    if (config.max_bosses <= 0 || config.boss_spawn_chance <= 0) {
        return false;
    }
    
    if (config.boss_waves_cooldown> 0 && waves_since_boss <= config.boss_waves_cooldown) {
        return false;
    }
    
    return enemy_info.bosses_alive < config.max_bosses;
}

struct EnemySpawnInfo {
    uintptr_t factory;
    std::vector<int> spawn_anims;
};

static std::array<EnemySpawnInfo, 23> enemy_spawn_info{{
    {0x0053F810, {4}},             // Leg Scarecrow
    {0x0055E710, {4}},             // Arm Scarecrow
    {0x0055F7E0, {4}},             // Mega Scarecrow
    {0x00561A10, {1,4}},           // Bianco Angelo
    {0x00576C80, {1,4}},           // Alto Angelo
    {0x0057F1E0, {3,4}},           // Mephisto
    {0x00595810, {3,4}},           // Faust
    {0x005A3F60, {4}},             // Frost
    {0x005B3170, {0,2}},           // Assault
    {0x005D1760, {1,2,4}},         // Blitz
    {0x005DC160, {2,3}},           // Chimera Seed
    {0x005F37F0, {2}},             // Cutlass
    {0x0060AFC0, {1,3,4,5,6,7,8}}, // Gladius
    {0x0061A7B0, {3}},             // Basilisk
    {0x00630AC0, {4}},             // Berial
    {0x00649CB0, {0}},             // Bael
    {0x00685340, {4}},             // Echidna
    {0x006AA2C0, {4}},             // Credo
    {0x006BDE60, {4}},             // Agnus
    {0x006F81E0, {4}},             // Sanctus
    {0x007022F0, {4}},             // Sanctus Diabolica
    {0x00723C00, {4}},             // Kyrie
    {0x007BF980, {4}},             // Dante
}};

glm::vec3 Survival::get_random_spawn_position() {
    static constexpr float radius    = 800.0f; // 1200 for bp size
    static constexpr float minHeight = 100.0f;
    static constexpr float maxHeight = 400.0f;
    float angle    = get_random_float(0.0f, 2.0f * 3.14159265f);
    float distance = get_random_float(5.0f, radius);
    float height   = get_random_float(minHeight, maxHeight);
    return glm::vec3(std::cos(angle) * distance, height, std::sin(angle) * distance);
}

static int get_random_spawn_anim(SpawnableEnemyType type) {
    std::vector anims = enemy_spawn_info.at((size_t)(type)).spawn_anims;
    return anims[Survival::get_random_int(0, (int)(anims.size()) - 1)];
}

static void set_survival_enemy_pos_and_anim(uEnemySomething* enemy, SpawnableEnemyType type, int spawnAnim) { // not in survival.hpp to save including EnemySpawn.hpp
    if (!enemy) {
        return;
    }
    if (spawnAnim >= 0) {
        enemy->m_enemy_spawn_effect_something = spawnAnim;
    } else {
        enemy->m_enemy_spawn_effect_something = get_random_spawn_anim(type);
    }
    if (type == SpawnableEnemyType::ASSAULT && enemy->m_enemy_spawn_effect_something == 0) { // sry
        enemy->m_spawn_coords = devil4_sdk::get_local_player()->mPos;
    }
    else {
        enemy->m_spawn_coords = Survival::get_random_spawn_position();
    }
}

static void spawn_enemy(SpawnableEnemyType index, int spawnAnim = -1) {
    uintptr_t em_function_pointer = enemy_spawn_info.at((size_t)size_t(index)).factory;
    if (!devil4_sdk::get_local_player())
        return; // only work while character is loaded

    // should never be an issue but lets do this to try to stop enemies not being hittable
    // even though I think that's a memory issue
    // while (devil4_sdk::is_loading_arc()) {
    //     Sleep(0);
    // }

    __asm {
		pushad
		pushfd
		call em_function_pointer // make actor
        mov esi, eax

        pushad
        push spawnAnim
        push index
        push esi
        call set_survival_enemy_pos_and_anim
        add esp, 0xc
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

static void spawn_enemy_from_pool(const std::vector<SpawnableEnemyType>& enemy_pool) {
    if (enemy_pool.empty()) {
        return;
    }
    int random_index = Survival::get_random_int(0, (int)(enemy_pool.size()) - 1);
    spawn_enemy(enemy_pool[random_index]);
}

void Survival::spawn_standard_enemy() {
    const WaveConfig& config = get_wave_config();
    spawn_enemy_from_pool(config.standard_enemies);
}

void Survival::spawn_side_enemy() {
    const WaveConfig& config = get_wave_config();
    spawn_enemy_from_pool(config.side_enemies);
}

void Survival::spawn_boss_enemy() {
    const WaveConfig& config = get_wave_config();
    spawn_enemy_from_pool(config.boss_enemies);
}

// When player is alive and in the correct room, this is called every x dante seconds or when every enemy is dead
void Survival::on_timer_trigger() {
    if (Survival::timer) {
        Survival::timer->start();
    }
        
    sMediator* sMed = devil4_sdk::get_sMediator();
    if (!sMed) { return; }
        
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
        
    uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
    Survival::EnemyInfo enemy_info = Survival::get_enemy_info(enemy);
        
    const WaveConfig& config = get_wave_config();
    bool is_ldk = (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT);

    // lasers does not care if an enemy can be spawned
    if (config.laser_spawn_chance > 0 && Survival::get_random_int(0, config.laser_spawn_chance - 1) == 0) {
        if (EnvironmentalHazards::laser_enabled) {
            RotatingLaser::spawn(
                {
                    get_random_float(-800.0f, 800.0f), // x pos
                    get_random_float(100.0f, 800.0f),  // y pos
                    get_random_float(-800.0f, 800.0f)  // z pos
                },
                {
                    get_random_float(-3.14f, 3.14f), // pitch
                    0.0f,                            // yaw
                    get_random_float(-3.14f, 3.14f), // roll
                },
                get_random_float(600.0f, 2400.0f), // length
                1.0f,                              // spawn delay
                30.0f,                             // lifetime
                get_random_int(0, 2));             // rotation type (still, rotate from one end, rotate from mid)
        }
    }
    
    if (!can_spawn_standard_enemy(enemy_info, sMed, config)) {
        return;
    }
        
    // Every enemy spawn is a new wave
    Survival::wave++;
        
    // spawn a boss
    bool boss_spawned = false;
    if (can_spawn_boss(enemy_info, config) && Survival::get_random_int(0, config.boss_spawn_chance - 1) == 0) {
        Survival::spawn_boss_enemy();
        waves_since_boss = 0;
        boss_spawned = true;
    } else {
        waves_since_boss++;
    }
    
    // Recalculate enemy info after potential boss spawn
    enemy_info = Survival::get_enemy_info(devil4_sdk::get_uEnemies());
    
    // Check if we can still spawn standard enemies
    if (can_spawn_standard_enemy(enemy_info, sMed, config)) {
        Survival::spawn_standard_enemy();
    }
        
    // spawn an extra dude in ldk
    enemy_info = Survival::get_enemy_info(devil4_sdk::get_uEnemies());
    if (is_ldk && can_spawn_standard_enemy(enemy_info, sMed, config)) {
        Survival::spawn_standard_enemy();
    }
        
    // spawn a side dude
    enemy_info = Survival::get_enemy_info(devil4_sdk::get_uEnemies());
    if (can_spawn_side_enemy(enemy_info, config) && 
        Survival::get_random_int(0, config.side_enemy_spawn_chance - 1) == 0 && 
        can_spawn_standard_enemy(enemy_info, sMed, config)) {
        Survival::spawn_side_enemy();
    }
        
    // spawn powerup
    if (config.powerup_spawn_chance > 0 && Survival::get_random_int(0, config.powerup_spawn_chance - 1) == 0) {
        if (basicPowerUpSystem) {
            basicPowerUpSystem->spawnRandomPowerUp();
        }
    }
}

static constexpr uintptr_t DisplayTimerCall = 0x494EA0;
naked void DisplayTimerOnTick() {
    _asm {
        pushad
        mov edx, 1
        push edx
        call dword ptr [DisplayTimerCall]
        popad
        ret
    }
}

static constexpr uintptr_t sWorkRatePtr = 0xE558D0;
static constexpr uintptr_t GetTimerTickCall = 0x4A6890;
naked void UpdateTimer() {
    _asm {
        pushad
        mov eax, [sWorkRatePtr]
        mov eax, [eax]
        push 05
        push eax
        call dword ptr [GetTimerTickCall]
        movss xmm1, [Survival::survivedTimer]
        addss xmm1, xmm0
        movss [Survival::survivedTimer], xmm1
        popad
        ret
    }
}

void Survival::on_frame(fmilliseconds& dt) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (memePowerUpSystem) {
        if (meme_timer) {
            bool meme_player_exists_now = (player != nullptr);
            if (meme_player_exists_now && !meme_player_existed_last_frame) {
                meme_timer->start();
            }
            meme_player_existed_last_frame = meme_player_exists_now;
            if (meme_player_exists_now) {
                memePowerUpSystem->on_frame(dt);
                float dante_seconds = player->m_delta_time / 60.0f;
                meme_timer->tick((fmilliseconds)dante_seconds * 1000.0f);
            }
        }
    }
    if (Survival::mod_enabled) {
        sMediator* sMed = devil4_sdk::get_sMediator();
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        if (!sMed || sMed->missionID == 50 || !s_area_ptr) {
            Survival::survival_active = false;
            reset_wave();
            return;
        }
        
        bool player_exists_now = (player != nullptr);
        bool player_is_alive = player_exists_now && player->damageStruct.HP > 0.0f;
        bool in_correct_room = (sMed->roomID == survivalRooms[Survival::currentRoomIndex].roomID);
        bool entering_survival = !survival_active_last_frame && player_exists_now && player_is_alive && in_correct_room;
        
        // check if player died while in survival mode and in correct room
        if (player_existed_last_frame && player_exists_now && !player_is_alive && in_correct_room && Survival::survival_active) {
            
            // kill all enemies
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                uDamage_Old* currentEnemyDamage = (uDamage_Old*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                currentEnemyDamage->HP = 0.0f;
                enemy = enemy->nextEnemy;
            }
            MutatorHolyWater::use_hw_asm_call();
            RotatingLaser::kill_all();
        }
        
        if ((player_exists_now && !player_existed_last_frame) || 
            (player_existed_last_frame && (!player_exists_now || !player_is_alive))) {
            reset_wave();
        }
        
        player_existed_last_frame = player_exists_now && player_is_alive;
        
        if (timer) {
            if (player && player_is_alive) {
                if (!in_correct_room) {
                    Survival::survival_active = false;
                    pending_arc_load = true;
                    accumulated_delta += player->m_delta_time;
                    if (accumulated_delta >= teleport_delay) {
                        AreaJump::jump_to_stage(AreaJump::bp_stage(survivalRooms[Survival::currentRoomIndex].roomNumber));
                        accumulated_delta = 0.0f;
                    }
                }
                else { // Player is spawned and in the correct room
                    accumulated_delta = 0.0f;
                    Survival::survival_active = true;
                    if (entering_survival) {
                        if (*(int8_t*)0x8AFB3E != 6 /*&& *(int8_t*)0x8AFB76 != 2 && *(int8_t*)0x8AFBAE != 6*/) { // is player using more mem exe?
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em000", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em001", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em003", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em005", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em006", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em008", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em009", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em010", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em011", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em012", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em013", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em015", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em016", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                            devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\enemy\\em017", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                        }
                    }

                    sMed->bpTimer = survivedTimer;
                    DisplayTimerOnTick();
                    if (!devil4_sdk::is_paused()) { // game is not paused
                        sUnit* sUnit = devil4_sdk::get_sUnit();
                        if (sUnit && sUnit->mMoveLine[7].mTop) { // @Siy find how the bp timer gets time
                            uHasDelta* sUnitHasDelta = (uHasDelta*)sUnit->mMoveLine[7].mTop;
                            float game_seconds;
                            game_seconds = sUnitHasDelta->m_delta_time / 60.0f;
                            UpdateTimer();
                        }
                        EnemyInfo enemy_info = get_enemy_info(devil4_sdk::get_uEnemies());
                        if (enemy_info.enemies_alive == 0) {
                            timer->m_time = (fseconds)timer->m_duration; // trigger timer reset if the player killed all enemies too fast
                        }
                        float dante_seconds = player->m_delta_time / 60.0f;
                        if (timer) {
                            timer->tick((fmilliseconds)dante_seconds * 1000.0f);
                        }
                        if (basicPowerUpSystem) {
                            basicPowerUpSystem->on_frame(dt);
                        }
                        auto windowSize = devil4_sdk::get_sRender()->screenRes;
                        ImGui::SetNextWindowPos(ImVec2(windowSize.x * 0.19f, windowSize.y * 0.28f), NULL, ImVec2(0.0f, 0.0f));
                        ImGui::Begin("SurvivalWaveDisplay", NULL, 
                            ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration |
                            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
                        ImGui::UpdateCurrentFontSize(2.0f * ImGui::GetStyle().FontSizeBase);
                        const ImVec2 pos = ImGui::GetCursorPos();
                        // shadow
                        ImGui::SetCursorPos(ImVec2(pos.x + 2.0f, pos.y + 2.0f));
                        ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.7f), "WAVE %i", Survival::wave);
                        // text
                        ImGui::SetCursorPos(pos);
                        ImGui::TextColored(ImVec4(0.392f, 1.0f, 1.0f, 1.0f), "WAVE %i", Survival::wave);
                        ImGui::End();
                    }
                }
            }
            else {
                Survival::survival_active = false;
            }
        }
    }
}

enum class PowerupEffectID {
    BLUE_SPIN            = 0,
    NONE                 = 1,
    YELLOW_SPIN          = 2,
    PURPLE_SPIN          = 3,
    WHITEY_GREEN         = 4,
    GREENER_GREEN        = 5,
    BIGGER_GREENER_GREEN = 6,
    SMALL_PURPLE         = 7,
    BIGGER_PURPLE        = 8,
    SMALL_CYAN           = 9,
};

static PowerUpSystem::PowerUpDefinition createDoppelPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "doppel",                 // name
        "DPL",                    // displayName
        ImColor(255, 0, 0, 255),  // color (Red)
        (int)PowerupEffectID::SMALL_CYAN, // effectID
        15.0f,                    // duration
        200.0f,                   // radius
        15.0f,                    // effectDuration
        []() {                    // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                if (!GermanWord::mod_enabled) {
                    GermanWord::survival_doppel_enabled = true;
                    GermanWord::spawn_queued = true;
                }
            }
        },
        [](float dt) {            // onUpdate
        
        },                        
        []() {                    // onExpire
            GermanWord::survival_doppel_enabled = false;
        }
    );
}

static PowerUpSystem::PowerUpDefinition createHealthRestorePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "health_restore",         // name
        "HP",                     // displayName
        ImColor(0, 255, 0, 255),  // color (Green)
        (int)PowerupEffectID::BIGGER_GREENER_GREEN,
        15.0f,                    // duration
        200.0f,                   // radius
        0.0f,                     // effectDuration
        []() {                    // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->damageStruct.HP += std::min(3000.0f, 20000.0f - player->damageStruct.HP);
            }
        },
        [](float dt) {            // onUpdate
        
        },
        []() {                    // onExpire
        
        }
    );
}

static PowerUpSystem::PowerUpDefinition createDevilTriggerPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "devil_trigger",            // name
        "DT",                       // displayName
        ImColor(128, 0, 255, 255),  // color (Purple)
        (int)PowerupEffectID::BIGGER_PURPLE,
        15.0f,                      // duration
        200.0f,                     // radius
        0.0f,                       // effectDuration
        []() {                      // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->DT += std::min(3000.0f, 10000.0f - player->DT);
            }
        },
        [](float dt) {              // onUpdate
        
        },
        []() {                      // onExpire
        
        }
    );
}

static PowerUpSystem::PowerUpDefinition createQuicksilverPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "quicksilver",              // name
        "QS",                       // displayName
        ImColor(50, 50, 50, 255),   // color (Gray)
        (int)PowerupEffectID::YELLOW_SPIN,
        15.0f,                      // duration
        200.0f,                     // radius
        0.0f,                       // effectDuration
        []() {                      // onActivate
            if (!Quicksilver::get_timer()->m_active) {
                Quicksilver::get_timer()->start();
                Quicksilver::qs_operator_new();
            }
        },
        [](float dt) {              // onUpdate
        
        },
        []() {                      // onExpire

        }
    );
}

static PowerUpSystem::PowerUpDefinition createHolyWaterPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "holywater",                // name
        "HW",                       // displayName
        ImColor(0, 191, 255, 255),  // color (Blue)
        (int)PowerupEffectID::BLUE_SPIN,
        15.0f,                      // duration
        200.0f,                     // radius
        0.0f,                       // effectDuration
        []() {                      // onActivate
            MutatorHolyWater::use_hw_asm_call(); // has its own player check
        },
        [](float dt) {              // onUpdate
        
        },
        []() {                      // onExpire

        }
    );
}

static PowerUpSystem::PowerUpDefinition createPlayerSmolPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "player_smol",             // name
        "SMOL",                    // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        (int)PowerupEffectID::YELLOW_SPIN,
        15.0f,                     // duration
        200.0f,                    // radius
        30.0f,                     // effectDuration
        []() {                     // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->mScale = MtVector3{ 0.5f, 0.5f, 0.5f };
            }
        },
        [](float dt) {             // onUpdate
        
        },
        []() {                     // onExpire
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->mScale = MtVector3{ 1.0f, 1.0f, 1.0f };
            }
        }
    );
}

static PowerUpSystem::PowerUpDefinition createPlayerHeadPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "player_head",             // name
        "HEAD",                    // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        (int)PowerupEffectID::YELLOW_SPIN,
        15.0f,                     // duration
        200.0f,                    // radius
        30.0f,                     // effectDuration
        []() {                     // onActivate
            BigHeadMode::big_head_mode_nero = !BigHeadMode::big_head_mode_nero;
            BigHeadMode::big_head_mode_dante = !BigHeadMode::big_head_mode_dante;
        },
        [](float dt) {             // onUpdate
            // 
        },
        []() {                     // onExpire
            BigHeadMode::big_head_mode_nero = !BigHeadMode::big_head_mode_nero;
            BigHeadMode::big_head_mode_dante = !BigHeadMode::big_head_mode_dante;
        }
    );
}

static PowerUpSystem::PowerUpDefinition createEnemySizePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "enemy_size",              // name
        "ENMY_SZ",                 // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        (int)PowerupEffectID::PURPLE_SPIN,
        15.0f,                     // duration
        200.0f,                    // radius
        30.0f,                     // effectDuration
        []() {                     // onActivate
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                float newScale = Survival::get_random_float(0.5f, 2.0f);
                enemy->scale = { newScale, newScale, newScale };
                enemy = enemy->nextEnemy;
            }
        },
        [](float dt) {             // onUpdate
        
        },
        []() {                     // onExpire
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                enemy->scale = {1.0f, 1.0f, 1.0f}; // not all enemies are 1.0 but its too annoying enemies staying random sizes after powerup expires
                enemy = enemy->nextEnemy;
            }
        }
    );
}

static PowerUpSystem::PowerUpDefinition createStaminaPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "stamina",               // name
        "STAMINA",               // displayName
        ImColor(255, 255, 0, 0), // color (Yellow)
        (int)PowerupEffectID::YELLOW_SPIN,
        15.0f,  // duration
        200.0f, // radius
        15.0f,  // effectDuration
        []() {  // onActivate
            DarkSoulsStamina::meme_stamina_enabled = true;
        },
        [](float dt) { // onUpdate
        },
        []() { // onExpire
            DarkSoulsStamina::meme_stamina_enabled = false;
        });
}

static void setupBasicPowerUpSystem() {
    // basicPowerUpSystem->setSpawnInterval(0.0f);
    basicPowerUpSystem->setMaxPowerUps(5);
    PowerUpSystem::SpawnArea customArea = {
        Vector3f(0, 0, 0),  // centre
        1200.0f,            // radius // bp size
        100.0f,             // min height
        1200.0f             // max height
    };
    basicPowerUpSystem->setSpawnArea(customArea);
}

static void setupMemePowerUpSystem() {
    // memePowerUpSystem->setSpawnInterval(15.0f);
    memePowerUpSystem->setMaxPowerUps(5);
    uPlayer* player = devil4_sdk::get_local_player();
    PowerUpSystem::SpawnArea customArea = {
        (player ? player->mPos : Vector3f(0, 0, 0)),  // centre
        500.0f,             // radius
        0.0f,               // min height
        500.0f             // max height
    };
    memePowerUpSystem->setSpawnArea(customArea);
}

void Survival::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Survival"), &Survival::mod_enabled)) {
            if (!Survival::mod_enabled) {
                Survival::survival_active = false;
            }
            Survival::toggle(Survival::mod_enabled);
            basicPowerUpSystem->setEnabled(Survival::mod_enabled);
        }
        ImGui::SameLine();
        help_marker(_("Tick and enter any non BP mission on your desired difficulty"));

        if (Survival::mod_enabled) {
            ImGui::Indent(lineIndent);
            utility::ImGooListboxTranslated translated_names(survival_room_names.data(), survival_room_names.size());
            ImGui::SetNextItemWidth(sameLineItemWidth);
            if (ImGui::BeginCombo(_("Stage Select"), translated_names.data()[Survival::currentRoomIndex])) {
                for (uint32_t i = 0; i < survivalRooms.size(); i++) {
                    const auto& room = survivalRooms[i];
                    bool is_selected = (Survival::currentRoomIndex == i);
                    if (ImGui::Selectable(translated_names.data()[i], is_selected)) {
                        Survival::currentRoomIndex = i;
                    }
                    if (is_selected) {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();

        //ImGui::BeginGroup();
        ImGui::SameLine(sameLineWidth);
        if (ImGui::Checkbox(_("Random Meme Modifiers"), &meme_effects)) {
            Survival::meme_toggle(Survival::meme_effects);
            memePowerUpSystem->setEnabled(Survival::meme_effects);
        }
        ImGui::SameLine();
        help_marker(_("Spawn random meme modifier pickups spawned near your location to try to avoid (or target)"));
#ifndef NDEBUG
        if (mod_enabled) {
            if (ImGui::CollapsingHeader("[SURVIVAL DEBUG]")) {
                ImGui::Indent(lineIndent);
                if (timer)
                    ImGui::InputFloat("Spawn Timer", (float*)&timer->m_time);
                ImGui::InputInt("Wave", &Survival::wave);
                if (ImGui::Button("Spawn PowerUp")) {
                    basicPowerUpSystem->spawnRandomPowerUp();
                }
                if (ImGui::Button("Spawn Enemy")) {
                    Survival::spawn_standard_enemy();
                }
                if (ImGui::Button("Reset Wave")) {
                    Survival::reset_wave();
                }
                if (ImGui::Button("Timer Trigger")) {
                    Survival::on_timer_trigger();
                }

                if (ImGui::Button("Spawn Meme")) {
                    memePowerUpSystem->spawnRandomPowerUp();
                }
                if (meme_timer)
                    ImGui::InputFloat("Meme Timer", (float*)&meme_timer->m_time);

                static bool bla = false;
                if (ImGui::Button("Check if loading something")) {
                    bla = devil4_sdk::is_loading_arc();
                }
                ImGui::SameLine();
                ImGui::BeginDisabled();
                ImGui::Checkbox("##LoadingCheckbox", &bla);
                ImGui::EndDisabled();

                if (ImGui::Button("Create and Spawn Laser")) {
                    devil4_sdk::get_stuff_from_files((MtDTI*)0x00ead4a0, "rom\\room\\st405", MODE_BLOCKING | MODE_USECACHE | MODE_QUALITY_HIGHEST);
                    devil4_sdk::easy_spawn(0x8825D0, 10);
                }

                ImGui::Unindent(lineIndent);
                ImGui::Separator();
            }
        }
 #endif
        //ImGui::EndGroup();
    }
}

void Survival::reset_wave() {
    Survival::wave = 0;
    waves_since_boss = 0;
    if (timer) {
        timer->start();
        survivedTimer = 0.0f;
    }
    
    if (basicPowerUpSystem) {
        basicPowerUpSystem->clearPowerUps();
    }
}

int Survival::get_random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

float Survival::get_random_float(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void Survival::toggle(bool toggle) {
    if (toggle) {
        if (!Survival::timer) {
            Survival::timer = new utility::Timer(10.0f, Survival::on_timer_trigger);
        }
        Survival::timer->start();
        basicPowerUpSystem->registerPowerUp(createDoppelPowerUp());
        basicPowerUpSystem->registerPowerUp(createHealthRestorePowerUp());
        basicPowerUpSystem->registerPowerUp(createDevilTriggerPowerUp());
        basicPowerUpSystem->registerPowerUp(createQuicksilverPowerUp());
        basicPowerUpSystem->registerPowerUp(createHolyWaterPowerUp());
    } else {
        if (Survival::timer) {
            Survival::timer->stop();
        }
        basicPowerUpSystem->removePowerUp("doppel");
        basicPowerUpSystem->removePowerUp("health_restore");
        basicPowerUpSystem->removePowerUp("devil_trigger");
        basicPowerUpSystem->removePowerUp("quicksilver");
        basicPowerUpSystem->removePowerUp("holywater");
    }
}

void Survival::meme_toggle(bool toggle) {
    if (toggle) {
        if (!Survival::meme_timer) {
            Survival::meme_timer = new utility::Timer(10.0f, Survival::on_meme_timer_trigger);
        }
        Survival::meme_timer->start();
        memePowerUpSystem->registerPowerUp(createPlayerSmolPowerUp());
        memePowerUpSystem->registerPowerUp(createPlayerHeadPowerUp());
        memePowerUpSystem->registerPowerUp(createEnemySizePowerUp());
        memePowerUpSystem->registerPowerUp(createStaminaPowerUp());
    } else {
        if (Survival::meme_timer) {
            Survival::meme_timer->stop();
        }
        memePowerUpSystem->removePowerUp("player_smol");
        memePowerUpSystem->removePowerUp("player_head");
        memePowerUpSystem->removePowerUp("enemy_size");
        memePowerUpSystem->removePowerUp("stamina");
    }
}

void Survival::on_meme_timer_trigger() {
    if (meme_timer) {
        Survival::meme_timer->start();
    }

    // 1/2 chance of getting a powerup every wave
    if (get_random_int(0, 1) == 0) {
        if (memePowerUpSystem) { memePowerUpSystem->spawnRandomPowerUp(); }
    }
}

static uintptr_t detour_hp_alt_ret = 0x4FF015;
naked void detour_hp() {
    _asm {
        call edx
        cmp byte ptr [HideHud::mod_enabled_health], 1
        je alt_ret
        cmp byte ptr [Survival::survival_active], 1
        je cont
    // code:
        test al, al
        je alt_ret
    cont:
		jmp dword ptr [Survival::jmp_return_hp]

    alt_ret:
        jmp dword ptr [detour_hp_alt_ret]
    }
}

static uintptr_t detour_red_timer_alt_ret = 0x4FDF59;
naked void detour_red_timer() {
    _asm {
        cmp byte ptr [Survival::survival_active], 1
        je cont
    // code:
        cmp edi, 0x00000708
        jle jle_code
    cont:
		jmp dword ptr [Survival::jmp_return_red_timer]
    jle_code:
        jmp dword ptr [detour_red_timer_alt_ret]
    }
}

std::optional<std::string> Survival::on_initialize() {
    if (!install_hook_offset(0xFEFE1, hook_hp, &detour_hp, &jmp_return_hp, 6)) {
        spdlog::error("Failed to init Survival mod 1\n");
        return "Failed to init Survival mod 1";
    }
    if (!install_hook_offset(0xFDF4D, hook_red_timer, &detour_red_timer, &jmp_return_red_timer, 8)) {
        spdlog::error("Failed to init Survival mod 3\n");
        return "Failed to init Survival mod 3";
    }

    basicPowerUpSystem->on_initialize();
    if (basicPowerUpSystem) { setupBasicPowerUpSystem(); }

    memePowerUpSystem->on_initialize();
    if (memePowerUpSystem) { setupMemePowerUpSystem(); }

    return Mod::on_initialize();
}

void Survival::on_config_load(const utility::Config& cfg){
    Survival::mod_enabled = cfg.get<bool>("Survival").value_or(false);
    if (Survival::mod_enabled) {
        SpawnedEnemiesAttack::mod_enabled = Survival::mod_enabled;
        Survival::toggle(Survival::mod_enabled);
        basicPowerUpSystem->setEnabled(Survival::mod_enabled);
    }
    Survival::currentRoomIndex = cfg.get<int>("Survival_room").value_or(5);

    Survival::meme_effects = cfg.get<bool>("Survival_memes").value_or(false);
    if (Survival::meme_effects) {
        Survival::meme_toggle(Survival::meme_effects);
        memePowerUpSystem->setEnabled(Survival::meme_effects);
    }
    // window_pos.x = cfg.get<float>("survival_imgui_window_pos_x").value_or(0.0f);
    // window_pos.y = cfg.get<float>("survival_imgui_window_pos_y").value_or(0.0f);
}

void Survival::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("Survival", Survival::mod_enabled);
    cfg.set<int>("Survival_room", Survival::currentRoomIndex);
    cfg.set<bool>("Survival_memes", Survival::meme_effects);
    // cfg.set<float>("survival_imgui_window_pos_x", window_pos.x);
    // cfg.set<float>("survival_imgui_window_pos_y", window_pos.y);
}
