#include "JointDisplay.hpp"
#include "../sdk/Devil4.hpp"
#include "sdk/World2Screen.hpp"
#include "../sdk/Player.hpp"
#include "../sdk/Enemy.hpp"

bool JointDisplay::mod_enabled = false;

static void drawJoints(UModelJoint* joints, int jointArraySize, int entityId, ImU32 normalColor, ImU32 selectedColor, float view[16], float projection[16],
    int& selectedEnemyIndex, int& selectedJointIndex, bool& isManipulating, ImGuizmo::OPERATION currentGizmoOperation, ImGuizmo::MODE currentGizmoMode) {
    
    // speed limits to prevent joints from shooting off into the horizon in 1f
    static constexpr float MAX_DELTA_PER_FRAME = 5.0f;
    static constexpr float MAX_SCALE_CHANGE = 0.1f;
    static constexpr float MAX_ROTATION_PER_FRAME = glm::radians(15.0f);
    
    for (int i = 0; i < jointArraySize; i++) {
        UModelJoint* joint = &joints[i];
        
        glm::vec3 jointPos = glm::vec3(joint->mWmat.m4.x, joint->mWmat.m4.y, joint->mWmat.m4.z);
        
        int uniqueJointIndex = entityId * 1000 + i;
        
        glm::mat4 jointTransform = glm::mat4(
            joint->mWmat.m1.x, joint->mWmat.m1.y, joint->mWmat.m1.z, joint->mWmat.m1.w,
            joint->mWmat.m2.x, joint->mWmat.m2.y, joint->mWmat.m2.z, joint->mWmat.m2.w,
            joint->mWmat.m3.x, joint->mWmat.m3.y, joint->mWmat.m3.z, joint->mWmat.m3.w,
            joint->mWmat.m4.x, joint->mWmat.m4.y, joint->mWmat.m4.z, joint->mWmat.m4.w
        );
        
        glm::mat4 newTransform;
        
        bool wasManipulated = w2s::DrawImGuizmoManipulator(
            jointTransform, newTransform, uniqueJointIndex, selectedJointIndex, isManipulating,
            currentGizmoOperation, currentGizmoMode, view, projection,
            (selectedJointIndex == uniqueJointIndex) ? selectedColor : normalColor,
            (selectedJointIndex == uniqueJointIndex) ? 8.0f : 5.0f,
            "Joint"
        );
        
        if (wasManipulated) {
            selectedEnemyIndex = entityId;
            
            if (currentGizmoOperation == ImGuizmo::TRANSLATE) {
                glm::vec3 newPos = glm::vec3(newTransform[3]);
                glm::vec3 deltaPos = newPos - jointPos;
                
                float deltaLength = glm::length(deltaPos);
                if (deltaLength > MAX_DELTA_PER_FRAME) {
                    deltaPos = glm::normalize(deltaPos) * MAX_DELTA_PER_FRAME;
                }
                
                if (currentGizmoMode == ImGuizmo::WORLD) {
                    joint->mOffset.x += deltaPos.x;
                    joint->mOffset.y += deltaPos.y;
                    joint->mOffset.z += deltaPos.z;
                } else {
                    glm::mat4 parentTransform = glm::mat4(
                        joints[joint->mParentIndex].mWmat.m1.x, joints[joint->mParentIndex].mWmat.m1.y, joints[joint->mParentIndex].mWmat.m1.z, joints[joint->mParentIndex].mWmat.m1.w,
                        joints[joint->mParentIndex].mWmat.m2.x, joints[joint->mParentIndex].mWmat.m2.y, joints[joint->mParentIndex].mWmat.m2.z, joints[joint->mParentIndex].mWmat.m2.w,
                        joints[joint->mParentIndex].mWmat.m3.x, joints[joint->mParentIndex].mWmat.m3.y, joints[joint->mParentIndex].mWmat.m3.z, joints[joint->mParentIndex].mWmat.m3.w,
                        joints[joint->mParentIndex].mWmat.m4.x, joints[joint->mParentIndex].mWmat.m4.y, joints[joint->mParentIndex].mWmat.m4.z, joints[joint->mParentIndex].mWmat.m4.w
                    );
                    
                    glm::mat4 invParentTransform = glm::inverse(parentTransform);
                    glm::vec3 localDelta = glm::vec3(invParentTransform * glm::vec4(deltaPos, 0.0f));
                    
                    joint->mOffset.x += localDelta.x;
                    joint->mOffset.y += localDelta.y;
                    joint->mOffset.z += localDelta.z;
                }
            }
            else if (currentGizmoOperation == ImGuizmo::SCALE) { // requires nop from big head mode detour, done
                glm::vec3 newScale;
                newScale.x = glm::length(glm::vec3(newTransform[0]));
                newScale.y = glm::length(glm::vec3(newTransform[1]));
                newScale.z = glm::length(glm::vec3(newTransform[2]));
                
                glm::vec3 currentScale = glm::vec3(joint->mScale.x, joint->mScale.y, joint->mScale.z);
                glm::vec3 scaleDelta = newScale - currentScale;
                
                scaleDelta.x = glm::clamp(scaleDelta.x, -MAX_SCALE_CHANGE, MAX_SCALE_CHANGE);
                scaleDelta.y = glm::clamp(scaleDelta.y, -MAX_SCALE_CHANGE, MAX_SCALE_CHANGE);
                scaleDelta.z = glm::clamp(scaleDelta.z, -MAX_SCALE_CHANGE, MAX_SCALE_CHANGE);
                
                joint->mScale.x += scaleDelta.x;
                joint->mScale.y += scaleDelta.y;
                joint->mScale.z += scaleDelta.z;
                
                joint->mScale.x = glm::max(joint->mScale.x, 0.1f);
                joint->mScale.y = glm::max(joint->mScale.y, 0.1f);
                joint->mScale.z = glm::max(joint->mScale.z, 0.1f);
            }
            else if (currentGizmoOperation == ImGuizmo::ROTATE) { // haven't nop'd whatever writes to rotation yet
                glm::mat3 rotationMatrix;
                rotationMatrix[0] = glm::normalize(glm::vec3(newTransform[0]));
                rotationMatrix[1] = glm::normalize(glm::vec3(newTransform[1]));
                rotationMatrix[2] = glm::normalize(glm::vec3(newTransform[2]));
                
                glm::quat newRotation = glm::quat_cast(rotationMatrix);
                
                glm::quat currentQuat(joint->mQuat.w, joint->mQuat.x, joint->mQuat.y, joint->mQuat.z);
                glm::quat deltaRotation = newRotation * glm::inverse(currentQuat);
                
                float angle = glm::angle(deltaRotation);
                if (angle > MAX_ROTATION_PER_FRAME) {
                    glm::vec3 axis = glm::axis(deltaRotation);
                    deltaRotation = glm::angleAxis(MAX_ROTATION_PER_FRAME, axis);
                }
                
                glm::quat finalQuat;
                
                if (currentGizmoMode == ImGuizmo::WORLD) {
                    finalQuat = deltaRotation * currentQuat;
                } else {
                    finalQuat = currentQuat * deltaRotation;
                }
                
                joint->mQuat.w = finalQuat.w;
                joint->mQuat.x = finalQuat.x;
                joint->mQuat.y = finalQuat.y;
                joint->mQuat.z = finalQuat.z;
            }
        }
    }
}

