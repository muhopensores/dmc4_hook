#include "PlayerTracker.hpp"
#include "WorkRate.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return{ NULL };
uPlayer* PlayerTracker::player_ptr{ NULL };

bool PlayerTracker::display_player_stats = false;
Vector3f PlayerTracker::savedPlayerPosition{ 0.0f, 0.0f, 0.0f };
float PlayerTracker::savedPlayerRotation = 0.0f;
int8_t PlayerTracker::savedPlayerWeight = 0;
float PlayerTracker::savedPlayerInertia = 0;
int8_t PlayerTracker::savedPlayerMoveID   = 0;
int8_t PlayerTracker::savedPlayerMoveBank = 0;
uint8_t PlayerTracker::savedPlayerCancels1[]{ 0, 0, 0, 0 };
uint32_t PlayerTracker::savedPlayerCancels2[]{ 0, 0, 0, 0 };
uint32_t PlayerTracker::savedPlayerSword = 0;
uint32_t PlayerTracker::savedPlayerGun = 0;
uint32_t PlayerTracker::savedPlayerStyle = 0;
uint32_t PlayerTracker::savedPlayerLockonAnimation = 0;
uint32_t PlayerTracker::savedPlayerCanWeaponChange = 0;

void PlayerTracker::SavePlayerXYZ() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        savedPlayerPosition = player->m_pos;
        savedPlayerRotation = player->rotation2;
    }
}
void PlayerTracker::LoadPlayerXYZ() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        player->m_pos = savedPlayerPosition;
        player->rotation2 = savedPlayerRotation;
    }
}

void PlayerTracker::SavePlayerMove() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player) {
        savedPlayerSword = player->currentSword;
        savedPlayerGun = player->currentGun;
        savedPlayerStyle = player->currentStyle;
        savedPlayerMoveBank = player->moveBank;
        savedPlayerMoveID = player->moveID2;
        savedPlayerWeight = player->weight;
        savedPlayerInertia = player->inertia;
        savedPlayerCancels1[0] = player->cancels1[0];
        savedPlayerCancels1[1] = player->cancels1[1];
        savedPlayerCancels1[2] = player->cancels1[2];
        savedPlayerCancels1[3] = player->cancels1[3];

        savedPlayerCancels2[0] = player->cancels2[0];
        savedPlayerCancels2[1] = player->cancels2[1];
        savedPlayerCancels2[2] = player->cancels2[2];
        savedPlayerCancels2[3] = player->cancels2[3];
        savedPlayerLockonAnimation = player->isLockonAnimation;
        savedPlayerCanWeaponChange = player->canWeaponChange;
        SavePlayerXYZ();
    }
}

void PlayerTracker::LoadPlayerMove() {
    uPlayer* player = devil4_sdk::get_local_player();
    if (player && savedPlayerSword) { // verify at least 1 save has happened
        player->nextSword = savedPlayerSword;
        player->nextGun = savedPlayerGun;
        player->currentStyle = savedPlayerStyle;
        player->moveBank = savedPlayerMoveBank;
        player->moveID2 = savedPlayerMoveID;
        player->weight = savedPlayerWeight;
        player->inertia = savedPlayerInertia;
        player->movePart    = 0;
        player->cancels1[0] = savedPlayerCancels1[0];
        player->cancels1[1] = savedPlayerCancels1[1];
        player->cancels1[2] = savedPlayerCancels1[2];
        player->cancels1[3] = savedPlayerCancels1[3];

        player->cancels2[0] = savedPlayerCancels2[0];
        player->cancels2[1] = savedPlayerCancels2[1];
        player->cancels2[2] = savedPlayerCancels2[2];
        player->cancels2[3] = savedPlayerCancels2[3];
        player->isLockonAnimation = savedPlayerLockonAnimation;
        player->canWeaponChange = savedPlayerCanWeaponChange;

        // grounded does not need to be set
        // player->characterSettingsOne->groundedActual = 0;
        // player->grounded = 0;
        // player->grounded2 = 0;
        LoadPlayerXYZ();
    }
}

std::optional<std::string> PlayerTracker::on_initialize() {
    MutatorRegistry::define("PocketKing").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(0.5f, 0.5f, 0.5f);
        }
    }).set_timer(30.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });

    MutatorRegistry::define("BigGuy").on_init([] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.35f, 1.35f, 1.35f);
        }
    }).set_timer(30.0f, [] {
        auto player = devil4_sdk::get_local_player();
        if (player) {
            player->m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
        }
    });

    console->system().RegisterCommand("playerdt", "Set DT value", 
        [](float value) {
            if (uPlayer* pl_ptr = devil4_sdk::get_local_player()) {
                pl_ptr->DT = value;
            }
        }, 
        csys::Arg<float>("0.0 - 10000.0"));

    console->system().RegisterCommand("playerhp", "Set HP value", 
        [](float value) {
            if (uPlayer* pl_ptr = devil4_sdk::get_local_player()) {
                pl_ptr->HP = value;
            }
        }, 
        csys::Arg<float>("0.0 - 20000.0"));

    return Mod::on_initialize();
}

void PlayerTracker::on_update_input(utility::Input & input) {
}

