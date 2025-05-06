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
bool Survival::survival_active = false;
uintptr_t Survival::jmp_return_hp = NULL;
uintptr_t Survival::jmp_return_combat = NULL;
uintptr_t Survival::jmp_return_red_timer = NULL;

ImVec2 Survival::window_pos{ 0.0f, 0.0f };
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
static int waves_since_boss = false;

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
    if (timer) {
        Survival::timer->start();
    }
    SMediator* sMed = devil4_sdk::get_sMediator();
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
    uEnemy* enemy = devil4_sdk::get_uEnemies();
    EnemyInfo enemy_info = Survival::get_enemy_info(enemy);

    if (can_spawn_enemy(enemy_info, sMed)) {
        // Every enemy spawn is a new wave
        Survival::wave++;

        // 1/8 chance of spawning a "boss" enemy if one has not existed for 10 waves. Boss enemies are Blitz or the less annoying bosses
        if (Survival::wave >= 35 && waves_since_boss > 10 && get_random_int(0, 7) == 0) {
            spawn_boss_enemy();
            waves_since_boss = 0;
        }
        else {
            waves_since_boss++;
            // Boss was not spawned, spawn a dude
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
            return;
        }

        bool player_exists_now = (player != nullptr);
        if (player_exists_now && !player_existed_last_frame) {
            reset_wave();
        }
        player_existed_last_frame = player_exists_now;
        if (timer) {
            if (player) {
                if (sMed->roomID != 700) {
                    Survival::survival_active = false;
                    accumulated_delta += player->m_delta_time;
                    if (accumulated_delta >= teleport_delay) {
                        AreaJump::jump_to_stage(AreaJump::bp_stage(101));
                        accumulated_delta = 0.0f; // Reset accumulated delta
                    }
                }
                else { // Player is spawned and in the correct room
                    accumulated_delta = 0.0f;
                    Survival::survival_active = true;
                    sMed->bpTimer = survivedTimer;
                    DisplayTimerOnTick();
                    // ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                    // ImGui::Begin("SurvivalStats", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBackground);
                    // window_pos = ImGui::GetWindowPos();
                    // int totalMilliseconds = static_cast<int>(survivedTimer * 1000);
                    // int hours   = totalMilliseconds / (1000 * 60 * 60);
                    // int minutes = (totalMilliseconds / (1000 * 60)) % 60;
                    // int seconds = (totalMilliseconds / 1000) % 60;
                    // int millis  = totalMilliseconds % 1000;
                    // ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%02i:%02i:%02i.%03i", hours, minutes, seconds, millis);
                    // ImGui::End();
                    if (!devil4_sdk::is_paused()) { // game is not paused
                        sUnit* sUnit = devil4_sdk::get_sUnit();
                        if (sUnit && sUnit->hasDelta) { // @Siy find how the bp timer gets time
                            float game_seconds = sUnit->hasDelta->m_delta_time / 60.0f;
                            //survivedTimer += game_seconds;
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
                    }
                }
            }
            else {
                Survival::survival_active = false;
            }
        }
    }
}

