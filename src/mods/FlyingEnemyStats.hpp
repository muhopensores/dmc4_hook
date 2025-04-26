#pragma once

#include "../mod.hpp"

class FlyingEnemyStats : public Mod {
public:
    FlyingEnemyStats() = default;
    
    static bool flyingEnemyStats;
    static bool showFlyingEnemyHP;
    static bool showFlyingEnemyDamageTaken;
    static bool showFlyingEnemyDT;
    static bool showFlyingEnemyStun;
    static bool showFlyingEnemyDisplacement;
    static bool showFlyingEnemyMoveID;
    static bool showFlyingEnemyMechanics;
    static bool showFlyingEnemyStunTimer;
    static bool showFlyingEnemyDebug;
    static bool showFlyingEnemyCollisionData;
    static int collisionPage;

    std::string get_mod_name() override { return "FlyingEnemyStats"; };

    std::optional<std::string> on_initialize() override;

    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;

private:
};
