#pragma once
#include "../mod.hpp"

enum class PowerUpType {
    DOPPELGANGER = 0,
    HEALTH_RESTORE = 1,
    DEVIL_TRIGGER = 2,
    QUICKSILVER = 3,
    COUNT
};

class PowerUpSystem {
public:
    struct PowerUp {
        PowerUpType type;
        float duration;       // How long the powerup stays active (seconds)
        float remainingTime;  // Time left before despawning
        Vector3f location;    // Position in the world
        float radius;         // Pickup radius
        bool active;          // Whether the powerup is currently spawned
        bool effectActive;    // Whether the player has the effect active
        float effectDuration; // How long the effect lasts (seconds)
        float effectTimeLeft; // Time left for the effect
    };
    
    PowerUpSystem();
    ~PowerUpSystem();
    
    std::optional<std::string> on_initialize();
    void on_frame(fmilliseconds& dt);
    void on_timer_trigger();
    void render();
    void spawnRandomPowerUp();
    void checkPlayerProximity();
    void applyPowerUpEffect(PowerUp& powerup);
    void clearPowerUps();
    void toggle(bool enable);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; toggle(enabled); }
    
    void setSpawnInterval(float seconds) { 
        m_spawnInterval = seconds; 
        if (m_spawnTimer) {
            m_spawnTimer->m_duration = seconds;
        }
    }
    float getSpawnInterval() const { return m_spawnInterval; }
    
    void setMaxPowerUps(int max) { m_maxPowerUps = max; }
    int getMaxPowerUps() const { return m_maxPowerUps; }
    
private:
    Vector3f getRandomPosition();
    int getRandomInt(int min, int max);
    float getRandomFloat(float min, float max);
    
    bool m_enabled;
    std::vector<PowerUp> m_powerUps;
    std::shared_ptr<utility::Timer> m_spawnTimer;
    static utility::Timer* doppel_timer;

    float m_spawnInterval;   // How often to spawn powerups (seconds)
    int m_maxPowerUps;       // Maximum number of powerups allowed at once
    float m_defaultDuration; // Default duration before despawn
    float m_defaultRadius;   // Default pickup radius
    
    std::mt19937 m_rng;
    std::random_device m_rd;
    
    std::vector<ImU32> m_powerUpColors;
};
