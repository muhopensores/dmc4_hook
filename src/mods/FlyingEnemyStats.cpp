#include "FlyingEnemyStats.hpp"
#include "EnemyTracker.hpp" // for damage specific offsets
#include "sdk/World2Screen.hpp"

#if 1
bool FlyingEnemyStats::flyingEnemyStats = false;
bool FlyingEnemyStats::showFlyingEnemyHP = false;
bool FlyingEnemyStats::showFlyingEnemyDamageTaken = false;
bool FlyingEnemyStats::showFlyingEnemyDT = false;
bool FlyingEnemyStats::showFlyingEnemyStun = false;
bool FlyingEnemyStats::showFlyingEnemyDisplacement = false;
bool FlyingEnemyStats::showFlyingEnemyMoveID = false;
bool FlyingEnemyStats::showFlyingEnemyMechanics = false;
bool FlyingEnemyStats::showFlyingEnemyStunTimer = false;
bool FlyingEnemyStats::showFlyingEnemyDebug = false;

class sUnit {
public:
    char pad_00[0x194];
    uEnemy* enemy;
};

sUnit* get_sUnit() {
		constexpr uintptr_t static_unit_ptr = 0xE552CC;
		sUnit* s_unit_ptr = (sUnit*)*(uintptr_t*)static_unit_ptr;
		return s_unit_ptr;
}

