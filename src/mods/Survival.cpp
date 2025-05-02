#include "Survival.hpp"
#include "../sdk/Devil4.hpp"
#include "EnemySpawn.hpp"
#include "AreaJump.hpp"
#include "SpawnedEnemiesAttack.hpp"
#include "PowerUpSystem.hpp"
#include "EnemyTracker.hpp" // for enemy specific damage offset
#include "Quicksilver.hpp"

// Shame about the hitch when an enemy is loaded
// Doppel is currently disabled because I couldn't figure out how to destroy p2, but the spawn and timer works (uh it sets you to p2 tho)
bool Survival::mod_enabled = false;
bool Survival::meme_effects = false;
std::shared_ptr<utility::Timer> Survival::timer;
int Survival::wave = 0;
bool Survival::player_existed_last_frame = false;
std::random_device Survival::rd;
std::mt19937 Survival::rng(Survival::rd());
static std::unique_ptr<PowerUpSystem> basicPowerUpSystem = std::make_unique<PowerUpSystem>();
static std::unique_ptr<PowerUpSystem> memePowerUpSystem = std::make_unique<PowerUpSystem>();

// safe to be called with no enemy
Survival::EnemyInfo Survival::get_enemy_info(uEnemy* enemy) {
    EnemyInfo enemy_info{ 0, false };
    while (enemy) {
        uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
        if (currentEnemyDamage->HP > 0.0f) {
            enemy_info.enemies_alive++;
            if (enemy->ID >= BERIAL || enemy->ID == BLITZ) {
                enemy_info.is_boss_spawned = true;
            }
        }
        enemy = enemy->nextEnemy;
    }
    return enemy_info;
}

bool Survival::can_spawn_enemy(EnemyInfo enemy_info, SMediator* sMed) {
    if (!sMed) { return false; }
    if (enemy_info.is_boss_spawned) { // if a boss is spawned, limit enemies
        if (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT) {
            if (enemy_info.enemies_alive < 3) {
                return true;
            }
        }
        else if (enemy_info.enemies_alive < 1) {
            return true;
        }
    }
    else { // if no boss is spawned, limit less
        if (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT) {
            if (enemy_info.enemies_alive < 10) {
                return true;
            }
        }
        else if (enemy_info.enemies_alive < 3) {
            return true;
        }
    }
    return false;
}

// When player is alive and in the correct room, this is called every x dante seconds or when every enemy is dead
void Survival::on_timer_trigger() {
    Survival::timer->start();
    SMediator* sMed = devil4_sdk::get_sMediator();
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
    uEnemy* enemy = devil4_sdk::get_uEnemies();
    EnemyInfo enemy_info = Survival::get_enemy_info(enemy);

    if (can_spawn_enemy(enemy_info, sMed)) {
        // Every enemy spawn is a new wave
        Survival::wave++;

        // 1/8 chance of spawning a "boss" enemy if one does not already exist. Boss enemies are Blitz or the less annoying bosses
        if (Survival::wave >= 35 && !enemy_info.is_boss_spawned && get_random_int(0, 7) == 0) {
            spawn_boss_enemy();
        }
        else {
            // Spawn a dude
            spawn_kinda_random_enemy();
            // spawn an extra dude every wave in LDK
            if (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT) {
                spawn_kinda_random_enemy();
            }
            // Above wave x, 1/4 chance of also spawning a "side" enemy. Side enemies are seeds swords, fish
            if (Survival::wave >= 20 && (get_random_int(0, 3) == 0)) spawn_side_enemy();
        }

        // 1/3 chance of getting a powerup every wave
        if (get_random_int(0, 2) == 0) {
            if (basicPowerUpSystem) { basicPowerUpSystem->spawnRandomPowerUp(); }
        }
        // 1/3 chance of getting a meme powerup every wave
        if (get_random_int(0, 2) == 0) {
            if (memePowerUpSystem) { memePowerUpSystem->spawnRandomPowerUp(); }
        }
    }
}