void Survival::spawn_kinda_random_enemy() {
    std::vector<int> available_enemies;
    if (Survival::wave < 5) {
        available_enemies = {
            0, // SCARECROW_LEG
            1, // SCARECROW_ARM
            2, // SCARECROW_MEGA
        };
    }
    else if (Survival::wave < 20) {
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
    else if (Survival::wave < 50) {
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
            13, // BASILISK
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
            13, // BASILISK
            // 4
            6, // FAUST
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
        17, // CREDO
        18, // AGNUS
    };
    if (!available_enemies.empty()) {
        int random_index = get_random_int(0, available_enemies.size() - 1);
        int enemy_type = available_enemies[random_index];
        EnemySpawn::spawn_em00x(enemy_type);
    }
}

PowerUpSystem::PowerUpDefinition createDoppelgangerPowerUp() {
    return PowerUpSystem::createPowerUpDef(
        "doppelganger",           // name
        "DPL",                    // displayName
        ImColor(255, 0, 0, 255),  // color (Red)
        15.0f,                    // duration
        0.0f,                     // radius
        15.0f,                    // effectDuration
        []() {                    // onActivate
                                  
        },                        
        [](float dt) {            // onUpdate
                                  
        },                        
        []() {                    // onExpire
        
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
                player->HP += std::min(3000.0f, 20000.0f - player->HP);
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
        ImColor(0, 191, 255, 255),  // color (Blue)
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
        100.0f,             // min
        600.0f              // max
    };
    memePowerUpSystem->setSpawnArea(customArea);
}

void setupMemePowerUpSystem() {
    // memePowerUpSystem->setSpawnInterval(15.0f);
    memePowerUpSystem->setMaxPowerUps(5);
    PowerUpSystem::SpawnArea customArea = {
        Vector3f(0, 0, 0),  // centre
        0.0f,               // radius
        0.0f,               // minimum
        0.0f                // maximum
    };
    memePowerUpSystem->setSpawnArea(customArea);
}

void Survival::on_gui_frame(int display) {
    ImGui::BeginGroup();
    if (ImGui::Checkbox(_("Survival"), &Survival::mod_enabled)) {
        if (Survival::mod_enabled) {
            SpawnedEnemiesAttack::mod_enabled = Survival::mod_enabled;
        }
        else {
            Survival::survival_active = false;
        }
        Survival::toggle(Survival::mod_enabled);
        basicPowerUpSystem->setEnabled(Survival::mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Tick and enter any non BP mission on your desired difficulty"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Random Meme Modifiers"), &meme_effects)) {
        Survival::meme_toggle(Survival::meme_effects);
        memePowerUpSystem->setEnabled(Survival::meme_effects);
    }
    ImGui::SameLine();
    help_marker(_("Random meme modifiers applied while you play"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);

        if (timer)
        ImGui::InputFloat("Spawn Timer", (float*)&timer->m_time);
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
        if (meme_timer)
        ImGui::InputFloat("Meme Timer", (float*)&meme_timer->m_time);

        ImGui::Unindent(lineIndent);
        
    }
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
        // powerUpSystem->registerPowerUp(createDoppelgangerPowerUp());
        basicPowerUpSystem->registerPowerUp(createHealthRestorePowerUp());
        basicPowerUpSystem->registerPowerUp(createDevilTriggerPowerUp());
        basicPowerUpSystem->registerPowerUp(createQuicksilverPowerUp());
    } else {
        if (Survival::timer) {
            Survival::timer->stop();
        }
        // basicPowerUpSystem->removePowerUp("doppelganger");
        basicPowerUpSystem->removePowerUp("health_restore");
        basicPowerUpSystem->removePowerUp("devil_trigger");
        basicPowerUpSystem->removePowerUp("quicksilver");
    }
}

void Survival::meme_toggle(bool toggle) {
    if (toggle) {
        if (!Survival::meme_timer) {
            Survival::meme_timer = new utility::Timer(10.0f, Survival::on_meme_timer_trigger);
        }
        Survival::meme_timer->start();
        memePowerUpSystem->registerPowerUp(createPlayerSmolPowerUp());
        memePowerUpSystem->registerPowerUp(createEnemySizePowerUp());
    } else {
        if (Survival::meme_timer) {
            Survival::meme_timer->stop();
        }
        memePowerUpSystem->removePowerUp("player_smol");
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

naked void detour_combat() {
    _asm {
        cmp byte ptr [Survival::survival_active], 0
        je code
        mov al, 00
        jmp cont
    code:
        mov al, 01
    cont:
        pop esi
        mov esp,ebp
		jmp dword ptr [Survival::jmp_return_combat]
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
    if (!install_hook_offset(0xA667F, hook_combat, &detour_combat, &jmp_return_combat, 5)) {
        spdlog::error("Failed to init Survival mod 2\n");
        return "Failed to init Survival mod 2";
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

    Survival::meme_effects = cfg.get<bool>("Survival_memes").value_or(false);
    if (Survival::meme_effects) {
        Survival::meme_toggle(Survival::meme_effects);
        memePowerUpSystem->setEnabled(Survival::meme_effects);
    }
    window_pos.x = cfg.get<float>("survival_imgui_window_pos_x").value_or(0.0f);
    window_pos.y = cfg.get<float>("survival_imgui_window_pos_y").value_or(0.0f);
}

void Survival::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("Survival", Survival::mod_enabled);
    cfg.set<bool>("Survival_memes", Survival::meme_effects);
    cfg.set<float>("survival_imgui_window_pos_x", window_pos.x);
    cfg.set<float>("survival_imgui_window_pos_y", window_pos.y);
}
