#include "JointDisplay.hpp"
#include "../sdk/Devil4.hpp"
#include "PhotoMode.hpp"
#include "sdk/World2Screen.hpp"
#include "sdk/uEnemy.hpp"
#include "sdk/uPlayer.hpp"
#include <commdlg.h>
#include <fstream>

bool JointDisplay::mod_enabled = false;
bool JointDisplay::show_manipulators = false;

static int selectedIndex = -1;
static bool isManipulating = false;
static ImGuizmo::OPERATION currentGizmoOperation = ImGuizmo::TRANSLATE;
static ImGuizmo::MODE currentGizmoMode = ImGuizmo::LOCAL;

static UModelJoint* lastManipulatedJoints = nullptr;
static int lastManipulatedJointCount      = 0;

struct Transformable {
    glm::vec3* position    = nullptr;
    glm::vec3* scale       = nullptr;
    glm::quat* rotation = nullptr;
    glm::mat4* worldMatrix = nullptr;
};

static bool DrawTransformableManipulator(Transformable& obj, int uniqueIndex, int& selectedIndex, bool& isManipulating,
    ImGuizmo::OPERATION& operation, ImGuizmo::MODE mode, float view[16], float projection[16], ImU32 normalColor, ImU32 selectedColor,
    float size, const char* label, bool apply = true, glm::mat4* outOldTransform = nullptr, glm::mat4* outNewTransform = nullptr) {
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

        if (outOldTransform)
            *outOldTransform = transform;
        if (outNewTransform)
            *outNewTransform = newTransform;

        if (apply) {
            if (operation == ImGuizmo::TRANSLATE && obj.position) {
                *obj.position = glm::vec3(newTransform[3]);
            } else if (operation == ImGuizmo::SCALE && obj.scale) {
                glm::vec3 newScale(glm::length(glm::vec3(newTransform[0])), glm::length(glm::vec3(newTransform[1])),
                    glm::length(glm::vec3(newTransform[2])));
                *obj.scale = newScale;
            } else if (operation == ImGuizmo::ROTATE && obj.rotation) {
                glm::mat3 rotMat;
                rotMat[0]     = glm::normalize(glm::vec3(newTransform[0]));
                rotMat[1]     = glm::normalize(glm::vec3(newTransform[1]));
                rotMat[2]     = glm::normalize(glm::vec3(newTransform[2]));
                *obj.rotation = glm::quat_cast(rotMat);
            }
        }
    }

    return manipulated;
}

/*typedef char(__thiscall* isKindOf)(void* entity, uintptr_t MtDTI);
isKindOf isKindOf_ptr = (isKindOf)0x402BD0;
char __fastcall MtObject__isKindOf(void* entity, uintptr_t MtDTI) {
    return isKindOf_ptr(entity, MtDTI);
}
static constexpr uintptr_t uCustomDemoActor_dti = 0xE586B8;*/

static inline int MakeEnemyId(int entityId) {
    return entityId << 1;
}
static inline int MakeJointId(int entityId, int jointIndex) {
    return ((entityId >> 4 ^ jointIndex) << 1) | 1;
}

static std::string OpenSaveFileDialog(const char* defaultName = "pose.txt") {
    char filename[MAX_PATH];
    strcpy_s(filename, defaultName);

    OPENFILENAMEA ofn = {};
    ofn.lStructSize   = sizeof(ofn);
    ofn.lpstrFilter   = "Pose Files (*.txt)\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile     = filename;
    ofn.nMaxFile      = MAX_PATH;
    ofn.lpstrDefExt   = "txt";
    ofn.Flags         = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetSaveFileNameA(&ofn))
        return std::string(filename);
    return "";
}

static std::string OpenLoadFileDialog() {
    char filename[MAX_PATH] = "";

    OPENFILENAMEA ofn = {};
    ofn.lStructSize   = sizeof(ofn);
    ofn.lpstrFilter   = "Pose Files (*.txt)\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile     = filename;
    ofn.nMaxFile      = MAX_PATH;
    ofn.Flags         = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn))
        return std::string(filename);
    return "";
}

