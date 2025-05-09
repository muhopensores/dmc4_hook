#pragma once

#include "../mod.hpp"

class FlyingStats : public Mod {
public:
    FlyingStats() = default;
    
    static bool showFlyingStats;
    static bool showFlyingEnemyStats;
    static bool showFlyingPlayerStats;
    static bool showFlyingHP;
    static bool showFlyingDamageTaken;
    static bool showFlyingDamageResist;
    static bool showFlyingDT;
    static bool showFlyingStun;
    static bool showFlyingDisplacement;
    static bool showFlyingMoveID;
    static bool showFlyingMechanics;
    static bool showFlyingStunTimer;
    static bool showFlyingDebug;
    static bool showFlyingCollisionData;
    static int collisionPage;

    std::string get_mod_name() override { return "FlyingStats"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
};
