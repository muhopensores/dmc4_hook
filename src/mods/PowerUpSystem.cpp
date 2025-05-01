#include "PowerUpSystem.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/World2Screen.hpp"
#include "EnemySpawn.hpp"
#include "Quicksilver.hpp"

utility::Timer* PowerUpSystem::doppel_timer{};

PowerUpSystem::PowerUpSystem() : 
    m_enabled(false),
    m_spawnInterval(30.0f),
    m_maxPowerUps(5),
    m_defaultDuration(30.0f),
    m_defaultRadius(200.0f),
    m_rng(m_rd())
{
    m_powerUpColors.resize((size_t)PowerUpType::COUNT);
    m_powerUpColors[(size_t)PowerUpType::DOPPELGANGER] = ImColor(255, 0, 0, 255);  // Red
    m_powerUpColors[(size_t)PowerUpType::HEALTH_RESTORE] = ImColor(0, 255, 0, 255);  // Green
    m_powerUpColors[(size_t)PowerUpType::DEVIL_TRIGGER] = ImColor(128, 0, 255, 255); // Purple
    m_powerUpColors[(size_t)PowerUpType::QUICKSILVER] = ImColor(0, 191, 255, 255);  // Blue
}

PowerUpSystem::~PowerUpSystem() {}

void PowerUpSystem::on_timer_trigger() {
    if (m_powerUps.size() < (size_t)m_maxPowerUps && m_enabled) {
        spawnRandomPowerUp();
    }
}

void doppel_callback() {
    // sUnit* sUnit = devil4_sdk::get_sUnit();
    // if (!sUnit) { return; }
    // uPlayer* sparePlayer = sUnit->player;
    // if (!sparePlayer) { return; }
    // uactor_sdk::uDevil4ModelDest(sparePlayer);
}

std::optional<std::string> PowerUpSystem::on_initialize() {
    m_spawnTimer = std::make_shared<utility::Timer>(
        m_spawnInterval,
        [this]() { on_timer_trigger(); }
    );
    doppel_timer = new utility::Timer(15.0f, doppel_callback);
    return std::nullopt;
}

void PowerUpSystem::on_frame(fmilliseconds& dt) {
    if (!m_enabled) return;
    
    if (m_spawnTimer) {
        m_spawnTimer->tick(dt);
    }

    if (doppel_timer) {
        doppel_timer->tick(dt);
    }
    
    float dtSeconds = dt.count() / 1000.0f;
    
    for (auto it = m_powerUps.begin(); it != m_powerUps.end();) {
        auto& powerup = *it;
        
        if (!powerup.effectActive) {
            powerup.remainingTime -= dtSeconds;
            if (powerup.remainingTime <= 0) {
                it = m_powerUps.erase(it);
                continue;
            }
        } else {
            powerup.effectTimeLeft -= dtSeconds;
            if (powerup.effectTimeLeft <= 0) {
                powerup.effectActive = false;
                powerup.active = false;
                it = m_powerUps.erase(it);
                continue;
            }
        }
        
        ++it;
    }
    
    checkPlayerProximity();
    
    render();
}

void PowerUpSystem::render() {
    if (!m_enabled) return;
    
    for (const auto& powerup : m_powerUps) {
        if (!powerup.active || powerup.effectActive) continue;
        
        glm::vec3 realPowerupPos(powerup.location.x, powerup.location.y, powerup.location.z);
        // lie about actual pos by 200 y so you can hit it with your body rather than feet
        glm::vec3 powerupPos(powerup.location.x, powerup.location.y + 100.0f, powerup.location.z);
        
        if (w2s::IsVisibleOnScreen(powerupPos, powerup.radius)) {
            glm::vec2 screenPos = w2s::WorldToScreen(powerupPos);
            
            ImU32 color = m_powerUpColors[(size_t)powerup.type];
            ImU32 outlineColor = ImColor(255, 255, 255, 200);
            
            float distance = w2s::GetDistanceFromCam(powerupPos);
            float baseSize = 30.0f;
            float sizeScale = std::max(0.3f, std::min(1.0f, 50.0f / distance));
            float size = baseSize * sizeScale;
            
            ImGui::GetBackgroundDrawList()->AddCircleFilled(
                ImVec2(screenPos.x, screenPos.y),
                size,
                color
            );
            
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(screenPos.x, screenPos.y),
                size + 1.0f, 
                outlineColor,
                0,
                1.0f
            );
            
            //                 size    pulse                                     speed
            float pulseScale = 0.45f + 0.05f * std::sin((float)ImGui::GetTime() * 2.0f); // Pulsing effect
            float sphereRadius = powerup.radius * pulseScale;
            
            w2s::DrawWireframeSphere(
                powerupPos,
                sphereRadius,
                (float)ImGui::GetTime(), // Rotation animation
                color,
                12,
                1.5f
            );
            /*const char* labelText = "";
            switch (powerup.type) {
                case PowerUpType::DOPPELGANGER: labelText = "DPL"; break;
                case PowerUpType::HEALTH_RESTORE: labelText = "HP"; break;
                case PowerUpType::DEVIL_TRIGGER: labelText = "DT"; break;
                case PowerUpType::QUICKSILVER: labelText = "QS"; break;
                default: labelText = "???"; break;
            }
            
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(screenPos.x - ImGui::CalcTextSize(labelText).x / 2.0f, 
                       screenPos.y - size - 20.0f),
                ImColor(255, 255, 255, 255),
                labelText
            );*/
        }
    }
}

