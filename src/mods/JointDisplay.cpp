#include "JointDisplay.hpp"
#include "../sdk/Devil4.hpp"
#include "sdk/World2Screen.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/uEnemy.hpp"

bool JointDisplay::mod_enabled = false;
bool JointDisplay::show_manipulators = false;

struct Transformable {
    glm::vec3* position    = nullptr;
    glm::vec3* scale       = nullptr;
    glm::quat* rotation    = nullptr; // optional
    glm::mat4* worldMatrix = nullptr; // optional for joints
    int parentIndex        = -1;      // for joints
};

static bool DrawTransformableManipulator(Transformable& obj, int uniqueIndex, int& selectedIndex, bool& isManipulating,
    ImGuizmo::OPERATION& operation, ImGuizmo::MODE mode, float view[16], float projection[16], ImU32 normalColor, ImU32 selectedColor,
    float size, const char* label) {
    glm::mat4 transform = glm::mat4(1.0f);

    if (obj.worldMatrix) {
        transform = *obj.worldMatrix;
    } else {
        transform = glm::translate(glm::mat4(1.0f), *obj.position) * glm::scale(glm::mat4(1.0f), *obj.scale);
    }

    ImU32 color = (selectedIndex == uniqueIndex) ? selectedColor : normalColor;
    glm::mat4 newTransform;

    bool manipulated = w2s::DrawImGuizmoManipulator(
        transform, newTransform, uniqueIndex, selectedIndex, isManipulating, operation, mode, view, projection, color, size, label);

    if (manipulated) {
        selectedIndex = uniqueIndex;

        if (operation == ImGuizmo::TRANSLATE && obj.position) {
            *obj.position = glm::vec3(newTransform[3]);
        } else if (operation == ImGuizmo::SCALE && obj.scale) {
            glm::vec3 newScale(
                glm::length(glm::vec3(newTransform[0])), glm::length(glm::vec3(newTransform[1])), glm::length(glm::vec3(newTransform[2])));
            *obj.scale = newScale;
        } else if (operation == ImGuizmo::ROTATE && obj.rotation) {
            glm::mat3 rotMat;
            rotMat[0]     = glm::normalize(glm::vec3(newTransform[0]));
            rotMat[1]     = glm::normalize(glm::vec3(newTransform[1]));
            rotMat[2]     = glm::normalize(glm::vec3(newTransform[2]));
            *obj.rotation = glm::quat_cast(rotMat);
        }
    }

    return manipulated;
}

static void drawJoints(UModelJoint* joints, int jointCount, int entityId, float view[16], float projection[16], int& selectedEnemyIndex,
    int& selectedJointIndex, bool& isManipulating, ImGuizmo::OPERATION& operation, ImGuizmo::MODE mode) {
    constexpr float MAX_DELTA_PER_FRAME    = 5.0f;
    constexpr float MAX_SCALE_CHANGE       = 0.1f;
    constexpr float MAX_ROTATION_PER_FRAME = glm::radians(15.0f);

    for (int i = 0; i < jointCount; i++) {
        UModelJoint* joint = &joints[i];
        int uniqueIndex    = entityId * 1000 + i;

        glm::mat4 jointTransform = glm::make_mat4(&joint->mWmat.m1.x); // row-major pointer
        glm::mat4 newTransform;

        bool manipulated = w2s::DrawImGuizmoManipulator(jointTransform, newTransform, uniqueIndex, selectedJointIndex, isManipulating,
            operation, mode, view, projection, (selectedJointIndex == uniqueIndex) ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 255, 0, 128),
            (selectedJointIndex == uniqueIndex) ? 8.0f : 5.0f, "Joint");

        if (manipulated) {
            selectedEnemyIndex = entityId;

            if (operation == ImGuizmo::TRANSLATE) {
                glm::vec3 delta = glm::vec3(newTransform[3] - jointTransform[3]);
                float len       = glm::length(delta);
                if (len > MAX_DELTA_PER_FRAME)
                    delta = glm::normalize(delta) * MAX_DELTA_PER_FRAME;

                glm::mat4 parentTransform(1.0f);
                if (mode == ImGuizmo::LOCAL && joint->mParentIndex >= 0)
                    parentTransform = glm::make_mat4(&joints[joint->mParentIndex].mWmat.m1.x);

                glm::vec3 localDelta = glm::vec3(glm::inverse(parentTransform) * glm::vec4(delta, 0.0f));
                joint->mOffset.x += localDelta.x;
                joint->mOffset.y += localDelta.y;
                joint->mOffset.z += localDelta.z;
            } else if (operation == ImGuizmo::SCALE) {
                glm::vec3 newScale(glm::length(glm::vec3(newTransform[0])), glm::length(glm::vec3(newTransform[1])),
                    glm::length(glm::vec3(newTransform[2])));
                glm::vec3 deltaScale = newScale - glm::vec3(joint->mScale.x, joint->mScale.y, joint->mScale.z);
                deltaScale           = glm::clamp(deltaScale, glm::vec3(-MAX_SCALE_CHANGE), glm::vec3(MAX_SCALE_CHANGE));
                joint->mScale.x += deltaScale.x;
                joint->mScale.y += deltaScale.y;
                joint->mScale.z += deltaScale.z;
                joint->mScale = glm::max(glm::vec3(joint->mScale.x, joint->mScale.y, joint->mScale.z), glm::vec3(0.1f));
            } else if (operation == ImGuizmo::ROTATE) {
                glm::mat3 rotMat;
                rotMat[0] = glm::normalize(glm::vec3(newTransform[0]));
                rotMat[1] = glm::normalize(glm::vec3(newTransform[1]));
                rotMat[2] = glm::normalize(glm::vec3(newTransform[2]));

                glm::quat newQuat = glm::quat_cast(rotMat);
                glm::quat currentQuat(joint->mQuat.w, joint->mQuat.x, joint->mQuat.y, joint->mQuat.z);
                glm::quat deltaQuat = newQuat * glm::inverse(currentQuat);

                float angle = glm::angle(deltaQuat);
                if (angle > MAX_ROTATION_PER_FRAME) {
                    glm::vec3 axis = glm::axis(deltaQuat);
                    deltaQuat      = glm::angleAxis(MAX_ROTATION_PER_FRAME, axis);
                }

                glm::quat finalQuat = (mode == ImGuizmo::WORLD) ? deltaQuat * currentQuat : currentQuat * deltaQuat;
                joint->mQuat.w      = finalQuat.w;
                joint->mQuat.x      = finalQuat.x;
                joint->mQuat.y      = finalQuat.y;
                joint->mQuat.z      = finalQuat.z;
            }
            std::memcpy(&joint->mWmat.m1.x, &newTransform, sizeof(glm::mat4));
        }
    }
}