static uintptr_t moveIDCallAddr{ 0x7AAC80 };
static void PlayMoveID(int moveID) {
    auto player = devil4_sdk::get_local_player();
    _asm {
        mov eax, [player]
        push -1
        mov ecx, [moveID]
        call dword ptr [moveIDCallAddr]
    }
}

static uintptr_t animIDCallAddr{ 0x7AA030 };
static void PlayAnimID(int animID) {
    auto player = devil4_sdk::get_local_player();
    _asm {
        mov ecx, [player]
        push dword ptr [animID]
        call dword ptr [animIDCallAddr]
    }
}

void PlayerTracker::on_gui_frame() {
    ImGui::Checkbox(_("Disable Game Pause When Opening The Trainer"), &WorkRate::disable_trainer_pause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    SMediator* s_med_ptr = devil4_sdk::get_sMediator();
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::InputInt("s_med_ptr", (int*)&s_med_ptr, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox(_("Display Player Stats"), &PlayerTracker::display_player_stats);
    ImGui::SameLine();
    help_marker(_("View various details about the player character"));
    if (PlayerTracker::display_player_stats) {
        ImGui::Indent(lineIndent);
        uPlayer* player = devil4_sdk::get_local_player();
        SMediator* s_med_ptr = devil4_sdk::get_sMediator();
        if (player) {
            ImGui::InputFloat(_("HP ##1"), &player->HP);
            ImGui::InputFloat(_("Max HP ##1"), &player->HPMax);
            ImGui::InputFloat(_("DT ##1"), &player->DT);
            ImGui::InputFloat(_("Max DT ##1"), &player->maxDT);
            ImGui::InputFloat(_("BP Timer ##1"), &s_med_ptr->bpTimer);
            ImGui::InputFloat3(_("XYZ Position ##1"), (float*)&player->m_pos);
            ImGui::InputFloat(_("Rotation ##1"), &player->rotation2);
            ImGui::InputFloat3(_("XYZ Scale ##1"), (float*)&player->m_scale);
            ImGui::InputFloat3(_("XYZ Velocity ##1"), (float*)&player->m_d_velocity);
            ImGui::InputFloat(_("Movement Speed ##1"), &player->m_d_vel_magnitude);
            ImGui::InputFloat(_("Inertia ##1"), &player->inertia);
            ImGui::SameLine();
            help_marker(_("Uhm, ehm, akshually, internia isn't a thing, it's a property of a thing, the shit you're showing is velocity, the "
                        "measure of inertia is mass, resistance to acceleration (slowing down is also acceleration just in the opposite "
                        "direction), if the thing that got yeeted doesn't like burn off it's layers in flight due to air friction like a "
                        "fucking meteor or some shit then it's inertia isn't changing. Get it right NERD"));
            ImGui::InputScalar(_("Weight ##1"), ImGuiDataType_U8, &player->weight);
            ImGui::InputScalar(_("Lock On ##1"), ImGuiDataType_U8, &player->lockedOn);
            if (player->controllerID == 0) { // dante
                ImGui::InputFloat(_("Disaster Gauge ##1"), &player->disasterGauge);
            } else { // nero
                ImGui::InputFloat(_("Exceed Timer ##1"), &player->exceedTimer);
                ImGui::SameLine();
                help_marker(_("If you press exceed while this timer is between 0 and 1, you'll get MAX-Act."));
            }
            ImGui::InputFloat(_("Animation Frame ##1"), &player->animFrame);
            ImGui::InputScalar(_("Move ID ##1"), ImGuiDataType_U8, &player->moveIDBest);
            ImGui::InputScalar(_("Move ID2 ##1"), ImGuiDataType_U8, &player->moveID2);
            ImGui::InputScalar(_("Move Bank ##1"), ImGuiDataType_U8, &player->moveBank);
            ImGui::InputScalar(_("Move Part ##1"), ImGuiDataType_U8, &player->movePart);
            ImGui::InputScalar(_("Saved Move Bank ##1"), ImGuiDataType_U8, &savedPlayerMoveBank);
            ImGui::InputScalar(_("Saved Move ID ##1"), ImGuiDataType_U8, &savedPlayerMoveID);
        }
        ImGui::Unindent(lineIndent);
    }
    if (ImGui::Button(_("Save Current Move"))) {
        SavePlayerMove();
    }
    ImGui::SameLine();
    if (ImGui::Button(_("Play Saved Move"))) {
        LoadPlayerMove();
    }
    ImGui::InputFloat3(_("Saved Player Position"), &savedPlayerPosition[0]);

    static int inputMoveID = 0;
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::InputInt("##InputMoveIDInputInt ##1", &inputMoveID, 1, 10, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("PlayMoveID")) {
        PlayMoveID(inputMoveID);
    }

    static int inputAnimID = 0;
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::InputInt("##InputAnimIDInputInt ##1", &inputAnimID, 1,10, ImGuiInputTextFlags_CharsHexadecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("PlayAnimID")) {
        PlayAnimID(inputAnimID);
    }
    ImGui::SameLine();
    help_marker("uhh this only plays things that don't require a certain player state idk why");
}

void PlayerTracker::on_config_save(utility::Config& cfg) {

}

void PlayerTracker::on_config_load(const utility::Config& cfg) {
}
#endif
