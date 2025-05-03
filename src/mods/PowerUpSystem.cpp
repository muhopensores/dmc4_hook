#include "PowerUpSystem.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/World2Screen.hpp"
#include "EnemySpawn.hpp"
#include "Quicksilver.hpp"

PowerUpSystem::PowerUpSystem() : 
    m_enabled(false),
    m_spawnInterval(15.0f),
    m_maxPowerUps(5),
    m_defaultDuration(15.0f),
    m_defaultRadius(200.0f),
    m_rng(m_rd())
{
    m_spawnArea = {
        Vector3f(0, 0, 0),  // centre
        1200.0f,            // radius
        0.0f,               // min
        600.0f              // max
    };
}

PowerUpSystem::PowerUpSystem(const SpawnArea& spawnArea) : 
    m_enabled(false),
    m_spawnInterval(15.0f),
    m_maxPowerUps(5),
    m_defaultDuration(15.0f),
    m_defaultRadius(200.0f),
    m_spawnArea(spawnArea),
    m_rng(m_rd())
{}

PowerUpSystem::~PowerUpSystem() {
    clearPowerUps();
}

void PowerUpSystem::on_timer_trigger() {
    if (m_powerUps.size() < (size_t)m_maxPowerUps && m_enabled) {
        spawnRandomPowerUp();
    }
}

std::optional<std::string> PowerUpSystem::on_initialize() {
    m_spawnTimer = std::make_shared<utility::Timer>(
        m_spawnInterval,
        [this]() { on_timer_trigger(); }
    );
    
    return std::nullopt;
}