float accumulated_delta = 0.0f;
const float TELEPORT_DELAY = 50.0f;

void Survival::on_frame(fmilliseconds& dt) {
    if (!Survival::mod_enabled) { return; }
    SMediator* sMed = devil4_sdk::get_sMediator();
    if (!sMed) { return; }
    if (sMed->missionID == 50) { return; }
    uPlayer* player = devil4_sdk::get_local_player();
    bool player_exists_now = (player != nullptr);
    if (player_exists_now && !player_existed_last_frame) {
        reset_wave();
    }
    player_existed_last_frame = player_exists_now;
    
    if (player) {
        sArea* s_area_ptr = devil4_sdk::get_sArea();
        sUnit* sUnit = devil4_sdk::get_sUnit();
        sArea* sArea = devil4_sdk::get_sArea();
        if (sMed->roomID != 700) {
            accumulated_delta += player->m_delta_time;
            if (accumulated_delta >= TELEPORT_DELAY) {
                AreaJump::jump_to_stage(AreaJump::bp_stage(101));
                accumulated_delta = 0.0f; // Reset accumulated delta
            }
        }
        else { // Player is spawned and in the correct room
            accumulated_delta = 0.0f;
            if (!Survival::timer->m_active) {
                if (!timer) {
                    create_timer();
                }
                timer->start();
            }
            else { // timer is active
                if (!devil4_sdk::is_paused()) { // game is not paused
                    float dante_seconds = (player->m_delta_time / 60.0f) * 1000.0f;
                    timer->tick((fmilliseconds)dante_seconds);
                    EnemyInfo enemy_info = get_enemy_info(devil4_sdk::get_uEnemies());
                    if (enemy_info.enemies_alive == 0) {
                        timer->m_time = (fseconds)timer->m_duration; // trigger timer reset if the player killed all enemies too fast
                    }
                    if (basicPowerUpSystem) {
                        basicPowerUpSystem->on_frame(dt);
                    }
                    if (memePowerUpSystem) {
                        memePowerUpSystem->on_frame(dt);
                    }
                }
            }
        }
    }
}

void Survival::spawn_kinda_random_enemy() {
    std::vector<int> available_enemies;
    if (Survival::wave <= 5) {
        available_enemies = {
            0, // SCARECROW_LEG
            1, // SCARECROW_ARM
            2, // SCARECROW_MEGA
        };
    }
    else if (Survival::wave <= 19) {
        available_enemies = {
            0, // SCARECROW_LEG
            1, // SCARECROW_ARM
            2, // SCARECROW_MEGA
            // 2
            3, // ANGELO_BIANCO,
            5, // MEPHISTO,
            8, // ASSAULT
        };
    }
    else {
        available_enemies = {
            0, // SCARECROW_LEG
            1, // SCARECROW_ARM
            2, // SCARECROW_MEGA
            // 2
            3, // ANGELO_BIANCO
            5, // MEPHISTO
            8, // ASSAULT
            // 3
            7, // FROST
            4, // ANGELO_ALTO
            6, // FAUST
            13, // BASILISK
        };
    }

    if (!available_enemies.empty()) {
        int random_index = get_random_int(0, available_enemies.size() - 1);
        int enemy_type = available_enemies[random_index];
        EnemySpawn::spawn_em00x(enemy_type);
    }
}

void Survival::spawn_side_enemy() {
    std::vector<int> available_enemies = {
        10, // CHIMERA_SEED
        11, // CUTLASS
        12, // GLADIUS
    };

    if (!available_enemies.empty()) {
        int random_index = get_random_int(0, available_enemies.size() - 1);
        int enemy_type = available_enemies[random_index];
        EnemySpawn::spawn_em00x(enemy_type);
    }
}