void PowerUpSystem::spawnRandomPowerUp() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) return;
    
    PowerUp newPowerUp;
    newPowerUp.type = (PowerUpType)getRandomInt(0, (int)PowerUpType::COUNT - 1);
    newPowerUp.duration = m_defaultDuration;
    newPowerUp.remainingTime = newPowerUp.duration;
    newPowerUp.location = getRandomPosition();
    newPowerUp.radius = m_defaultRadius;
    newPowerUp.active = true;
    newPowerUp.effectActive = false;
    
    switch (newPowerUp.type) {
        case PowerUpType::DOPPELGANGER:
            newPowerUp.effectDuration = 15.0f;
            break;
        case PowerUpType::HEALTH_RESTORE:
            newPowerUp.effectDuration = 0.0f;
            break;
        case PowerUpType::DEVIL_TRIGGER:
            newPowerUp.effectDuration = 0.0f;
            break;
        case PowerUpType::QUICKSILVER:
            newPowerUp.effectDuration = 15.0f;
            break;
        default:
            newPowerUp.effectDuration = 0.0f;
            break;
    }
    
    newPowerUp.effectTimeLeft = newPowerUp.effectDuration;
    m_powerUps.push_back(newPowerUp);
}

void PowerUpSystem::checkPlayerProximity() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) return;
    
    Vector3f playerPos = player->m_pos;
    
    for (auto& powerup : m_powerUps) {
        if (!powerup.active || powerup.effectActive) continue;
        
        float distance = glm::distance(
            glm::vec3(playerPos.x, playerPos.y, playerPos.z),
            glm::vec3(powerup.location.x, powerup.location.y, powerup.location.z)
        );
        
        if (distance <= powerup.radius) {
            applyPowerUpEffect(powerup);
        }
    }
}

void PowerUpSystem::applyPowerUpEffect(PowerUp& powerup) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    sUnit* sUnit = devil4_sdk::get_sUnit();
    if (!sUnit) { return; }
    
    powerup.effectActive = true;
    
    switch (powerup.type) {
        case PowerUpType::DOPPELGANGER:
            // if (!sparePlayer) {
            //     EnemySpawn::spawn_player();
            // }
            // doppel_timer->start();
            break;
            
        case PowerUpType::HEALTH_RESTORE:
            player->DT += std::min(3000.0f, 20000.0f - player->HP);
            powerup.effectActive = false; // One-time effect
            powerup.active = false;
            break;
            
        case PowerUpType::DEVIL_TRIGGER:
            player->DT += std::min(3000.0f, 10000.0f - player->DT);
            powerup.effectActive = false;
            powerup.active = false;
            break;
            
        case PowerUpType::QUICKSILVER:
            if (Quicksilver::get_timer()) {
				if (Quicksilver::get_timer()->m_active == false) {
					Quicksilver::qs_operator_new();
				}
		        Quicksilver::get_timer()->start();
			}
            break;
            
        default:
            powerup.effectActive = false;
            powerup.active = false;
            break;
    }
}

void PowerUpSystem::clearPowerUps() {
    m_powerUps.clear();
}

void PowerUpSystem::toggle(bool enable) {
    m_enabled = enable;
    
    if (m_enabled) {
        if (!m_spawnTimer) {
            on_initialize();
        }
        m_spawnTimer->start();
    } else {
        if (m_spawnTimer) {
            m_spawnTimer->stop();
        }
    }
}

Vector3f PowerUpSystem::getRandomPosition() {
    Vector3f basePos = Vector3f(0, 0, 0);
    
    float radius = 1200.0f;
    float angle = getRandomFloat(0, 2.0f * 3.14159f);
    float distance = getRandomFloat(5.0f, radius);
    float height = getRandomFloat(0.0f, 600.0f);
    
    return Vector3f(
        basePos.x + std::cos(angle) * distance,
        basePos.y + height,
        basePos.z + std::sin(angle) * distance
    );
}

int PowerUpSystem::getRandomInt(int min, int max) {
    std::uniform_int_distribution<> dist(min, max);
    return dist(m_rng);
}

float PowerUpSystem::getRandomFloat(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_rng);
}
