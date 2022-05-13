#include "../mods.h"
#include "modRoseOptions.hpp"

#if 1
static bool enabledNoDespawnEnemy = false;
static bool enabledNoDespawnObject = false;
static bool roseInfiniteTimer = false;

std::optional<std::string> RoseOptions::onInitialize() {
    return Mod::onInitialize();
}

void RoseOptions::toggleDisableRoseDespawnOnHittingEnemy(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435944, patch1, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch1.revert();
    }
}

void RoseOptions::toggleDisableRoseDespawnOnHittingObject(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435922, patch2, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch2.revert();
    }
}

void RoseOptions::toggleRoseInfiniteTimer(bool enabled) {
    if (enabled) {
        install_patch_offset(0x0435937, patch3, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch3.revert();
    }
}

void RoseOptions::onGUIframe() {
    if (ImGui::Checkbox("Rose Survives Enemies", &enabledNoDespawnEnemy)) {
        toggleDisableRoseDespawnOnHittingEnemy(enabledNoDespawnEnemy);
    }
    ImGui::SameLine();
    HelpMarker("Rose will hit the enemy and then continue past the enemy");

    if (ImGui::Checkbox("Rose Survives Objects", &enabledNoDespawnObject)) {
        toggleDisableRoseDespawnOnHittingObject(enabledNoDespawnObject);
    }
    ImGui::SameLine();
    HelpMarker("Rose will sit on whatever surface it lands on until its timer expires");

    if (ImGui::Checkbox("Infinite Rose Timer", &roseInfiniteTimer)) {
        toggleRoseInfiniteTimer(roseInfiniteTimer);
    }
    ImGui::SameLine();
    HelpMarker("Rose will only despawn when colliding with something or having another placed");
}

void RoseOptions::onConfigLoad(const utils::Config& cfg) {
    enabledNoDespawnEnemy = cfg.get<bool>("no_despawn_rose_enemy").value_or(false);
    toggleDisableRoseDespawnOnHittingEnemy(enabledNoDespawnEnemy);
    enabledNoDespawnObject = cfg.get<bool>("no_despawn_rose_object").value_or(false);
    toggleDisableRoseDespawnOnHittingObject(enabledNoDespawnObject);
    roseInfiniteTimer = cfg.get<bool>("rose_infinite_timer").value_or(false);
    toggleRoseInfiniteTimer(roseInfiniteTimer);
};

void RoseOptions::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("no_despawn_rose_enemy", enabledNoDespawnEnemy);
    cfg.set<bool>("no_despawn_rose_object", enabledNoDespawnObject);
    cfg.set<bool>("rose_infinite_timer", roseInfiniteTimer);
};

#endif