static void drawEnemies(uEnemy_Old* enemies, float view[16], float projection[16], int& selectedEnemyIndex, bool& isManipulating, ImGuizmo::OPERATION& operation) {
    int enemyIndex = 0;
    while (enemies) {
        Transformable t;
        t.position = &enemies->position;
        t.scale    = &enemies->scale;
        bool manipulated = DrawTransformableManipulator(t, enemyIndex, selectedEnemyIndex, isManipulating, operation, ImGuizmo::WORLD, view, projection, IM_COL32(255, 255, 0, 200), IM_COL32(255, 0, 0, 255), (selectedEnemyIndex == enemyIndex) ? 12.0f : 8.0f, "Enemy");
        enemyIndex++;
        enemies = enemies->nextEnemy;
    }
    w2s::ImGuizmoDeselection(selectedEnemyIndex);
}

void JointDisplay::on_frame(fmilliseconds& dt) {
    if (JointDisplay::mod_enabled || show_manipulators) {
        if (ImGui::Begin(_("Manipulation Controls"))) {
            ImGui::Text(_("G - Translate"));
            // ImGui::Text(_("R - Rotate"));
            ImGui::Text(_("S - Scale"));
            // ImGui::Text(_("T - Toggle World/Local space"));
            ImGui::End();
        }
        uPlayer* player = devil4_sdk::get_local_player();
        if (!player)
            return;

        w2s::ImGuizmoSetup();
        float view[16], projection[16];
        if (!w2s::GetImGuizmoMatrices(view, projection))
            return;

        static int selectedEnemyIndex                    = -1;
        static int selectedJointIndex                    = -1;
        static bool isManipulating                       = false;
        static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
        static ImGuizmo::MODE currentGizmoMode           = ImGuizmo::LOCAL;

        if (show_manipulators) {
            drawEnemies(devil4_sdk::get_uEnemies(), view, projection, selectedEnemyIndex, isManipulating, currentGizmoOperation);
        }

        if (JointDisplay::mod_enabled) {
            uEnemy_Old* enemy = devil4_sdk::get_uEnemies();
            int enemyCount    = 0;
            while (enemy) {
                if (enemy->joints) {
                    drawJoints(enemy->joints->joint, enemy->m_joint_array_size, enemyCount, view, projection, selectedEnemyIndex,
                        selectedJointIndex, isManipulating, currentGizmoOperation, currentGizmoMode);
                }
                enemyCount++;
                enemy = enemy->nextEnemy;
            }

            if (player->joint_array) {
                drawJoints(player->joint_array->joint, player->m_joint_array_size, -1, view, projection, selectedEnemyIndex,
                    selectedJointIndex, isManipulating, currentGizmoOperation, currentGizmoMode);
            }
        }

        w2s::ImGuizmoKeyboardShortcuts(currentGizmoOperation, currentGizmoMode);
        w2s::ImGuizmoDeselection(selectedJointIndex);
    }
}

void JointDisplay::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("JointDisplay"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("JointDisplayHelpMarker"));

        ImGui::Checkbox(_("Show Manipulators"), &show_manipulators);
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> JointDisplay::on_initialize() {
    return Mod::on_initialize();
}

void JointDisplay::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("JointDisplay").value_or(false);
    show_manipulators = cfg.get<bool>("show_manipulators").value_or(false);
}

void JointDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("JointDisplay", mod_enabled);
    cfg.set<bool>("show_manipulators", show_manipulators);
}