void PowerUpSystem::on_frame(fmilliseconds& dt) {
    if (!m_enabled) return;
    
    if (m_spawnTimer) {
        m_spawnTimer->tick(dt);
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
            
            // Call update callback while effect is active
            const PowerUpDefinition* def = getPowerUpDefinition(powerup.typeId);
            if (def && def->onUpdate) {
                def->onUpdate(dtSeconds);
            }
            
            if (powerup.effectTimeLeft <= 0) {
                powerup.effectActive = false;
                powerup.active = false;
                
                // Call expire callback
                if (def && def->onExpire) {
                    def->onExpire();
                }
                
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
        // lie about actual pos by 100 y so you can hit it with your body rather than feet
        glm::vec3 powerupPos(powerup.location.x, powerup.location.y + 100.0f, powerup.location.z);
        
        if (w2s::IsVisibleOnScreen(powerupPos, powerup.radius)) {
            glm::vec2 screenPos = w2s::WorldToScreen(powerupPos);
            
            const PowerUpDefinition* def = getPowerUpDefinition(powerup.typeId);
            if (!def) continue;
            
            ImU32 colour = def->colour;
            ImU32 outlinecolour = ImColor(255, 255, 255, 200);
            
            float distance = w2s::GetDistanceFromCam(powerupPos);
            float baseSize = 30.0f;
            float sizeScale = std::max(0.3f, std::min(1.0f, 50.0f / distance));
            float size = baseSize * sizeScale;
            
            ImGui::GetBackgroundDrawList()->AddCircleFilled(
                ImVec2(screenPos.x, screenPos.y),
                size,
                colour
            );
            
            ImGui::GetBackgroundDrawList()->AddCircle(
                ImVec2(screenPos.x, screenPos.y),
                size + 1.0f, 
                outlinecolour,
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
                colour,
                12,
                1.5f
            );
            
            ImGui::GetBackgroundDrawList()->AddText(
                ImVec2(screenPos.x - ImGui::CalcTextSize(def->displayName.c_str()).x / 2.0f, 
                       screenPos.y - size - 20.0f),
                ImColor(255, 255, 255, 255),
                def->displayName.c_str()
            );
        }
    }
}

void PowerUpSystem::spawnRandomPowerUp() {
    // Get all available powerup types
    std::vector<std::string> availableTypes = getAvailablePowerUpTypes();
    if (availableTypes.empty()) return;
    
    // Select a random powerup type
    std::string selectedType = availableTypes[getRandomInt(0, availableTypes.size() - 1)];
    spawnSpecificPowerUp(selectedType);
}

void PowerUpSystem::spawnSpecificPowerUp(const std::string& typeId) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) return;
    
    const PowerUpDefinition* def = getPowerUpDefinition(typeId);
    if (!def) return;
    
    PowerUp newPowerUp;
    newPowerUp.typeId = typeId;
    newPowerUp.duration = def->duration;
    newPowerUp.remainingTime = newPowerUp.duration;
    newPowerUp.location = getRandomPosition();
    newPowerUp.radius = def->radius;
    newPowerUp.active = true;
    newPowerUp.effectActive = false;
    newPowerUp.effectDuration = def->effectDuration;
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
    const PowerUpDefinition* def = getPowerUpDefinition(powerup.typeId);
    if (!def) return;
    
    std::string currentTypeId = powerup.typeId;
    
    if (def->onActivate) {
        def->onActivate();
    }
    
    powerup.effectActive = true;
    
    if (powerup.effectDuration <= 0.0f) {
        powerup.effectActive = false;
        powerup.active = false;
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
    float angle = getRandomFloat(0, 2.0f * 3.14159f);
    float distance = getRandomFloat(5.0f, m_spawnArea.radius);
    float height = getRandomFloat(m_spawnArea.minHeight, m_spawnArea.maxHeight);
    
    return Vector3f(
        m_spawnArea.centre.x + std::cos(angle) * distance,
        m_spawnArea.centre.y + height,
        m_spawnArea.centre.z + std::sin(angle) * distance
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

void PowerUpSystem::registerPowerUp(const PowerUpDefinition& definition) {
    m_powerUpDefinitions[definition.name] = definition;
}

bool PowerUpSystem::removePowerUp(const std::string& typeId) {
    auto it = m_powerUpDefinitions.find(typeId);
    if (it != m_powerUpDefinitions.end()) {
        m_powerUpDefinitions.erase(it);
        
        // Also remove any spawned powerups of this type
        for (auto powerupIt = m_powerUps.begin(); powerupIt != m_powerUps.end();) {
            if (powerupIt->typeId == typeId) {
                powerupIt = m_powerUps.erase(powerupIt);
            } else {
                ++powerupIt;
            }
        }
        
        return true;
    }
    return false;
}

const PowerUpSystem::PowerUpDefinition* PowerUpSystem::getPowerUpDefinition(const std::string& typeId) const {
    auto it = m_powerUpDefinitions.find(typeId);
    if (it != m_powerUpDefinitions.end()) {
        return &(it->second);
    }
    return nullptr;
}

std::vector<std::string> PowerUpSystem::getAvailablePowerUpTypes() const {
    std::vector<std::string> types;
    for (const auto& pair : m_powerUpDefinitions) {
        types.push_back(pair.first);
    }
    return types;
}

bool PowerUpSystem::isPowerUpRegistered(const std::string& typeId) const {
    return m_powerUpDefinitions.find(typeId) != m_powerUpDefinitions.end();
}

bool PowerUpSystem::isEffectActive(const std::string& typeId) const {
    for (const auto& powerup : m_powerUps) {
        if (powerup.typeId == typeId && powerup.effectActive) {
            return true;
        }
    }
    return false;
}

bool PowerUpSystem::resetEffectTimer(const std::string& typeId, float newDuration) {
    for (auto& powerup : m_powerUps) {
        if (powerup.typeId == typeId && powerup.effectActive) {
            float duration = (newDuration > 0.0f) ? newDuration : powerup.effectDuration;
            
            powerup.effectTimeLeft = duration;
            
            return true;
        }
    }
    return false;
}

bool PowerUpSystem::extendEffectTimer(const std::string& typeId, float additionalTime) {
    for (auto& powerup : m_powerUps) {
        if (powerup.typeId == typeId && powerup.effectActive) {
            // Add the additional time to the remaining time
            powerup.effectTimeLeft += additionalTime;
            
            return true;
        }
    }
    return false;
}