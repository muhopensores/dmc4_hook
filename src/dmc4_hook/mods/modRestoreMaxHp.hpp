#pragma once

#include "../mod.hpp"

class RestoreMaxHp : public Mod
{
public:
    RestoreMaxHp() = default;

    static bool modEnabled;
    static uintptr_t _restoreMaxHpContinueScarecrow;
    static uintptr_t _restoreMaxHpContinueFrost;
    static uintptr_t _restoreMaxHpContinueArmour;
    static uintptr_t _restoreMaxHpContinueGhosts;
    static uintptr_t _restoreMaxHpContinueSwordFly;
    static uintptr_t _restoreMaxHpContinueSwordFish;
    static uintptr_t _restoreMaxHpContinueSeed;
    static uintptr_t _restoreMaxHpContinueRiot;
    static uintptr_t _restoreMaxHpContinueBlitz;
    static uintptr_t _restoreMaxHpContinueDog;
    static uintptr_t _restoreMaxHpContinueBerial;
    static uintptr_t _restoreMaxHpContinueFrog;
    static uintptr_t _restoreMaxHpContinueEchidna;
    static uintptr_t _restoreMaxHpContinueCredo;
    static uintptr_t _restoreMaxHpContinueAgnus;
    static uintptr_t _restoreMaxHpContinueSanctus;
    static uintptr_t _restoreMaxHpContinueDante;

    std::string getModName() override { return "RestoreMaxHp"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hookScarecrow;
    hl::Hooker hookFrost;
    hl::Hooker hookArmour;
    hl::Hooker hookGhosts;
    hl::Hooker hookSwordFly;
    hl::Hooker hookSwordFish;
    hl::Hooker hookSeed;
    hl::Hooker hookRiot;
    hl::Hooker hookBlitz;
    hl::Hooker hookDog;
    hl::Hooker hookBerial;
    hl::Hooker hookFrog;
    hl::Hooker hookEchidna;
    hl::Hooker hookCredo;
    hl::Hooker hookAgnus;
    hl::Hooker hookSanctus;
    hl::Hooker hookDante;
};