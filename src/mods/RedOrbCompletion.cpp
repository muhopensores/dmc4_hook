#include "RedOrbCompletion.hpp"
#include "../sdk/Devil4.hpp"

static float G_WINDOW_HEIGHT_HACK_IDK_ORBS{ 270.0f };
bool RedOrbCompletion::mod_enabled{ false };

std::optional<std::string> RedOrbCompletion::on_initialize() {
    console->system().RegisterCommand("redorbcompletion", "Enable red orb completion HUD", []() {
        RedOrbCompletion::mod_enabled = !RedOrbCompletion::mod_enabled;
    });

	return Mod::on_initialize();
}

void RedOrbCompletion::custom_imgui_window() {
    if (RedOrbCompletion::mod_enabled) {
        if (devil4_sdk::get_local_player()) {
            static float orbsFoundPercent = 0.0f;
            SMediator* sMediatorPtr = devil4_sdk::get_sMediator();
            if (sMediatorPtr->orbMissionPotential && sMediatorPtr->orbMissionCurrent && sMediatorPtr->missionID != 50) { // always shows 50 for BP)
                ImGuiIO& io = ImGui::GetIO();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;

                // bandaid
                const float text_height = ImGui::GetTextLineHeightWithSpacing();
                const glm::vec2 margin = glm::vec2(text_height * 1.666f, text_height * 5.6f);
                glm::vec2 window_size = glm::vec2(400 + margin.x, G_WINDOW_HEIGHT_HACK_IDK_ORBS);
                glm::vec2 window_pos = glm::vec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                ImGui::SetNextWindowSize(window_size);
                ImGui::Begin("Red Orb Completion HUD", NULL, window_flags);
                ImGui::NewLine();

                ImGui::Text(_("Found Orbs: %i"), sMediatorPtr->orbMissionCurrent);
                ImGui::Text(_("Potential Orbs: %i"), sMediatorPtr->orbMissionPotential);

                orbsFoundPercent = ((float)sMediatorPtr->orbMissionCurrent / (float)sMediatorPtr->orbMissionPotential) * 100.0f;
                ImGui::Text(_("%.0f%% Orbs found up to current point"), orbsFoundPercent);
                if (orbsFoundPercent < 45.0f) ImGui::Text("D");
                else if (orbsFoundPercent < 60.0f) ImGui::Text("C");
                else if (orbsFoundPercent < 75.0f) ImGui::Text("B");
                else if (orbsFoundPercent < 95.0f) ImGui::Text("A");
                else ImGui::Text("S");
                G_WINDOW_HEIGHT_HACK_IDK_ORBS = ImGui::GetCursorPosY();
                ImGui::End();
            }
        }
    }
}

void RedOrbCompletion::on_gui_frame() {
    ImGui::Checkbox(_("Show Red Orb Completion %"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Opens up a UI that shows the orb % you've picked up on the current mission"));
}

void RedOrbCompletion::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("red_orb_completion_imgui_enabled", mod_enabled);
}

void RedOrbCompletion::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("red_orb_completion_imgui_enabled").value_or(false);
}