void Survival::spawn_boss_enemy() {
    std::vector<int> available_enemies = {
        9, // BLITZ
        14, // BERIAL
        15, // BAEL
        17, // CREDO
        18, // AGNUS
    };
    if (!available_enemies.empty()) {
        int random_index = get_random_int(0, available_enemies.size() - 1);
        int enemy_type = available_enemies[random_index];
        EnemySpawn::spawn_em00x(enemy_type);
    }
}

PowerUpSystem::PowerUpDefinition createDoppelgangerPowerUp(float duration = 15.0f, float radius = 200.0f) {
    return PowerUpSystem::createPowerUpDef(
        "doppelganger",           // name
        "DPL",                    // displayName
        ImColor(255, 0, 0, 255),  // color (Red)
        duration,                 // duration
        radius,                   // radius
        15.0f,                    // effectDuration
        []() {                    // onActivate
                                  
        },                        
        [](float dt) {            // onUpdate
                                  
        },                        
        []() {                    // onExpire
        
        }
    );
}

PowerUpSystem::PowerUpDefinition createHealthRestorePowerUp(float duration = 15.0f, float radius = 200.0f) {
    return PowerUpSystem::createPowerUpDef(
        "health_restore",         // name
        "HP",                     // displayName
        ImColor(0, 255, 0, 255),  // color (Green)
        duration,                 // duration
        radius,                   // radius
        0.0f,                     // effectDuration (instant)
        []() {                    // onActivate
            uPlayer* player = devil4_sdk::get_local_player();
            if (player) {
                player->HP += std::min(3000.0f, 20000.0f - player->HP);
            }
        },
        [](float dt) {            // onUpdate
        
        },
        []() {                    // onExpire
        
        }
    );
}

