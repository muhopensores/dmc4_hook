#include "JointDisplay.hpp"
#include "../sdk/Devil4.hpp"
#include "sdk/World2Screen.hpp"
#include <ImGuizmo.h>

bool JointDisplay::mod_enabled = false;

void JointDisplay::on_frame(fmilliseconds& dt) {
    if (!JointDisplay::mod_enabled) { return; }
    if (uPlayer* player = devil4_sdk::get_local_player()) {
        uEnemy* enemy = devil4_sdk::get_uEnemies();
        int enemyCount = 0;
        while (enemy) {
            if (!enemy->joints) {
                enemyCount++;
                enemy = enemy->nextEnemy;
                continue;
            }
            for (int i = 0; i < enemy->m_joint_array_size; i++) {
                UModelJoint* joint = &enemy->joints->joint[i];
                Vector3f jointPos = Vector3f(joint->mWmat.m4.x, joint->mWmat.m4.y, joint->mWmat.m4.z);
                w2s::DrawWireframeCapsule(jointPos, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, IM_COL32(0, 0, 0, 255), 16, 1.0f);
            }
            enemyCount++;
            enemy = enemy->nextEnemy;
        }
        { // player
            if (!player->joint_array) {
                return;
            }
            for (int i = 0; i < player->m_joint_array_size; i++) {
                UModelJoint* joint = &player->joint_array->joint[i];
                Vector3f jointPos = Vector3f(joint->mWmat.m4.x, joint->mWmat.m4.y, joint->mWmat.m4.z);
                w2s::DrawWireframeCapsule(jointPos, 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, IM_COL32(0, 0, 0, 255), 16, 1.0f);
            }
        }
    }
}

void JointDisplay::on_gui_frame(int display) {
    ImGui::Checkbox(_("JointDisplay"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("JointDisplayHelpMarker"));
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> JointDisplay::on_initialize() {
    return Mod::on_initialize();
}

void JointDisplay::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("JointDisplay").value_or(false);
}

void JointDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("JointDisplay", mod_enabled);
}
