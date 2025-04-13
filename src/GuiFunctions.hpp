#pragma once

#include "Mods.hpp"
#include "imgui/imgui.h"
#include "Shellapi.h"

namespace gui {
    // gui function definitions
    inline void under_line(const ImColor& col) {
        ImVec2 min = ImGui::GetItemRectMin();
        ImVec2 max = ImGui::GetItemRectMax();
        min.y = max.y;
        ImGui::GetWindowDrawList()->AddLine(min, max, col, 1.0f);
    }
    // visual theme of the gui, can be changed to change the look
    void dark_theme();

    // helpmarker function, to avoid typing it every time in the gui section
    void help_marker(const char* desc);

    void im_gui_main_window_proc(Mods* pmods);

    struct ImGuiURL {
        std::string text;
        std::string url;
        const ImVec4 color_hover{ 0.356f, 0.764f, 0.960f, 1.00f };
        const ImVec4 color_regular{ 0.950f, 0.960f, 0.980f, 1.00f };

        void draw() {

            ImGui::TextColored(color_regular, text.c_str());
            if (ImGui::IsItemHovered()) {
                under_line(color_hover);
            }
            if (ImGui::IsItemClicked()) {
                ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
            }
        }
    };
}