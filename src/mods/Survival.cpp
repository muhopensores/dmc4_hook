#include "Survival.hpp"
#include "../sdk/Devil4.hpp"
#include "EnemySpawn.hpp"
#include "AreaJump.hpp"
#include "SpawnedEnemiesAttack.hpp"
#include "PowerUpSystem.hpp"
#include "EnemyTracker.hpp" // for enemy specific damage offset

// Enemy spawning isn't really reliable enough for this yet, you crash sometimes when an enemy spawns after fptr_update_actor_list call
// Doppel is currently disabled because I couldn't figure out how to destroy p2, but the spawn and timer works (uh it sets you to p2 tho)

bool Survival::mod_enabled = false;
bool Survival::special_effects = false;
std::shared_ptr<utility::Timer> Survival::timer;
int Survival::wave = 0;
int Survival::wave_difficulty = 0;
bool Survival::player_existed_last_frame = false;
std::random_device Survival::rd;
std::mt19937 Survival::rng(Survival::rd());
static std::unique_ptr<PowerUpSystem> powerUpSystem = std::make_unique<PowerUpSystem>();

bool Survival::can_spawn_enemy() {
    SMediator* sMed = devil4_sdk::get_sMediator();
    int enemyCount = devil4_sdk::get_enemy_count();
    if (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT) {
        if (enemyCount < 10) {
            return true;
        }
    }
    else if (enemyCount < 3) {
        return true;
    }
    return false;
}

// Called every x dante seconds or when every enemy is dead, only when player is alive and in the correct room
void Survival::on_timer_trigger() {
    Survival::timer->start();
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
    
    SMediator* sMed = devil4_sdk::get_sMediator();
    if (can_spawn_enemy()) {
        // Every 5 waves, go up a wave_difficulty. Higher wave difficulties have harder enemies
        Survival::wave++;
        if (wave % 5 == 0) Survival::wave_difficulty++;
        spawn_kinda_random_enemy();
        if (sMed->gameDifficulty == GameDifficulty::LEGENDARY_DARK_KNIGHT)
            spawn_kinda_random_enemy(); // spawn an extra dude every wave in LDK
        // Above a certain difficulty, 1/4 chance of spawning a "side" enemy. Side enemies are things like seeds or swords
        if (Survival::wave_difficulty >= 4 && (get_random_int(0, 3) == 0)) spawn_side_enemy();
        // 1/4 chance of getting a powerup every wave
        if (get_random_int(0, 3) == 0) {
            powerUpSystem->spawnRandomPowerUp();
        }
    }
}

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
        if (sMed->roomID != 700) {
            AreaJump::jump_to_stage(AreaJump::bp_stage(101)); // if not in bp 101, tele there
        }
        else { // Player is spawned and in the correct room
            if (!Survival::timer->m_active) {
                if (!timer) {
                    create_timer();
                }
                timer->start();
            }
            else {
                if (sUnit->enemy) {
                    if (!sUnit->enemy->nextEnemy) { // last enemy alive
                        uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)sUnit->enemy + EnemyTracker::get_enemy_specific_damage_offset(sUnit->enemy->ID));
                        if (currentEnemyDamage->HP <= 0.0f) { // has 0 hp
                            Survival::on_timer_trigger();
                        }
                    }
                }
                else { // no enemy exists
                    Survival::on_timer_trigger();
                }
                if (!devil4_sdk::is_paused()) {
                    float dante_seconds = (player->m_delta_time / 60.0f) * 1000.0f;
                    timer->tick((fmilliseconds)dante_seconds);
                }
            }
        }
    }
    if (powerUpSystem) {
        powerUpSystem->on_frame(dt);
    }
}

void Survival::spawn_kinda_random_enemy() {
    std::vector<int> available_enemies;
    if (Survival::wave_difficulty == 0) {
        available_enemies = {
            0, // SCARECROW_LEG
            1, // SCARECROW_ARM
            2, // SCARECROW_MEGA
        };
    }
    else if (Survival::wave_difficulty == 1) {
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
    else if (Survival::wave_difficulty == 2) {
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
            // 4
            9, // BLITZ
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

void Survival::on_gui_frame(int display) {
    ImGui::BeginGroup();
    if (ImGui::Checkbox(_("Survival"), &Survival::mod_enabled)) {
        SpawnedEnemiesAttack::mod_enabled = Survival::mod_enabled;
        toggle(Survival::mod_enabled);
        powerUpSystem->setEnabled(Survival::mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Tick and enter any non BP mission on your desired difficulty"));

    /*if (ImGui::Button("Spawn Player")) {
        EnemySpawn::spawn_player();
    }
    if (Survival::mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::InputInt("Wave", &Survival::wave);
        ImGui::InputInt("Wave Difficulty", &Survival::wave_difficulty);
        ImGui::InputFloat("Wave Timer", (float*)&timer->m_time);
        ImGui::Text("PowerUp System");
        float spawnInterval = powerUpSystem->getSpawnInterval();
        if (ImGui::SliderFloat("PowerUp Natural Spawn Interval", &spawnInterval, 10.0f, 60.0f, "%.1f")) {
            powerUpSystem->setSpawnInterval(spawnInterval);
        }
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
        }
        ImGui::Unindent(lineIndent);
    }*/
    ImGui::EndGroup();
}

void Survival::reset_wave() {
    Survival::wave = 0;
    Survival::wave_difficulty = 0;
    if (timer) {
        timer->start();
    }
    
    if (powerUpSystem) {
        powerUpSystem->clearPowerUps();
    }
}

int Survival::get_random_int(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

void Survival::change_something() {
    int thing = get_random_int(0, 1);
}

void Survival::toggle(bool enable) {
    if (enable) {
        if (!timer) {
            create_timer();
        }
        timer->start();
        reset_wave();
        
        if (powerUpSystem) {
            powerUpSystem->setEnabled(true);
            powerUpSystem->setSpawnInterval(0.0f);
        }
    } else {
        if (timer) {
            timer->stop();
        }
        
        if (powerUpSystem) {
            powerUpSystem->setEnabled(false);
        }
    }
}

void Survival::create_timer() {
    timer = std::make_shared<utility::Timer>(10.0f, Survival::on_timer_trigger);
}

std::optional<std::string> Survival::on_initialize() {
    powerUpSystem->on_initialize();
    return Mod::on_initialize();
}

void Survival::on_config_load(const utility::Config& cfg){
    Survival::mod_enabled = cfg.get<bool>("Survival").value_or(false);
    if (Survival::mod_enabled) {
        SpawnedEnemiesAttack::mod_enabled = Survival::mod_enabled;
        powerUpSystem->setEnabled(Survival::mod_enabled);
        toggle(Survival::mod_enabled);
    }
}

void Survival::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("Survival", Survival::mod_enabled);
}
