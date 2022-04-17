#include "../mods.h"
#include "modRestoreMaxHp.hpp"

bool RestoreMaxHp::modEnabled{ false };

float twoMinutesTimer = 7200.0f;
bool resetTimer = false;

constexpr uintptr_t staticMediatorPtr = 0x00E558B8;

std::optional<std::string> RestoreMaxHp::onInitialize() {
    return Mod::onInitialize();
}

/*
I don't think there's a pointer chain in common, they just set up esi before calling damage, e.g.
frost has
DevilMayCry4_DX9.exe+1AD0ED - lea esi,[ebx+00001504] 
[...]
call damage

scarecrow has
DevilMayCry4_DX9.exe+157D9F - lea eax,[ebx+0000152C]
DevilMayCry4_DX9.exe+157E4D - mov esi,eax
[...]
call damage
*/

int RestoreMaxHp::getEnemySpecificDamageOffset(int enemyID) {
    switch (enemyID)
    {
    // 0x152C
    case 0x0: // Scarecrow Leg
        return 0x152C;
    case 0x1: // Scarecrow Arm
        return 0x152C;
    case 0x3: // Scarecrow Mega
        return 0x152C;

    // 0x1500
    case 0x8: // mephisto
        return 0x1500;
    case 0x9: // faust
        return 0x1500;
    case 0xB: // assault
        return 0x1500;
    case 0x10: // gladius
        return 0x1500;


    // 0x1504
    case 0x5: // Alto
        return 0x1504;
    case 0x6: // Bianco
        return 0x1504;
    case 0xA: // Frost
        return 0x1504;
    case 0xC: // Blitz
        return 0x1504;

    // 1508
    case 0xF: // Cutlass
        return 0x1508;

    // 7FC4
    case 0x11: // Basilisk
        return 0x7FC4;
    }
    return NULL;
    // seeds aren't on the enemy list
    // i am too lazy to find a fault but they probably aren't either
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
                        int damageInfoOffset = getEnemySpecificDamageOffset(enemyID);
                        if (damageInfoOffset != NULL) {
                            float& enemyHP = *(float*)(enemyBase + damageInfoOffset + 0x18);
                            float& enemyMaxHP = *(float*)(enemyBase + damageInfoOffset + 0x1C);
                            enemyHP = enemyMaxHP;
                        }
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
