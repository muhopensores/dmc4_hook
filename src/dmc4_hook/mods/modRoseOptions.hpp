#pragma once

#include "../mod.hpp"

class RoseOptions : public Mod {
public:
    RoseOptions() = default;

    void toggleDisableRoseDespawnOnHittingEnemy(bool enabled);
    void toggleDisableRoseDespawnOnHittingObject(bool enabled);
    void toggleRoseInfiniteTimer(bool enabled);

    std::string getModName() override { return "RoseOptions"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
    hl::Patch patch3;
};
