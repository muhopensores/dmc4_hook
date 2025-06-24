#include "Survival.hpp"
#include "../sdk/Devil4.hpp"
#include "EnemySpawn.hpp"
#include "AreaJump.hpp"
#include "SpawnedEnemiesAttack.hpp"
#include "PowerUpSystem.hpp"
#include "EnemyTracker.hpp" // for enemy specific damage offset
#include "Quicksilver.hpp"
#include "MutatorHolyWater.hpp"
#include "BigHeadMode.hpp"
#include "HideHud.hpp"

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
    std::vector<EnemyType> standard_enemies;
    std::vector<EnemyType> side_enemies;
    std::vector<EnemyType> boss_enemies;
    WaveConfig(
        int _max_with_boss, int _max_without_boss,
        int _ldk_max_with_boss, int _ldk_max_without_boss,
        int _boss_cooldown, int _max_bosses,
        int _max_side_enemies,
        int _boss_chance, int _side_chance, int _powerup_chance,
        const std::vector<EnemyType>& _standard_enemies = {},
        const std::vector<EnemyType>& _side_enemies = {},
        const std::vector<EnemyType>& _boss_enemies = {}
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
        standard_enemies(_standard_enemies),
        side_enemies(_side_enemies),
        boss_enemies(_boss_enemies)
    {}
};

// Shame about the hitch when an enemy is loaded
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
std::random_device Survival::rd;
std::mt19937 Survival::rng(Survival::rd());
static std::unique_ptr<PowerUpSystem> basicPowerUpSystem = std::make_unique<PowerUpSystem>();
static std::unique_ptr<PowerUpSystem> memePowerUpSystem = std::make_unique<PowerUpSystem>();
static float accumulated_delta = 0.0f;
static const float teleport_delay = 50.0f;
static int waves_since_boss = 0;

static uintptr_t doppelAddr = NULL; // use this when spawning doppel dante or nero

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
Survival::EnemyInfo Survival::get_enemy_info(uEnemy* enemy) {
    EnemyInfo enemy_info{ 0, 0 };
    while (enemy) {
        uDamage* currentEnemyDamage = (uDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
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

const std::map<int, WaveConfig> WAVE_CONFIGS = {
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA}, // tier 1 enemies with duplicates for less chance of mega
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // Side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT}, // tier 2 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK}, // tier 3 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK}, // tier 3 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK, // tier 3 enemies
         EnemyType::FAUST}, // tier 4 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK, // tier 3 enemies
         EnemyType::FAUST}, // tier 4 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK, // tier 3 enemies
         EnemyType::FAUST}, // tier 4 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
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
        {EnemyType::SCARECROW_LEG, EnemyType::SCARECROW_ARM, EnemyType::SCARECROW_MEGA, // tier 1 enemies
         EnemyType::ANGELO_BIANCO, EnemyType::MEPHISTO, EnemyType::ASSAULT, // tier 2 enemies
         EnemyType::FROST, EnemyType::ANGELO_ALTO, EnemyType::BASILISK, // tier 3 enemies
         EnemyType::FAUST}, // tier 4 enemies
        {EnemyType::CHIMERA_SEED, EnemyType::CUTLASS, EnemyType::GLADIUS}, // side enemies
        {EnemyType::BLITZ, EnemyType::CREDO, EnemyType::BERIAL, EnemyType::BAEL} // Boss enemies
    )}
};

const WaveConfig& get_wave_config() {
    for (const auto& [wave_threshold, config] : WAVE_CONFIGS) {
        if (Survival::wave < wave_threshold) {
            return config;
        }
    }
    return WAVE_CONFIGS.rbegin()->second;
}

bool can_spawn_standard_enemy(const Survival::EnemyInfo& enemy_info, SMediator* sMed, const WaveConfig& config) {
    bool is_ldk = (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT);
        
    int max_enemies = is_ldk
        ? (enemy_info.bosses_alive > 0 ? config.ldk_max_enemies_with_boss : config.ldk_max_enemies_without_boss)
        : (enemy_info.bosses_alive > 0 ? config.max_enemies_with_boss : config.max_enemies_without_boss);
        
    return enemy_info.enemies_alive < max_enemies;
}

bool can_spawn_side_enemy(const Survival::EnemyInfo& enemy_info, const WaveConfig& config) {
    if (config.max_side_enemies <= 0 || config.side_enemy_spawn_chance <= 0) {
        return false;
    }
    
    return enemy_info.side_enemies_alive < config.max_side_enemies;
}

bool can_spawn_boss(const Survival::EnemyInfo& enemy_info, const WaveConfig& config) {
    if (config.max_bosses <= 0 || config.boss_spawn_chance <= 0) {
        return false;
    }
    
    if (config.boss_waves_cooldown> 0 && waves_since_boss <= config.boss_waves_cooldown) {
        return false;
    }
    
    return enemy_info.bosses_alive < config.max_bosses;
}