void JointDisplay::on_frame(fmilliseconds& dt) {
    if (!JointDisplay::mod_enabled) { return; }
    
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) { return; }
    
    if (ImGui::Begin(_("Joint Display Controls"))) {
        ImGui::Text(_("G - Translate"));
        ImGui::Text(_("R - Rotate"));
        ImGui::Text(_("S - Scale"));
        ImGui::Text(_("T - Toggle World/Local space"));
    }
    ImGui::End();

    w2s::ImGuizmoSetup();
    float view[16], projection[16];
    if (!w2s::GetImGuizmoMatrices(view, projection)) {
        return;
    }
    
    static int selectedEnemyIndex = -1;
    static int selectedJointIndex = -1;
    static bool isManipulating = false;
    static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
    static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;
    
    uEnemy* enemy = devil4_sdk::get_uEnemies();
    int enemyCount = 0;
    while (enemy) {
        if (enemy->joints) {
            drawJoints(enemy->joints->joint, enemy->m_joint_array_size, enemyCount,
                      IM_COL32(255, 255, 0, 128), // yellow
                      IM_COL32(0, 255, 0, 255), // green
                      view, projection,
                      selectedEnemyIndex, selectedJointIndex, 
                      isManipulating, currentGizmoOperation, currentGizmoMode);
        }
        enemyCount++;
        enemy = enemy->nextEnemy;
    }
    
    if (player->joint_array) {
        drawJoints(player->joint_array->joint, player->m_joint_array_size, -1,
                  IM_COL32(255, 255, 0, 128), // yellow
                  IM_COL32(0, 255, 0, 255), // green
                  view, projection,
                  selectedEnemyIndex, selectedJointIndex, 
                  isManipulating, currentGizmoOperation, currentGizmoMode);
    }
    
    w2s::ImGuizmoKeyboardShortcuts(currentGizmoOperation, currentGizmoMode);
    w2s::ImGuizmoDeselection(selectedJointIndex);
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
