#include "RedOrbCompletion.hpp"
#include "../sdk/Devil4.hpp"

bool RedOrbCompletion::mod_enabled = false;
ImVec2 RedOrbCompletion::window_pos{ 0.0f, 0.0f };

std::optional<std::string> RedOrbCompletion::on_initialize() {
    console->system().RegisterCommand("redorbcompletion", "Enable red orb completion HUD", []() {
        RedOrbCompletion::mod_enabled = !RedOrbCompletion::mod_enabled;
    });

	return Mod::on_initialize();
}

void RedOrbCompletion::on_frame(fmilliseconds& dt) {
    if (RedOrbCompletion::mod_enabled) {
        if (devil4_sdk::get_local_player()) {
            SMediator* sMediatorPtr = devil4_sdk::get_sMediator();
            if (sMediatorPtr && sMediatorPtr->missionID != 50) { // always shows 50 for BP
                static float orbsFoundPercent = 0.0f;
                ImGuiIO& io = ImGui::GetIO();
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize;
                
                ImVec4 shadowColor(0.0f, 0.0f, 0.0f, 0.7f);
                float shadowOffsetX = 2.0f;
                float shadowOffsetY = 2.0f;
                
                const float text_height = ImGui::GetTextLineHeightWithSpacing();
                ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                ImGui::Begin("Red Orb Completion HUD", NULL, window_flags);
                window_pos = ImGui::GetWindowPos();
                
                orbsFoundPercent = ((float)sMediatorPtr->orbMissionCurrent / (float)sMediatorPtr->orbMissionPotential) * 100.0f;
                const char* styleRank;
                if (orbsFoundPercent < 45.0f) styleRank = "D";
                else if (orbsFoundPercent < 60.0f) styleRank = "C";
                else if (orbsFoundPercent < 75.0f) styleRank = "B";
                else if (orbsFoundPercent < 95.0f) styleRank = "A";
                else styleRank = "S";
                
                ImVec2 currentPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(currentPos.x + shadowOffsetX, currentPos.y + shadowOffsetY));
                ImGui::PushStyleColor(ImGuiCol_Text, shadowColor);
                ImGui::Text(_("Found Orbs: %i"), sMediatorPtr->orbMissionCurrent);
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(currentPos);
                ImGui::Text(_("Found Orbs: %i"), sMediatorPtr->orbMissionCurrent);
                
                currentPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(currentPos.x + shadowOffsetX, currentPos.y + shadowOffsetY));
                ImGui::PushStyleColor(ImGuiCol_Text, shadowColor);
                ImGui::Text(_("Potential Orbs: %i"), sMediatorPtr->orbMissionPotential);
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(currentPos);
                ImGui::Text(_("Potential Orbs: %i"), sMediatorPtr->orbMissionPotential);
                
                currentPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(currentPos.x + shadowOffsetX, currentPos.y + shadowOffsetY));
                ImGui::PushStyleColor(ImGuiCol_Text, shadowColor);
                ImGui::Text(_("%.0f%% Orbs found up to current point"), orbsFoundPercent);
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(currentPos);
                ImGui::Text(_("%.0f%% Orbs found up to current point"), orbsFoundPercent);
                
                currentPos = ImGui::GetCursorPos();
                ImGui::SetCursorPos(ImVec2(currentPos.x + shadowOffsetX, currentPos.y + shadowOffsetY));
                ImGui::PushStyleColor(ImGuiCol_Text, shadowColor);
                ImGui::Text("%s", styleRank);
                ImGui::PopStyleColor();
                ImGui::SetCursorPos(currentPos);
                ImGui::Text("%s", styleRank);
                
                ImGui::End();
            }
        }
    }
}

void RedOrbCompletion::on_gui_frame(int display) {
    ImGui::Checkbox(_("Show Red Orb Completion %"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Opens up a UI that shows the orb % you've picked up on the current mission"));
}

void RedOrbCompletion::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("red_orb_completion_imgui_enabled", mod_enabled);
    cfg.set<float>("red_orb_completion_imgui_window_pos_x", window_pos.x);
    cfg.set<float>("red_orb_completion_imgui_window_pos_y", window_pos.y);
}

void RedOrbCompletion::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("red_orb_completion_imgui_enabled").value_or(false);
    window_pos.x = cfg.get<float>("red_orb_completion_imgui_window_pos_x").value_or(0.0f);
    window_pos.y = cfg.get<float>("red_orb_completion_imgui_window_pos_y").value_or(0.0f);
}