static bool SavePoseToFile(UModelJoint* joints, int jointCount, const std::string& path) {
    if (!joints || jointCount <= 0)
        return false;

    struct Entry {
        int mNo;
        float offset[3];
        float scale[3];
        float quat[4]; // w, x, y, z
    };

    std::vector<Entry> entries;
    entries.reserve(jointCount);
    for (int i = 0; i < jointCount; i++) {
        UModelJoint& j = joints[i];
        if (j.mNo == 0 || j.mNo >= 150)
            continue;

        Entry e;
        e.mNo       = j.mNo;
        e.offset[0] = j.mOffset.x;
        e.offset[1] = j.mOffset.y;
        e.offset[2] = j.mOffset.z;
        e.scale[0]  = j.mScale.x;
        e.scale[1]  = j.mScale.y;
        e.scale[2]  = j.mScale.z;
        e.quat[0]   = j.mQuat.w;
        e.quat[1]   = j.mQuat.x;
        e.quat[2]   = j.mQuat.y;
        e.quat[3]   = j.mQuat.z;
        entries.push_back(e);
    }

    std::ofstream file(path);
    if (!file.is_open())
        return false;

    file << entries.size() << "\n";
    for (auto& e : entries) {
        file << e.mNo << " " << e.offset[0] << " " << e.offset[1] << " " << e.offset[2] << " " << e.scale[0] << " " << e.scale[1] << " "
             << e.scale[2] << " " << e.quat[0] << " " << e.quat[1] << " " << e.quat[2] << " " << e.quat[3] << "\n";
    }
    return true;
}

static bool LoadPoseFromFile(UModelJoint* joints, int jointCount, const std::string& path) {
    if (!joints || jointCount <= 0)
        return false;

    std::ifstream file(path);
    if (!file.is_open())
        return false;

    int savedCount = 0;
    file >> savedCount;
    if (!file)
        return false;

    int matched = 0;
    for (int i = 0; i < savedCount; i++) {
        int mNo;
        float ox, oy, oz, sx, sy, sz, qw, qx, qy, qz;
        file >> mNo >> ox >> oy >> oz >> sx >> sy >> sz >> qw >> qx >> qy >> qz;
        if (!file)
            break;

        for (int j = 0; j < jointCount; j++) {
            if (joints[j].mNo == mNo) {
                joints[j].mOffset.x = ox;
                joints[j].mOffset.y = oy;
                joints[j].mOffset.z = oz;
                joints[j].mScale.x  = sx;
                joints[j].mScale.y  = sy;
                joints[j].mScale.z  = sz;
                joints[j].mQuat.w   = qw;
                joints[j].mQuat.x   = qx;
                joints[j].mQuat.y   = qy;
                joints[j].mQuat.z   = qz;
                matched++;
                break;
            }
        }
    }
    return true;
}

static void drawJoints(UModelJoint* joints, int jointCount, int entityId, float view[16], float projection[16], int& selectedIndex,
    bool& isManipulating, ImGuizmo::OPERATION& operation, ImGuizmo::MODE mode) {
    constexpr float MAX_DELTA_PER_FRAME = 5.0f;
    constexpr float MAX_SCALE_CHANGE    = 0.1f;

    for (int i = 0; i < jointCount; i++) {
        UModelJoint* joint = &joints[i];
        if (joint->mNo == 0 || joint->mNo >= 150)
            continue;

        int uniqueIndex = MakeJointId(entityId, i);

        glm::mat4 jointTransform = glm::make_mat4(&joint->mWmat.m1.x);

        Transformable t;
        t.worldMatrix = &jointTransform;

        glm::mat4 oldTransform, newTransform;
        bool manipulated = DrawTransformableManipulator(t, uniqueIndex, selectedIndex, isManipulating, operation, mode, view, projection,
            IM_COL32(255, 255, 0, 128), IM_COL32(0, 255, 0, 255), (selectedIndex == uniqueIndex) ? 8.0f : 5.0f, "Joint", false,
            &oldTransform, &newTransform);

        if (manipulated) {
            lastManipulatedJoints     = joints;
            lastManipulatedJointCount = jointCount;

            if (operation == ImGuizmo::TRANSLATE) {
                glm::vec3 delta = glm::vec3(newTransform[3] - oldTransform[3]);
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
                glm::mat4 localTransform = newTransform;
                if (joint->mParentIndex >= 0) {
                    glm::mat4 parentWorld = glm::make_mat4(&joints[joint->mParentIndex].mWmat.m1.x);
                    localTransform        = glm::inverse(parentWorld) * newTransform;
                }
                glm::quat localQuat = glm::normalize(glm::quat_cast(glm::mat3(localTransform)));
                joint->mQuat.w      = localQuat.w;
                joint->mQuat.x      = localQuat.x;
                joint->mQuat.y      = localQuat.y;
                joint->mQuat.z      = localQuat.z;
            }
        }
    }
}

