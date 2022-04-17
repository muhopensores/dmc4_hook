#include "../mods.h"
#include "modRestoreMaxHp.hpp"
#include "modInputStates.hpp"

bool RestoreMaxHp::modEnabled{ false };

float twoMinutesTimer = 7200.0f;
bool resetTimer = false;

constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

std::optional<std::string> RestoreMaxHp::onInitialize() {
    return Mod::onInitialize();
}

void RestoreMaxHp::onFrame(fmilliseconds& dt) {
    // uintptr_t* sMedPtr = *(uintptr_t**)staticMediatorPtr;
    // uintptr_t* uLocalPlr = *(uintptr_t**)((uintptr_t)sMedPtr + 0x24);
    uintptr_t* sMedPtr = (uintptr_t*)staticMediatorPtr;
    uintptr_t sMediator = *sMedPtr;
    uintptr_t* playerPtr = (uintptr_t*)(sMediator + 0x24);
    uintptr_t uLocalPlr = *playerPtr;
    if (uLocalPlr) {
        uint8_t& grounded = *(uint8_t*)(uLocalPlr + 0xEA8);
        uint8_t& desiredInput = *(uint8_t*)(uLocalPlr + 0x140C);
        if (modEnabled) {
            if (grounded == 1 && desiredInput & 0x10 && desiredInput & 0x08) {
                int8_t& enemyCount = *(int8_t*)(sMediator + 0x1E8);
                for (int i = 1; i <= enemyCount; i++) {
                    uintptr_t* enemyPtr = (uintptr_t*)(sMediator + 0x1B4 + i * 4);
                    uintptr_t enemyBase = *enemyPtr;
                    if (enemyBase) {
                        int& enemyID = *(int*)(enemyBase + 0x1410);
                        if (enemyID == SCARECROW_ARM || enemyID == SCARECROW_LEG || enemyID == SCARECROW_MEGA) {
                            float& enemyHP = *(float*)(enemyBase + 0x1544);
                            float& enemyMaxHP = *(float*)(enemyBase + 0x1548);
                            enemyHP = enemyMaxHP;
                        }
                        if (enemyID == FROST || enemyID == ANGELO_ALTO || enemyID == ANGELO_BIANCO || enemyID == BLITZ) {
                            float& enemyHP = *(float*)(enemyBase + 0x151C);
                            float& enemyMaxHP = *(float*)(enemyBase + 0x1520);
                            enemyHP = enemyMaxHP;
                        }
                        if (enemyID == MEPHISTO || enemyID == FAUST || enemyID == GLADIUS || enemyID == ASSAULT) {
                            float& enemyHP = *(float*)(enemyBase + 0x1518);
                            float& enemyMaxHP = *(float*)(enemyBase + 0x151C);
                            enemyHP = enemyMaxHP;
                        }
                        if (enemyID == BASILISK) {
                            float& enemyHP = *(float*)(enemyBase + 0x7FDC);
                            float& enemyMaxHP = *(float*)(enemyBase + 0x7FE0);
                            enemyHP = enemyMaxHP;
                        }
                        if (enemyID == CUTLASS) {
                            float& enemyHP = *(float*)(enemyBase + 0x1520);
                            float& enemyMaxHP = *(float*)(enemyBase + 0x1524);
                            enemyHP = enemyMaxHP;
                        }
                        // seeds aren't on the enemy list
                        // i am too lazy to find a fault but they probably aren't either
                    }
                }
                uintptr_t* bossPtr = (uintptr_t*)(sMediator + 0xB0);
                uintptr_t bossBase = *bossPtr;
                if (bossBase) {
                    float& bossHP = *(float*)(bossBase + 0x151C);
                    float& bossMaxHP = *(float*)(bossBase + 0x1520);
                    bossHP = bossMaxHP;
                }
            }
        }
        if (resetTimer) {
            if (grounded == 1 && desiredInput & 0x10 && desiredInput & 0x08) {
                float& bpTimer = *(float*)(sMediator + 0x250);
                bpTimer = twoMinutesTimer;
            }
        }
    }
}

void RestoreMaxHp::onGUIframe() {
    ImGui::Checkbox("Restore Enemy HP", &modEnabled);
    ImGui::SameLine();
    HelpMarker("Press Lock On + Taunt while grounded to restore Max HP to enemies");
    ImGui::SameLine(205);
    ImGui::Checkbox("Reset Timer", &resetTimer);
    ImGui::SameLine();
    HelpMarker("Press Lock On + Taunt while grounded to reset the BP timer");
}

void RestoreMaxHp::onConfigLoad(const utils::Config& cfg) {
    modEnabled = cfg.get<bool>("restore_max_hp").value_or(false);
    resetTimer = cfg.get<bool>("reset_bp_timer").value_or(false);
}

void RestoreMaxHp::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("restore_max_hp", modEnabled);
    cfg.set<bool>("reset_bp_timer", resetTimer);
}