void spawn_enemy_from_pool(const std::vector<EnemyType>& enemy_pool) {
    if (!enemy_pool.empty()) {
        int random_index = Survival::get_random_int(0, enemy_pool.size() - 1);
        EnemyType enemy_type = enemy_pool[random_index];
        EnemySpawn::spawn_em00x(enemy_type);
    }
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
        
    SMediator* sMed = devil4_sdk::get_sMediator();
    if (!sMed) { return; }
        
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
        
    uEnemy* enemy = devil4_sdk::get_uEnemies();
    Survival::EnemyInfo enemy_info = Survival::get_enemy_info(enemy);
        
    const WaveConfig& config = get_wave_config();
    bool is_ldk = (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT);
    
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

uintptr_t DisplayTimerCall = 0x494EA0;
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
naked float UpdateTimer() {
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
            bool player_exists_now = (player != nullptr);
            if (player_exists_now && !player_existed_last_frame) {
                meme_timer->start();
            }
            player_existed_last_frame = player_exists_now;
            if (player_exists_now) {
                memePowerUpSystem->on_frame(dt);
                float dante_seconds = player->m_delta_time / 60.0f;
                meme_timer->tick((fmilliseconds)dante_seconds * 1000.0f);
            }
        }
    }
    if (Survival::mod_enabled) {
        SMediator* sMed = devil4_sdk::get_sMediator();
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        if (!sMed || sMed->missionID == 50 || !s_area_ptr) {
            Survival::survival_active = false;
            reset_wave();
            return;
        }
        
        bool player_exists_now = (player != nullptr);
        bool player_is_alive = player_exists_now && player->damageStruct.HP > 0.0f;
        bool in_correct_room = (sMed->roomID == survivalRooms[Survival::currentRoomIndex].roomID);
        
        // check if player died while in survival mode and in correct room
        if (player_existed_last_frame && player_exists_now && 
            !player_is_alive && in_correct_room && Survival::survival_active) {
            
            // kill all enemies
            uEnemy* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                uDamage* currentEnemyDamage = (uDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                currentEnemyDamage->HP = 0.0f;
                enemy = enemy->nextEnemy;
            }
            MutatorHolyWater::use_hw_asm_call();
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
                    accumulated_delta += player->m_delta_time;
                    if (accumulated_delta >= teleport_delay) {
                        AreaJump::jump_to_stage(AreaJump::bp_stage(survivalRooms[Survival::currentRoomIndex].roomNumber));
                        accumulated_delta = 0.0f;
                    }
                }
                else { // Player is spawned and in the correct room
                    accumulated_delta = 0.0f;
                    Survival::survival_active = true;
                    sMed->bpTimer = survivedTimer;
                    DisplayTimerOnTick();
                    if (!devil4_sdk::is_paused()) { // game is not paused
                        sUnit* sUnit = devil4_sdk::get_sUnit();
                        if (sUnit && sUnit->mMoveLine[7].mTop) { // @Siy find how the bp timer gets time
                            uHasDelta* sUnitHasDelta = (uHasDelta*)sUnit->mMoveLine[7].mTop;
                            float game_seconds = sUnitHasDelta->m_delta_time / 60.0f;
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
                        ImGui::SetWindowFontScale(2.0f);
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

static std::mutex g_mutex;
static constexpr uintptr_t danteSpawnAddr = 0x7B2130;
static uintptr_t some_struct = 0x00E552CC;
static uintptr_t fptr_update_actor_list = 0x008DC540;
void EnemySpawn::spawn_dante() {
    std::lock_guard<std::mutex> lk(g_mutex);
    if (!devil4_sdk::get_local_player()) return;
    __asm {
		pushad
		pushfd
        call dword ptr [danteSpawnAddr]
        mov [doppelAddr], eax
        mov esi, eax
        mov eax, [some_struct]
        mov eax, [eax]
        push 0x0F
        call fptr_update_actor_list
		popfd
		popad
    }
}

PowerUpSystem::PowerUpDefinition createDantePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "dante",                  // name
        "DNTE",                   // displayName
        ImColor(255, 0, 0, 255),  // color (Red)
        15.0f,                    // duration
        200.0f,                   // radius
        15.0f,                    // effectDuration
        []() {                    // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                if (!doppelAddr) {
                    EnemySpawn::spawn_dante(); // crashes if nero
                }
            }
        },
        [](float dt) {            // onUpdate
        
        },                        
        []() {                    // onExpire
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                uPlayer* doppel = (uPlayer*)doppelAddr;
                if (doppel) {
                    for (int i = 0; i < 15; i++) {
                        if (doppel->luciferPins[i]) {
                            uactor_sdk::despawn(doppel->luciferPins[i]);
                        }
                    }
                    uactor_sdk::despawn(doppel); // crashes if a shell is spawned, luci pins fixed, need to find funship missiles
                }
            }
            doppelAddr = NULL;
        }
    );
}

PowerUpSystem::PowerUpDefinition createHealthRestorePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "health_restore",         // name
        "HP",                     // displayName
        ImColor(0, 255, 0, 255),  // color (Green)
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

PowerUpSystem::PowerUpDefinition createDevilTriggerPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "devil_trigger",            // name
        "DT",                       // displayName
        ImColor(128, 0, 255, 255),  // color (Purple)
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