static void drawEnemies(uEnemy_Old* entity, float view[16], float projection[16], int& selectedIndex, bool& isManipulating, ImGuizmo::OPERATION& operation) {
    while (entity) {
        int entityId    = (int)entity;
        int uniqueIndex = MakeEnemyId(entityId);

        Transformable t;
        t.position = &entity->position;
        t.scale    = &entity->scale;

        DrawTransformableManipulator(t, uniqueIndex, selectedIndex, isManipulating, operation, ImGuizmo::WORLD, view, projection,
            IM_COL32(255, 255, 0, 200), IM_COL32(255, 0, 0, 255), 12.0f, "Enemy");

        entity = entity->nextEnemy;
    }
}

static void drawAllJoints(void* ml, float view[16], float projection[16], int& selectedIndex, bool& isManipulating,
    ImGuizmo::OPERATION& operation, ImGuizmo::MODE mode) {
    uEnemy_Old* entity = (uEnemy_Old*)ml;
    while (entity) {
        int entityId = (int)entity;
        if (entity->joints /* && MtObject__isKindOf(entity, uCustomDemoActor_dti)*/) {
            drawJoints(entity->joints->joint, entity->m_joint_array_size, entityId, view, projection, selectedIndex, isManipulating,
                operation, mode);
        }
        entity = entity->nextEnemy;
    }
}

void JointDisplay::on_frame(fmilliseconds& dt) {
    if ((JointDisplay::mod_enabled || show_manipulators) && PhotoMode::mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (!player) { return; }

        if (ImGui::Begin(_("Manipulation Controls"))) {
            ImGui::Text(_("G - Translate"));
            ImGui::Text(_("R - Rotate"));
            ImGui::Text(_("S - Scale"));
            // ImGui::Text(_("T - Toggle World/Local space"));
            ImGui::End();
        }

        w2s::ImGuizmoSetup();
        float view[16], projection[16];
        if (!w2s::GetImGuizmoMatrices(view, projection))
            return;

        // if (player) {
            if (show_manipulators) {
                drawEnemies((uEnemy_Old*)devil4_sdk::get_moveline_top(13), view, projection, selectedIndex, isManipulating, currentGizmoOperation); // players
                drawEnemies((uEnemy_Old*)devil4_sdk::get_moveline_top(15), view, projection, selectedIndex, isManipulating, currentGizmoOperation); // enemies
            }

            if (JointDisplay::mod_enabled) {
                drawAllJoints((uEnemy_Old*)devil4_sdk::get_moveline_top(13), view, projection, selectedIndex, isManipulating, currentGizmoOperation, currentGizmoMode); // players
                drawAllJoints((uEnemy_Old*)devil4_sdk::get_moveline_top(15), view, projection, selectedIndex, isManipulating, currentGizmoOperation, currentGizmoMode); // enemies
            }

            w2s::ImGuizmoKeyboardShortcuts(currentGizmoOperation, currentGizmoMode);
        /* } else {
            if (show_manipulators) {
                drawEnemies(
                    (uEnemy_Old*)devil4_sdk::get_moveline_top(5), view, projection, selectedIndex, isManipulating, currentGizmoOperation);
            }

            if (JointDisplay::mod_enabled) {
                drawAllJoints((uEnemy_Old*)devil4_sdk::get_moveline_top(5), view, projection, selectedIndex, isManipulating,
                    currentGizmoOperation, currentGizmoMode);
            }

            w2s::ImGuizmoKeyboardShortcuts(currentGizmoOperation, currentGizmoMode);
        } */

        w2s::ImGuizmoDeselection(selectedIndex, isManipulating);
    }
}

void JointDisplay::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Joint Display"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Display Joints in the game world"));

        ImGui::Checkbox(_("Show Manipulators"), &show_manipulators);

        ImGui::Separator();

        bool haveEntity = lastManipulatedJoints != nullptr;
        if (!haveEntity)
            ImGui::BeginDisabled();

        if (ImGui::Button(_("Save Pose"))) {
            std::string path = OpenSaveFileDialog();
            if (!path.empty()) SavePoseToFile(lastManipulatedJoints, lastManipulatedJointCount, path);
        }
        ImGui::SameLine();
        if (ImGui::Button(_("Load Pose"))) {
            std::string path = OpenLoadFileDialog();
            if (!path.empty()) LoadPoseFromFile(lastManipulatedJoints, lastManipulatedJointCount, path);
        }

        if (!haveEntity) {
            ImGui::EndDisabled();
            ImGui::TextDisabled(_("Click a joint to select an entity."));
        }
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> JointDisplay::on_initialize() {
    return Mod::on_initialize();
}

void JointDisplay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("JointDisplay").value_or(false);
    show_manipulators = cfg.get<bool>("show_manipulators").value_or(false);
}

void JointDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("JointDisplay", mod_enabled);
    cfg.set<bool>("show_manipulators", show_manipulators);
}