PowerUpSystem::PowerUpDefinition createDevilTriggerPowerUp(float duration = 15.0f, float radius = 200.0f) {
    return PowerUpSystem::createPowerUpDef(
        "devil_trigger",            // name
        "DT",                       // displayName
        ImColor(128, 0, 255, 255),  // color (Purple)
        duration,                   // duration
        radius,                     // radius
        0.0f,                       // effectDuration (instant)
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

PowerUpSystem::PowerUpDefinition createQuicksilverPowerUp(float duration = 15.0f, float radius = 200.0f) {
    return PowerUpSystem::createPowerUpDef(
        "quicksilver",              // name
        "QS",                       // displayName
        ImColor(0, 191, 255, 255),  // color (Blue)
        duration,                   // duration
        radius,                     // radius
        15.0f,                      // effectDuration
        []() {                      // onActivate
            Quicksilver::qs_operator_new();
        },
        [](float dt) {              // onUpdate
        
        },
        []() {                      // onExpire
            Quicksilver::on_timer_callback();
        }
    );
}

PowerUpSystem::PowerUpDefinition createPlayerSmolPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "player_smol",             // name
        "SMOL",                    // displayName
        ImColor(255, 255, 0, 255), // color (Yellow)
        15.0f,                     // duration
        200.0f,                    // radius
        10.0f,                     // effectDuration
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

PowerUpSystem::PowerUpDefinition createEnemySizePowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "enemy_size",              // name
        "ENMY_SZ",                 // displayName
        ImColor(255, 255, 0, 255), // color (Yellow)
        15.0f,                     // duration
        200.0f,                    // radius
        10.0f,                     // effectDuration
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

void Survival::toggle_basic_powerups(bool toggle) {
    if (toggle) {
        // powerUpSystem->registerPowerUp(createDoppelgangerPowerUp());
        basicPowerUpSystem->registerPowerUp(createHealthRestorePowerUp());
        basicPowerUpSystem->registerPowerUp(createDevilTriggerPowerUp());
        basicPowerUpSystem->registerPowerUp(createQuicksilverPowerUp());
    }
    else {
        // powerUpSystem->removePowerUp("doppelganger");
        basicPowerUpSystem->removePowerUp("health_restore");
        basicPowerUpSystem->removePowerUp("devil_trigger");
        basicPowerUpSystem->removePowerUp("quicksilver");
    }
}

void Survival::toggle_meme_powerups(bool toggle) {
    if (toggle) {
        memePowerUpSystem->registerPowerUp(createPlayerSmolPowerUp());
        memePowerUpSystem->registerPowerUp(createEnemySizePowerUp());
    }
    else {
        memePowerUpSystem->removePowerUp("player_smol");
        memePowerUpSystem->removePowerUp("enemy_size");

    }
}

void setupBasicPowerUpSystem() {
    basicPowerUpSystem->setSpawnInterval(0.0f);
    basicPowerUpSystem->setMaxPowerUps(5);
}

void setupMemePowerUpSystem() {
    memePowerUpSystem->setSpawnInterval(0.0f);
    memePowerUpSystem->setMaxPowerUps(5);
}

void Survival::on_gui_frame(int display) {
    ImGui::BeginGroup();
    if (ImGui::Checkbox(_("Survival"), &Survival::mod_enabled)) {
        SpawnedEnemiesAttack::mod_enabled = Survival::mod_enabled;
        toggle(Survival::mod_enabled);
        basicPowerUpSystem->setEnabled(Survival::mod_enabled);
        toggle_basic_powerups(Survival::mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Tick and enter any non BP mission on your desired difficulty"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        if (ImGui::Checkbox(_("Extra Effects"), &meme_effects)) {
            memePowerUpSystem->setEnabled(Survival::mod_enabled);
            toggle_meme_powerups(Survival::meme_effects);
        }
    
        /*if (ImGui::Button("Spawn Player")) {
            EnemySpawn::spawn_player();
        }
        ImGui::Indent(lineIndent);
        ImGui::InputInt("Wave", &Survival::wave);
        ImGui::InputFloat("Wave Timer", (float*)&timer->m_time);
        float spawnInterval = powerUpSystem->getSpawnInterval();
        int maxPowerUps = powerUpSystem->getMaxPowerUps();
        if (ImGui::SliderInt("Max PowerUps", &maxPowerUps, 1, 10)) {
            powerUpSystem->setMaxPowerUps(maxPowerUps);
        }
        if (ImGui::Button("Spawn PowerUp")) {
            powerUpSystem->spawnRandomPowerUp();
        }
        if (ImGui::Button("Spawn Enemy")) {
            Survival::spawn_kinda_random_enemy();
        }
        if (ImGui::Button("Reset Wave")) {
            Survival::reset_wave();
        }
        if (ImGui::Button("Timer Trigger")) {
            Survival::on_timer_trigger();
        }*/
        ImGui::Unindent(lineIndent);
        
    }
    ImGui::EndGroup();
}

void Survival::reset_wave() {
    Survival::wave = 0;
    if (timer) {
        timer->start();
    }
    
    if (basicPowerUpSystem) {
        basicPowerUpSystem->clearPowerUps();
    }

    if (memePowerUpSystem) {
        memePowerUpSystem->clearPowerUps();
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
        if (!timer) {
            create_timer();
        }
        timer->start();
        reset_wave();
    } else {
        if (timer) {
            timer->stop();
        }
    }
}

void Survival::create_timer() {
    timer = std::make_shared<utility::Timer>(10.0f, Survival::on_timer_trigger);
}

std::optional<std::string> Survival::on_initialize() {
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
        toggle(Survival::mod_enabled);
        basicPowerUpSystem->setEnabled(Survival::mod_enabled);
        toggle_basic_powerups(Survival::mod_enabled);
    }
    Survival::meme_effects = cfg.get<bool>("Survival_memes").value_or(false);
    if (Survival::meme_effects) {
        memePowerUpSystem->setEnabled(Survival::mod_enabled);
        toggle_meme_powerups(Survival::meme_effects);
    }
}

void Survival::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("Survival", Survival::mod_enabled);
    cfg.set<bool>("Survival_memes", Survival::meme_effects);
}