void FlyingEnemyStats::on_frame(fmilliseconds& dt) {
    if (SMediator* sMedPtr = devil4_sdk::get_sMediator()) {
        if (sMedPtr->player_ptr) {
            if (flyingEnemyStats) {
                sUnit* sUnit = get_sUnit();
                if (!sUnit) { return; }
                if (uEnemy* enemy = sUnit->enemy) {
                    while (enemy) {
                        glm::vec3 objectPosition = enemy->position;
                        float objectDistance = w2s::GetDistanceFromCam(objectPosition);
                        float guiFriendlyDistance = glm::min(1000.0f / objectDistance, 1.0f);
                        glm::vec2 screenPos = w2s::WorldToScreen(objectPosition);
                        std::string windowName = "EnemyStats##" + std::to_string((uintptr_t)enemy);
                        if (w2s::IsVisibleOnScreen(objectPosition)) {
                            ImGui::Begin(windowName.c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
                            ImGui::PushID((uintptr_t)enemy);
                            ImGui::SetWindowPos(screenPos);
                            ImGui::SetWindowFontScale(1.0f * guiFriendlyDistance);
                            uEnemyDamage* currentEnemyDamage = (uEnemyDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
                            ImGui::PushItemWidth((sameLineItemWidth / 2.5f) * guiFriendlyDistance);
                            if (enemy->ID == 29 || enemy->ID == 30) { // Sanctus M19/M20
                                uBoss* boss = (uBoss*)enemy;
                                if (showFlyingEnemyHP) ImGui::SliderFloat("HP##EnemyFly", &boss->sanctusHP, 0.0f, boss->sanctusHPMax, "%.0f");
                            }
                            else {
                                if (showFlyingEnemyHP) ImGui::SliderFloat("HP##EnemyFly", &currentEnemyDamage->HP, 0.0f, currentEnemyDamage->HPMax, "%.0f");
                            }
                            if (showFlyingEnemyDamageTaken) ImGui::InputFloat("Damage##EnemyFly", &currentEnemyDamage->HPTaken, NULL, NULL, "%.0f");
                            if (showFlyingEnemyDT) ImGui::InputFloat("DT Timer##EnemyFly", &enemy->DTTimer, NULL, NULL, "%.0f"); // id * 4 + DevilMayCry4_DX9.exe+9EC0E0
                            if (showFlyingEnemyStun) ImGui::InputInt("Stun##EnemyFly", &currentEnemyDamage->stun[0], NULL, NULL);
                            if (showFlyingEnemyDisplacement) ImGui::InputInt("Displacement##EnemyFly", &currentEnemyDamage->displacement[0], NULL, NULL);
                            if (showFlyingEnemyStunTimer)ImGui::SliderFloat("Stun Reset Timer##EnemyFly", &currentEnemyDamage->stunResetTimer, 0.0f, 180.0f, "%.0f");
                            if (showFlyingEnemyMoveID) ImGui::InputScalar("MoveID##EnemyFly", ImGuiDataType_U8, &enemy->moveID, NULL, NULL, "%02X", ImGuiInputTextFlags_CharsHexadecimal);
                            if (showFlyingEnemyMechanics) {
                                if (enemy->ID == 5 || enemy->ID == 6) { // Bianco, Alto
                                    ImGui::SliderFloat("Shield##EnemyFly", &enemy->angeloShield, 0.0f, enemy->angeloShieldMax, "%.0f");
                                }
                                if (enemy->ID == 8 || enemy->ID == 9) { // Mephisto, Faust
                                    if (enemy->faustCloak > 0.0f) {
                                        ImGui::SliderFloat("Cloak##EnemyFly", &enemy->faustCloak, 0.0f, enemy->faustCloakMax, "%.0f");
                                    }
                                    else {
                                        ImGui::InputFloat("Cloak Timer##EnemyFly", &enemy->faustCloakTimer, NULL, NULL, "%.0f");
                                    }
                                }
                                if (enemy->ID == 12) { // Blitz
                                    if (enemy->blitzElectric > 0.0f) {
                                        ImGui::SliderFloat("Electric##EnemyFly", &enemy->blitzElectric, 0.0f, 1000.0f, "%.0f");
                                    }
                                    else {
                                        if (currentEnemyDamage->HP < enemy->blitzElectricSuicideHPRequirement && enemy->blitzElectricSuicideTimer > 0.0f) {
                                            ImGui::SliderFloat("Suicide Timer##EnemyFly", &enemy->blitzElectricSuicideTimer, 0.0f, 1800.0f, "%.0f");
                                        }
                                        else {
                                            ImGui::SliderFloat("Electric Timer##EnemyFly", &enemy->blitzElectricTimer, 0.0f, 900.0f, "%.0f");
                                        }
                                    }
                                }
                                if (enemy->ID == 16) { // Gladius
                                    if (enemy->gladiusBuried) {
                                        ImGui::SliderFloat("Buried Timer##EnemyFly", &enemy->gladiusTimer, 0.0f, 300.0f, "%.0f");
                                    }
                                }
                                uBoss* boss = (uBoss*)enemy;
                                if (boss->ID == 18) { // Berial
                                    if (boss->berialFireTimer > 0.0f) {
                                        ImGui::InputFloat("Fire Timer##EnemyFly", &boss->berialFireTimer, NULL, NULL, "%.0f");
                                    }
                                    else {
                                        ImGui::SliderFloat("Fire Damage##EnemyFly", &boss->berialFire, 0.0f, boss->berialFireMax, "%.0f");
                                    }
                                }
                                if (boss->ID == 22) { // Credo
                                    if (boss->credoShield > 0.0f) {
                                        ImGui::SliderFloat("Shield##EnemyFly", &boss->credoShield, 0.0f, 4000.0f, "%.0f");
                                    }
                                    else {
                                        ImGui::InputFloat("Shield Timer##EnemyFly", &boss->credoShieldTimer, NULL, NULL, "%.0f");
                                    }
                                }
                                if (boss->ID == 29) { // Sanctus M11
                                    if (boss->sanctusShieldTimerM11 > 0.0f) {
                                        ImGui::InputFloat("Shield Timer##EnemyFly", &boss->sanctusShieldTimerM11, NULL, NULL, "%.0f");
                                    }
                                    else {
                                        ImGui::SliderFloat("Shield Damage##EnemyFly", &boss->sanctusShieldM11, 0.0f, 720.0f, "%.0f");
                                    }
                                }
                                if (enemy->ID == 30) { // Sanctus M20
                                    if (boss->sanctusShieldTimerM20 > 0.0f) {
                                        ImGui::InputFloat("Shield Timer##EnemyFly", &boss->sanctusShieldTimerM20, NULL, NULL, "%.0f");
                                    }
                                    else {
                                        ImGui::SliderFloat("Shield Damage##EnemyFly", &boss->sanctusShieldM20, 0.0f, 600.0f, "%.0f");
                                    }
                                }
                            }
                            if (showFlyingEnemyDebug) {
                                ImGui::PushItemWidth(sameLineItemWidth);
                                ImGui::InputFloat3(_("XYZ Position##EnemyFly"), (float*)&enemy->position);
                                ImGui::InputFloat3(_("XYZ Rotation##EnemyFly"), (float*)&enemy->rotation);
                                ImGui::InputFloat3(_("XYZ Velocity##EnemyFly"), (float*)&enemy->velocity);
                                ImGui::InputFloat3(_("XYZ Scale##EnemyFly"), (float*)&enemy->scale);
                                ImGui::PopItemWidth();
                                ImGui::InputScalar(_("Anim ID##EnemyFly"), ImGuiDataType_U16, &enemy->animID, NULL, NULL, "%04X", ImGuiInputTextFlags_CharsHexadecimal);
                                ImGui::InputScalar(_("Move Part##EnemyFly"), ImGuiDataType_U8, &enemy->movePart);
                                ImGui::InputScalar(_("Grounded##EnemyFly"), ImGuiDataType_U8, &enemy->grounded);
                                ImGui::SliderFloat(_("Animation Frame##EnemyFly"), &enemy->animFrame, 0.0f, enemy->animFrameMax);
                            }
                            ImGui::PopItemWidth();
                            ImGui::PopID();
                            ImGui::End();
                        }
                        enemy = enemy->nextEnemy;
                    }
                }
            }
        }
    }
}

std::optional<std::string> FlyingEnemyStats::on_initialize() {

    return Mod::on_initialize();
}

void FlyingEnemyStats::on_gui_frame(int display) {
    ImGui::Checkbox(_("Flying Enemy Stats"), &flyingEnemyStats);
    ImGui::SameLine();
    help_marker(_("Render ImGui stats on enemies"));
    if (flyingEnemyStats) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox("Display HP", &showFlyingEnemyHP);
        ImGui::Checkbox("Display Damage Taken", &showFlyingEnemyDamageTaken);
        ImGui::Checkbox("Display DT Timer", &showFlyingEnemyDT);
        ImGui::Checkbox("Display Stun", &showFlyingEnemyStun);
        ImGui::Checkbox("Display Displacement", &showFlyingEnemyDisplacement);
        ImGui::Checkbox("Display Move ID", &showFlyingEnemyMoveID);
        ImGui::Checkbox("Display Mechanics", &showFlyingEnemyMechanics);
        ImGui::Checkbox("Display Debug Info", &showFlyingEnemyDebug);
        ImGui::Unindent(lineIndent);
    }
}

void FlyingEnemyStats::on_config_load(const utility::Config& cfg) {
    flyingEnemyStats = cfg.get<bool>("flyingEnemyStats").value_or(false);
    showFlyingEnemyHP = cfg.get<bool>("showFlyingEnemyHP").value_or(true);
    showFlyingEnemyDT = cfg.get<bool>("showFlyingEnemyDT").value_or(false);
    showFlyingEnemyDamageTaken = cfg.get<bool>("showFlyingEnemyDamageTaken").value_or(false);
    showFlyingEnemyMoveID = cfg.get<bool>("showFlyingEnemyMoveID").value_or(false);
    showFlyingEnemyStun = cfg.get<bool>("showFlyingEnemyStun").value_or(false);
    showFlyingEnemyDisplacement = cfg.get<bool>("showFlyingEnemyDisplacement").value_or(false);
    showFlyingEnemyStunTimer = cfg.get<bool>("showFlyingEnemyStunTimer").value_or(false);
    showFlyingEnemyMechanics = cfg.get<bool>("showFlyingEnemyMechanics").value_or(false);
    showFlyingEnemyDebug = cfg.get<bool>("showFlyingEnemyDebug").value_or(false);
}

void FlyingEnemyStats::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("flyingEnemyStats", flyingEnemyStats);
    cfg.set<bool>("showFlyingEnemyHP", showFlyingEnemyHP);
    cfg.set<bool>("showFlyingEnemyDT", showFlyingEnemyDT);
    cfg.set<bool>("showFlyingEnemyDamageTaken", showFlyingEnemyDamageTaken);
    cfg.set<bool>("showFlyingEnemyMoveID", showFlyingEnemyMoveID);
    cfg.set<bool>("showFlyingEnemyStun", showFlyingEnemyStun);
    cfg.set<bool>("showFlyingEnemyMechanics", showFlyingEnemyMechanics);
    cfg.set<bool>("showFlyingEnemyStunTimer", showFlyingEnemyStunTimer);
    cfg.set<bool>("showFlyingEnemyDebug", showFlyingEnemyDebug);
}
#endif
