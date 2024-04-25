#include "PlayerTracker.hpp"
#include "WorkRate.hpp"

#if 1
uintptr_t PlayerTracker::jmp_return{ NULL };
uPlayer* PlayerTracker::player_ptr{ NULL };
bool PlayerTracker::lock_on_alloc{ false };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8; // DevilMayCry4_DX9.exe+A558B8
bool PlayerTracker::pin_imgui_enabled = false;
static bool display_player_stats = false;

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
    return Mod::on_initialize();
}

void PlayerTracker::on_gui_frame() {
    ImGui::Checkbox("Disable Game Pause When Opening The Trainer", &WorkRate::disable_trainer_pause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Show Pin Timers", &PlayerTracker::pin_imgui_enabled);
    ImGui::SameLine();
    help_marker("Opens up a UI when pins are placed that shows the time remaining on each pin");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    SMediator* s_med_ptr = *(SMediator**)static_mediator_ptr;
    ImGui::PushItemWidth(sameLineItemWidth);
    ImGui::InputInt("s_med_ptr", (int*)&s_med_ptr, 0, 0, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox("Display Player Stats", &display_player_stats);
    ImGui::SameLine();
    help_marker("View various details about the player character");
    if (display_player_stats) {
        ImGui::Indent(lineIndent);
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            ImGui::InputFloat("HP ##1", &player->HP);
            ImGui::InputFloat("Max HP ##1", &player->HPMax);
            ImGui::InputScalar("Move ID Debug ##1", ImGuiDataType_U8, &player->moveIDBest);
            ImGui::InputFloat3("XYZ Position ##1", (float*)&player->m_pos);
            ImGui::InputFloat4("Rotation ##1", &player->rotation2);
            ImGui::InputFloat3("XYZ Scale ##1", (float*)&player->m_scale);
            ImGui::InputFloat3("XYZ Velocity ##1", (float*)&player->m_d_velocity);
            ImGui::InputFloat("Movement Speed ##1", &player->m_d_vel_magnitude);
            ImGui::InputFloat("Inertia ##1", &player->inertia);
            ImGui::SameLine();
            help_marker("Uhm, ehm, akshually, internia isn't a thing, it's a property of a thing, the shit you're showing is velocity, the "
                        "measure of inertia is mass, resistance to acceleration (slowing down is also acceleration just in the opposite "
                        "direction), if the thing that got yeeted doesn't like burn off it's layers in flight due to air friction like a "
                        "fucking meteor or some shit then it's inertia isn't changing. Get it right NERD");
            ImGui::InputScalar("Weight ##1", ImGuiDataType_U8, &player->weight);
            ImGui::InputScalar("Lock On ##1", ImGuiDataType_U8, &player->lockedOn);
            if (player->controllerID == 0) { // dante
                ImGui::InputFloat("Disaster Gauge ##1", &player->disasterGauge);
            } else { // nero
                ImGui::InputFloat("Exceed Timer ##1", &player->exceedTimer);
                ImGui::SameLine();
                help_marker("If you press exceed while this timer is between 0 and 1, you'll get MAX-Act.");
            }
            ImGui::InputFloat("Animation Frame ##1", &player->animFrame);
            ImGui::InputScalar("Move Part ##1", ImGuiDataType_U8, &player->movePart);
            ImGui::InputScalar("Move Bank ##1", ImGuiDataType_U8, &player->moveBank);
            ImGui::InputScalar("Move ID ##1", ImGuiDataType_U8, &player->moveID2);
            static int8_t tempMoveBank = 0;
            static int8_t tempMoveID   = 0;
            if (ImGui::Button("Save Current Move")) {
                tempMoveBank = player->moveBank;
                tempMoveID   = player->moveID2;
            }
            ImGui::InputScalar("Saved Move Bank ##1", ImGuiDataType_U8, &tempMoveBank);
            ImGui::InputScalar("Saved Move ID ##1", ImGuiDataType_U8, &tempMoveID);
            if (ImGui::Button("Play Saved Move")) {
                player->moveBank = tempMoveBank;
                player->moveID2  = tempMoveID;
                player->movePart = 0;
                player->canWeaponChange = 12;
            }
        }
        ImGui::Unindent(lineIndent);
    }
}

void PlayerTracker::custom_imgui_window() {
    if (pin_imgui_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            if (player->controllerID == 0) { // dante
                if (player->luciferPins[0]) {
                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags window_flags =
                        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;
                    ImVec2 window_size = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.35f);
                    ImVec2 window_pos  = ImVec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);
                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                    ImGui::Begin("Lucifer Pin Timers", NULL, window_flags);

                    for (int i = 0; i < 15; i++) {
                        if (player->luciferPins[i]) {
                            if (player->luciferPins[i]->penetrated) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                            }
                            ImGui::Text("Pin %i = %.0f", i + 1, player->luciferPins[i]->timer);
                            ImGui::SameLine();
                            ImGui::Text("/ %.0f", player->luciferPins[i]->timerMax);
                            ImGui::PopStyleColor();
                        }
                    }
                    ImGui::End();
                }
            }
        }
    }
}

void PlayerTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pin_imgui_enabled", pin_imgui_enabled);
}

void PlayerTracker::on_config_load(const utility::Config& cfg) {
    pin_imgui_enabled = cfg.get<bool>("pin_imgui_enabled").value_or(false);
}
#endif