PowerUpSystem::PowerUpDefinition createQuicksilverPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "quicksilver",              // name
        "QS",                       // displayName
        ImColor(50, 50, 50, 255),   // color (Gray)
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

PowerUpSystem::PowerUpDefinition createHolyWaterPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "holywater",                // name
        "HW",                       // displayName
        ImColor(0, 191, 255, 255),  // color (Blue)
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

PowerUpSystem::PowerUpDefinition createPlayerSmolPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "player_smol",             // name
        "SMOL",                    // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        15.0f,                     // duration
        0.0f,                      // radius
        15.0f,                     // effectDuration
        []() {                     // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->m_scale = { 0.5f, 0.5f, 0.5f };
            }
        },
        [](float dt) {             // onUpdate
        
        },
        []() {                     // onExpire
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->m_scale = { 1.0f, 1.0f, 1.0f };
            }
        }
    );
}

PowerUpSystem::PowerUpDefinition createPlayerHeadPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "player_head",             // name
        "HEAD",                    // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        15.0f,                     // duration
        0.0f,                      // radius
        15.0f,                     // effectDuration
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

PowerUpSystem::PowerUpDefinition createEnemySizePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "enemy_size",              // name
        "ENMY_SZ",                 // displayName
        ImColor(255, 255, 0, 0),   // color (Yellow)
        15.0f,                     // duration
        0.0f,                      // radius
        15.0f,                     // effectDuration
        []() {                     // onActivate
            uEnemy* enemy = devil4_sdk::get_uEnemies();
            while (enemy) {
                float newScale = Survival::get_random_float(0.5f, 2.0f);
                enemy->scale = { newScale, newScale, newScale };
                enemy = enemy->nextEnemy;
            }
        },
        [](float dt) {             // onUpdate
        
        },
        []() {                     // onExpire

        }
    );
}

void setupBasicPowerUpSystem() {
    // basicPowerUpSystem->setSpawnInterval(0.0f);
    basicPowerUpSystem->setMaxPowerUps(5);
    PowerUpSystem::SpawnArea customArea = {
        Vector3f(0, 0, 0),  // centre
        1200.0f,            // radius
        100.0f,             // min height
        1200.0f              // max height
    };
    basicPowerUpSystem->setSpawnArea(customArea);
}

void setupMemePowerUpSystem() {
    // memePowerUpSystem->setSpawnInterval(15.0f);
    memePowerUpSystem->setMaxPowerUps(5);
    PowerUpSystem::SpawnArea customArea = {
        Vector3f(0, 0, 0),  // centre
        0.0f,               // radius
        0.0f,               // min height
        0.0f                // max height
    };
    memePowerUpSystem->setSpawnArea(customArea);
}

void Survival::on_gui_frame(int display) {
    ImGui::BeginGroup();
    if (ImGui::Checkbox(_("Survival"), &Survival::mod_enabled)) {
        if (!Survival::mod_enabled) { Survival::survival_active = false; }
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

    ImGui::BeginGroup();
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Random Meme Modifiers"), &meme_effects)) {
        Survival::meme_toggle(Survival::meme_effects);
        memePowerUpSystem->setEnabled(Survival::meme_effects);
    }
    ImGui::SameLine();
    help_marker(_("Random meme modifiers applied while you play"));
    /*
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        if (timer) ImGui::InputFloat("Spawn Timer", (float*)&timer->m_time);
        if (ImGui::Button("Spawn Player")) {
            EnemySpawn::spawn_player();
        }
        ImGui::Indent(lineIndent);
        ImGui::InputInt("Wave", &Survival::wave);
        if (ImGui::Button("Spawn PowerUp")) {
            basicPowerUpSystem->spawnRandomPowerUp();
        }
        if (ImGui::Button("Spawn Enemy")) {
            Survival::spawn_kinda_random_enemy();
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
        if (meme_timer) ImGui::InputFloat("Meme Timer", (float*)&meme_timer->m_time);

        ImGui::Unindent(lineIndent);

    }
    */
    ImGui::EndGroup();
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
        // basicPowerUpSystem->registerPowerUp(createDantePowerUp());
        basicPowerUpSystem->registerPowerUp(createHealthRestorePowerUp());
        basicPowerUpSystem->registerPowerUp(createDevilTriggerPowerUp());
        basicPowerUpSystem->registerPowerUp(createQuicksilverPowerUp());
        basicPowerUpSystem->registerPowerUp(createHolyWaterPowerUp());
    } else {
        if (Survival::timer) {
            Survival::timer->stop();
        }
        // basicPowerUpSystem->removePowerUp("dante");
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
    } else {
        if (Survival::meme_timer) {
            Survival::meme_timer->stop();
        }
        memePowerUpSystem->removePowerUp("player_smol");
        memePowerUpSystem->removePowerUp("player_head");
        memePowerUpSystem->removePowerUp("enemy_size");
    }
}

void Survival::on_meme_timer_trigger() {
    if (meme_timer) {
        Survival::meme_timer->start();
    }

    // 1/3 chance of getting a powerup every wave
    if (get_random_int(0, 2) == 0) {
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
