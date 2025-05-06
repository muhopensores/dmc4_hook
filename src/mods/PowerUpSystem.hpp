#pragma once
#include "../mod.hpp"

using PowerUpActivateCallback = std::function<void()>;
using PowerUpUpdateCallback = std::function<void(float)>;
using PowerUpExpireCallback = std::function<void()>;

class PowerUpSystem {
public:
    struct PowerUpDefinition {
        std::string name;
        std::string displayName;
        ImU32 colour;
        float duration; // How long the powerup stays in the world
        float radius; // Pickup radius
        float effectDuration; // How long the effect lasts (0 = instant)
        PowerUpActivateCallback onActivate;// Called when player picks up the powerup
        PowerUpUpdateCallback onUpdate; // Called every frame while effect is active
        PowerUpExpireCallback onExpire; // Called when effect duration ends
    };
    
    static PowerUpDefinition createPowerUpDef(
        const std::string& name,
        const std::string& displayName,
        ImU32 colour,
        float duration,
        float radius,
        float effectDuration,
        PowerUpActivateCallback onActivate,
        PowerUpUpdateCallback onUpdate,
        PowerUpExpireCallback onExpire
    ) {
        PowerUpDefinition def;
        def.name = name;
        def.displayName = displayName;
        def.colour = colour;
        def.duration = duration;
        def.radius = radius;
        def.effectDuration = effectDuration;
        def.onActivate = onActivate;
        def.onUpdate = onUpdate;
        def.onExpire = onExpire;
        return def;
    }
    
    struct PowerUp {
        std::string typeId; // Reference to the powerup definition
        float duration; // How long the powerup stays active (seconds)
        float remainingTime; // Time left before despawning
        Vector3f location; // Position in the world
        float radius; // Pickup radius
        bool active; // Whether the powerup is currently spawned
        bool effectActive; // Whether the player has the effect active
        float effectDuration; // How long the effect lasts (seconds)
        float effectTimeLeft; // Time left for the effect
        std::shared_ptr<utility::Timer> effectTimer; // Timer for this effect
    };
    
    struct SpawnArea {
        Vector3f centre;        // Centre position of spawn area
        float radius;           // Horizontal radius from centre
        float minHeight;        // Minimum height from centre.y
        float maxHeight;        // Maximum height from centre.y
    };
    
    PowerUpSystem();
    
    PowerUpSystem(const SpawnArea& spawnArea);
    
    ~PowerUpSystem();
    
    std::optional<std::string> on_initialize();
    void on_frame(fmilliseconds& dt);
    void render();
    void spawnRandomPowerUp();
    void spawnSpecificPowerUp(const std::string& typeId);
    void checkPlayerProximity();
    void applyPowerUpEffect(PowerUp& powerup);
    void clearPowerUps();
    void toggle(bool enable);
    
    bool isEnabled() const { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; toggle(enabled); }
    
    float getSpawnInterval() const { return m_spawnInterval; }
    
    void setMaxPowerUps(int max) { m_maxPowerUps = max; }
    int getMaxPowerUps() const { return m_maxPowerUps; }

    const SpawnArea& getSpawnArea() const { return m_spawnArea; }
    void setSpawnArea(const SpawnArea& spawnArea) { m_spawnArea = spawnArea; }
    
    void registerPowerUp(const PowerUpDefinition& definition);
    
    bool removePowerUp(const std::string& typeId);
    
    const PowerUpDefinition* getPowerUpDefinition(const std::string& typeId) const;
    
    std::vector<std::string> getAvailablePowerUpTypes() const;
    
    bool isPowerUpRegistered(const std::string& typeId) const;
    
    bool isEffectActive(const std::string& typeId) const;
    bool resetEffectTimer(const std::string& typeId, float newDuration = 0.0f);
    bool extendEffectTimer(const std::string& typeId, float additionalTime);
private:
    Vector3f getRandomPosition();
    int getRandomInt(int min, int max);
    float getRandomFloat(float min, float max);
    
    bool m_enabled;
    std::vector<PowerUp> m_powerUps;
    float m_spawnInterval;   // How often to spawn powerups (seconds)
    int m_maxPowerUps;       // Maximum number of powerups allowed at once
    float m_defaultDuration; // Default duration before despawn
    float m_defaultRadius;   // Default pickup radius
    
    SpawnArea m_spawnArea;
    
    std::unordered_map<std::string, PowerUpDefinition> m_powerUpDefinitions;
    
    std::mt19937 m_rng;
    std::random_device m_rd;
};
