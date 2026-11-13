#include "Windows.h"
#include "GuiFunctions.hpp"
#include "ModsHeaders.hpp"
#include "Config.hpp"
#include "utility/Locales.hpp"
#include <string>
#include "LicenseStrings.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/ReClass_Internal.hpp"

#include "fw-imgui/imgui_impl_dx9.h"
#include "fw-imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#include <shlobj.h>

#define GUI_VERSION "dmc4_hook 1.6.3 PRE RELEASE"

static constexpr char* version{GUI_VERSION};
static constexpr float uiWidth       = 600.0f;
static constexpr float sameLineWidth = uiWidth / 2.0f; // redef in mod.hpp, I was too scared to include that here
static float maxUIHeight             = 0.0f;
static float uiHeight                = 0.0f;
static float tabHeight               = 0.0f;
static float endHeight               = 0.0f;
static float gameWindowHeight        = 0.0f;

namespace gui {

    // visual theme of the gui, can be changed to change the look

    void dark_theme() {
        ImGui::GetStyle().FrameRounding = 2.5f;
        ImGui::GetStyle().GrabRounding = 3.0f;

        auto& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(6.0f, 4.0f);
        style.WindowRounding = 6.0f;
        style.WindowBorderSize = 0.0f;
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_Button] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.25f, 0.29f, 0.95f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabSelected] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_TabDimmed] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabDimmedSelected] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavCursor] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.4588f, 0.45880f, 0.4588f, 0.35f);
    }
    /*
    void SockTheme() {
        ImGui::GetStyle().FrameRounding = 2.5f;
        ImGui::GetStyle().GrabRounding = 3.0f;

        auto& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(6, 4);
        style.WindowRounding = 3.0f;

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.0026f, 0.0026f, 0.0026f, 0.90f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.08f, 0.10f, 0.12f, 1.00f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.20f, 0.28f, 1.00f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.12f, 0.14f, 1.00f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.65f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.15f, 0.18f, 0.22f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.39f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_Button] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_Header] = ImVec4(0.20f, 0.25f, 0.29f, 0.55f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImVec4(0.20f, 0.25f, 0.29f, 0.95f);
        colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_TabActive] = ImVec4(0.0078f, 0.53f, 0.8196f, 0.95f);
        colors[ImGuiCol_TabUnfocused] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.11f, 0.15f, 0.17f, 1.00f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.4588f, 0.45880f, 0.4588f, 0.35f);
    }
    void DefaultDarkTheme() {
        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
        colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
        colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
        colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
        colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
        colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
        colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
        colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
        colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
        colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
        colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
        colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
        colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
        colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
        colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
        colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
        colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
        colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
        colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
        colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
        colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
        colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
        colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
        colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
        colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);
        colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
        colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
        colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
        colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
        colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
        colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
        colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
        colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
        colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
        colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
    }
    */

    // helpmarker function, to avoid typing it every time in the gui section
    void help_marker(const char* desc) {
        ImGui::TextDisabled("(?)");
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    void im_gui_main_window_proc(Mods* pmods) {
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        static bool showDemoWindow = false;
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        ImGui::Begin(version, NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize);
        tabHeight = 0.0f;
        if (sRender* sRender = devil4_sdk::get_sRender()) maxUIHeight = sRender->screenRes.y * 0.9f;
        else maxUIHeight = 1080.0f * 0.9f;
        ImGui::Text(_("Average %.3f ms/frame (%.1f FPS)"), 1000.0f / io.Framerate, io.Framerate);
        ImGui::SameLine();
        pmods->on_draw_ui<LocalizationManager>();
        const char* save_config_label = _("Save Config");
        ImVec2 btn_size = ImGui::CalcTextSize(save_config_label);
        btn_size.x += ImGui::GetStyle().FramePadding.x * 2.0f;
        btn_size.y += ImGui::GetStyle().FramePadding.y * 2.0f;
        float rightEdge = ImGui::GetWindowPos().x + ImGui::GetContentRegionAvail().x;
        ImGui::SameLine(rightEdge - ImGui::GetWindowPos().x - btn_size.x);
        if (ImGui::Button(save_config_label)) {
            pmods->on_config_save();
        }
        
        if (ImGui::BeginTabBar("Trainer", ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll)) {
            uiHeight = ImGui::GetCursorPosY();
            if (ImGui::BeginTabItem(_("Training"))) {
                ImGui::BeginChild("TrainingChild");

                ImGui::SeparatorText(_("Practice"));

                pmods->on_draw_ui<HealthSettings>(DISPLAY_SYSTEM_A); // needs its own line // damage modifier

                pmods->on_draw_ui<InfDT>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<InfRevive>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<RestoreMaxHp>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<StunAnything>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<RemoveLaunchArmour>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<DtEnemiesDontStun>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<BerialDaze>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<InfFaustCloak>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<InfFrostHeal>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<FreezeEnemies>(DISPLAY_SYSTEM_A);
                
                pmods->on_draw_ui<InfiniteTime>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Difficulty"));

                pmods->on_draw_ui<DifficultySelect>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<DmdBloodyPalace>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<DisableChimeraBlades>(DISPLAY_SYSTEM_A); // enemy AI mods: Environment > Enemy AI

                pmods->on_draw_ui<GhostWalls>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<EnvironmentalHazards>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Game Mode"));

                pmods->on_draw_ui<Survival>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<HealthSettings>(DISPLAY_SYSTEM_B); // needs its own line // must style

                pmods->on_draw_ui<AreaJump>(DISPLAY_SYSTEM_B); // needs its own line // random bp & boss rush

                pmods->on_draw_ui<FloorIsLava>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<DarkSoulsStamina>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<EnemyReplace>(DISPLAY_SYSTEM_B); // enemy randomizer
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<GermanWord>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<ArcadeMode>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<CharSwitcher>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<EnvironmentalHazards>(DISPLAY_SYSTEM_B); // temp platforms

                pmods->on_draw_ui<Macro>(DISPLAY_SYSTEM_A); // needs its own line
                
                // pmods->on_draw_ui<TestMod>(DISPLAY_SYSTEM_A);

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("Character"))) {
                ImGui::BeginChild("CharacterChild");
                float windowWidth = ImGui::GetWindowWidth();

                if (ImGui::BeginTabBar("CharacterTabBar", ImGuiTabBarFlags_FittingPolicyShrink)) {
                    tabHeight += ImGui::GetCursorPosY();

                    static int numTabs = 2;
                    float tabWidth = windowWidth / numTabs;
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Nero"))) {
                        ImGui::PopStyleVar(2);
                        ImGui::BeginChild("NeroChild");

                        ImGui::SeparatorText(_("Limit Removal"));

                        pmods->on_draw_ui<HeightRestriction>(DISPLAY_NERO_A); // needs its own line

                        pmods->on_draw_ui<LimitAdjust>(DISPLAY_NERO_A); // needs its own line

                        ImGui::SeparatorText(_("Abilities"));

                        pmods->on_draw_ui<RevFlying>(DISPLAY_NERO_A); // needs its own line

                        pmods->on_draw_ui<NeroSnatchLength>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<AngelSnatch>(DISPLAY_NERO_A);
                        
                        pmods->on_draw_ui<Quicksilver>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<WitchTime>(DISPLAY_NERO_A);

                        pmods->on_draw_ui<RunWhileHolding>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<NoHbKnockback>(DISPLAY_NERO_A);

                        pmods->on_draw_ui<ExceedLock>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<ExceedLimiter>(DISPLAY_NERO_A);

                        pmods->on_draw_ui<Payline>(DISPLAY_NERO_A); // needs its own line // keep on the last line, it has a download button

                        pmods->on_draw_ui<GuardianDevil>(DISPLAY_NERO_A); // keep on the last line, it has a download button

                        ImGui::SeparatorText(_("Speedups"));

                        pmods->on_draw_ui<FastSprint>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<ChargeChecker>(DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Infinite"));

                        pmods->on_draw_ui<InfAirHikes>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<FreeJc>(DISPLAY_NERO_A);

                        pmods->on_draw_ui<InfTableHopper>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<InfCalibur>(DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Accessibility"));

                        pmods->on_draw_ui<EasyJc>(DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Selective Cancels"));

                        pmods->on_draw_ui<SelectiveCancels>(DISPLAY_NERO_A); // needs its own line

                        ImGui::SeparatorText(_("Taunt Select"));
                        
                        pmods->on_draw_ui<TauntSelect>(DISPLAY_NERO_A); // needs its own line
                        
                        ImGui::SeparatorText(_("Memes"));
                        
                        pmods->on_draw_ui<BigHeadMode>(DISPLAY_NERO_A); // needs its own line
                        
                        pmods->on_draw_ui<MutatorSuperhot>(DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<PlayerRotation>(DISPLAY_NERO_A);

                        tabHeight += ImGui::GetCursorPosY();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    else {
                        ImGui::PopStyleVar(2);
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Dante"))) {
                        ImGui::PopStyleVar(2);
                        ImGui::BeginChild("DanteChild");

                        ImGui::SeparatorText(_("Limit Removal"));

                        pmods->on_draw_ui<HeightRestriction>(DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui<LimitAdjust>(DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Abilities"));

                        pmods->on_draw_ui<NoLockonRestriction>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<SwordSpin>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<NoHbKnockback>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<Quicksilver>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<TrackingFullHouse>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<TrickDown>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<ManualTwosomeTime>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<LiveMoveTableSwaps>(DISPLAY_DANTE_A); // instant honeycomb

                        pmods->on_draw_ui<ActiveBlock>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<RgMultiplier>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<KnockbackEdits>(DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui<TurnSpeedEdits>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<TrackingSkyStar>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<DisableDTStinger>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<GunStingerDistance>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<InfDreadnaught>(DISPLAY_DANTE_B); // run in dread
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<CrossGunCharge>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<PandoraCharge>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<TrackingJealousy>(DISPLAY_DANTE_A);

                        // pmods->on_draw_ui<FireworksTweaks>(DISPLAY_DANTE_A);
                        pmods->on_draw_ui<LongerJealousy>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<ReversableDraw>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<MustangWalls>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<WitchTime>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<BackForwardQuickDrive>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<DanteJdc>(DISPLAY_DANTE_A); // keep on the last line, it has a download button
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<GroundedGunCancels>(DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Lucifer"));

                        pmods->on_draw_ui<ForceLucifer>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<InputStates>(DISPLAY_DANTE_A); // taunt ecstasy

                        pmods->on_draw_ui<InfinitePinTimer>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<LuciAirThrow>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<PinTrick>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<ManualLuciferAim>(DISPLAY_DANTE_A);

                        ImGui::Spacing();
                        ImGui::Text(_("Rose"));
                        ImGui::Spacing();

                        pmods->on_draw_ui<RoseOptions>(DISPLAY_DANTE_A); // 1.5 lines
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<RoseRemovesPins>(DISPLAY_DANTE_A);

                        ImGui::Spacing();
                        ImGui::Text(_("Pins"));
                        ImGui::Spacing();

                        pmods->on_draw_ui<PinProperties>(DISPLAY_DANTE_A); // needs its own line // Gunslinger passive, Consistent embedding

                        ImGui::SeparatorText(_("Aerial grounded moves"));

                        pmods->on_draw_ui<RisingSun>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<AerialStinger>(DISPLAY_DANTE_A);

                        // pmods->on_draw_ui<AirMustang>(DISPLAY_DANTE_A);
                        // ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<AerialDrive>(DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Accessibility"));

                        pmods->on_draw_ui<SkipWeapons>(DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui<EasyJc>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<HighTimeWeaponSwitch>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<FasterFastDrive>(DISPLAY_DANTE_B); // easier quickdrive
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<TricksterDashDT>(DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Speedups"));

                        pmods->on_draw_ui<FastSprint>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<InstantTrick>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<FastPandora>(DISPLAY_DANTE_A); // needs its own line
                        
                        pmods->on_draw_ui<ChargeChecker>(DISPLAY_DANTE_A); // fast round trip charge
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<FastRoundTrip>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<FasterFastDrive>(DISPLAY_DANTE_A); // faster quickdrive

                        ImGui::SeparatorText(_("Infinite"));

                        pmods->on_draw_ui<InfTrickRange>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<InfDreadnaught>(DISPLAY_DANTE_A); // inf dread
                        
                        pmods->on_draw_ui<InfSkyStars>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<AlwaysRoyalRelease>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<InfAirHikes>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<FreeJc>(DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Darkslayer"));

                        pmods->on_draw_ui<DoubleTapDarkslayer>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<TargetChangeDarkslayer>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<DisableDarkslayer>(DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Selective Cancels"));

                        pmods->on_draw_ui<SelectiveCancels>(DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Taunt Select"));

                        pmods->on_draw_ui<TauntSelect>(DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Memes"));

                        pmods->on_draw_ui<BigHeadMode>(DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui<MutatorSuperhot>(DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<PlayerRotation>(DISPLAY_DANTE_A);

                        pmods->on_draw_ui<GuardSteer>(DISPLAY_DANTE_A);

                        tabHeight += ImGui::GetCursorPosY();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    else {
                        ImGui::PopStyleVar(2);
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("Environment"))) {
                ImGui::BeginChild("EnvironmentChild");
                float windowWidth = ImGui::GetWindowWidth();
    
                if (ImGui::BeginTabBar("EnvironmentTabBar", ImGuiTabBarFlags_FittingPolicyShrink)) {
                    tabHeight += ImGui::GetCursorPosY();
                    static int numTabs = 3;
                    float tabWidth = windowWidth / numTabs;

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Stage"))) {
                        ImGui::PopStyleVar(2);
            
                        ImGui::BeginChild("StageChild");
                        pmods->on_draw_ui<AreaJump>(DISPLAY_SYSTEM_A); // needs its own line
                        tabHeight += ImGui::GetCursorPosY();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    else {
                        ImGui::PopStyleVar(2);
                    }
        
                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Enemies"))) {
                        ImGui::PopStyleVar(2);
            
                        ImGui::BeginChild("EnemiesChild");
                        ImGui::SeparatorText(_("Enemy Spawner"));
                        pmods->on_draw_ui<EnemySpawn>(DISPLAY_SYSTEM_A); // needs its own line
                        pmods->on_draw_ui<SpawnedEnemiesAttack>(DISPLAY_SYSTEM_A);
                        ImGui::SeparatorText(_("Enemy Replacement"));
                        pmods->on_draw_ui<EnemyReplace>(DISPLAY_SYSTEM_A); // needs its own line
                        tabHeight += ImGui::GetCursorPosY();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    else {
                        ImGui::PopStyleVar(2);
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Enemy AI"))) {
                        ImGui::PopStyleVar(2);

                        ImGui::BeginChild("EnemyAIChild");
                        ImGui::SeparatorText(_("Behaviour"));

                        pmods->on_draw_ui<EnemySlotting>(DISPLAY_SYSTEM_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<DmdLevelAi>(DISPLAY_SYSTEM_A);

                        pmods->on_draw_ui<LdkWithDmd>(DISPLAY_SYSTEM_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<FrenziedBossAI>(DISPLAY_SYSTEM_A);

                        pmods->on_draw_ui<FrostsDontJump>(DISPLAY_SYSTEM_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui<AssaultsDontJump>(DISPLAY_SYSTEM_A);

                        pmods->on_draw_ui<EnemyAttackOffScreen>(DISPLAY_SYSTEM_A); // needs its own line

                        pmods->on_draw_ui<EnemyDT>(DISPLAY_SYSTEM_A); // needs its own line
#ifndef PUBLIC_BUILD
                        pmods->on_draw_ui<EnemyAI>(DISPLAY_SYSTEM_A); // Provoke, Anti-air (its own section headers)
#endif
                        tabHeight += ImGui::GetCursorPosY();
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }
                    else {
                        ImGui::PopStyleVar(2);
                    }
        
                    ImGui::EndTabBar();
                }
    
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("System"))) {
                ImGui::BeginChild("SystemChild");

                ImGui::SeparatorText(_("dmc4_hook"));
                
                pmods->on_draw_ui<DisableKeyboard>(DISPLAY_SYSTEM_B);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<TrainerPause>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<MessageDisplayMod>(DISPLAY_SYSTEM_A); // needs its own line
                
                pmods->on_draw_ui<WorkRate>(DISPLAY_SYSTEM_B);

                ImGui::SeparatorText(_("System"));

                pmods->on_draw_ui<BackgroundRendering>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<DisableKeyboard>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<FpsLimit>(DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<Borderless>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<FastStart>(DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<FpsFixes>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Misc"));

                pmods->on_draw_ui<CharacterSwap>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<NoAutomaticCharacters>(DISPLAY_SYSTEM_A); // needs its own line
                    
                pmods->on_draw_ui<CustomModelFix>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<BpPortal>(DISPLAY_SYSTEM_A); // needs to be on the left
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<PsychoMantis>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<ForceInCombat>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<SlowWalk>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Graphics"));

                pmods->on_draw_ui<GraphicsSettings>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Cutscenes"));

                pmods->on_draw_ui<CutsceneSkip>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<CutscenePause>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<DisableCameraEvents>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("HUD"));

                pmods->on_draw_ui<HideHud>(DISPLAY_SYSTEM_A); // 1.5
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<HpInOrbsDisplay>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Custom HUD elements"));

                pmods->on_draw_ui<PinTimer>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<RedOrbCompletion>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<EnemyStepDisplay>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<ExceedDisplay>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<GuardTimer>(DISPLAY_SYSTEM_A); // guard timer
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<GuardTimer>(DISPLAY_SYSTEM_B); // guard chart

                pmods->on_draw_ui<StylePoints>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<AnimationOverlay>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Camera"));

                pmods->on_draw_ui<CameraSettings>(DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<NoclipCam>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<PhotoMode>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<CameraSettings>(DISPLAY_SYSTEM_B);

                pmods->on_draw_ui<WorkRate>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<TwitchClient>(DISPLAY_SYSTEM_A); // needs its own line

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("Debug"))) {
                float startCursorY = ImGui::GetCursorPosY();
                ImGui::BeginChild("DebugChild");

                ImGui::SeparatorText(_("File Loading"));

                pmods->on_draw_ui<LoadOrder>(DISPLAY_SYSTEM_A);
                    
                ImGui::SeparatorText(_("Noclip"));

                pmods->on_draw_ui<NoClip>(DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui<InfAirHikes>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Stats"));

                pmods->on_draw_ui<MoveTable>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<KnockbackEdits>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<PlayerTracker>(DISPLAY_SYSTEM_A); // needs its own line, also contains game pause
                
                pmods->on_draw_ui<EnemyTracker>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<FlyingStats>(DISPLAY_SYSTEM_A); // needs its own line

                ImGui::SeparatorText(_("Misc"));

                pmods->on_draw_ui<VisualizeHitbox>(DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui<DebugCam>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<RoomRespawn>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<MutatorSelfAdvertisement>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<EffectColours>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<PlayerTracker>(DISPLAY_SYSTEM_B); // shadow settings

                pmods->on_draw_ui<BrainrotPalace>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<WallRun>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<NeroStyle>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<Gerb>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<DanteAirDodge>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<FreeRide>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<NeroGunslinger>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<SpineAim>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<ThirdPersonAim>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<ModelExtension>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<MoveDef>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<FileTracker>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<ViewportWindow>(DISPLAY_SYSTEM_A);
#ifndef PUBLIC_BUILD
                pmods->on_draw_ui<UnitPacks>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<ChainFix>(DISPLAY_SYSTEM_A);
                pmods->on_draw_ui<SpiderTwerk>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<Cock>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<RayTest>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<Coop>(DISPLAY_SYSTEM_A);

                pmods->on_draw_ui<AfterImage>(DISPLAY_SYSTEM_A);
#endif
                ImGui::SeparatorText(_("Multi-character Fixes"));

                pmods->on_draw_ui<MultiCharFixes>(DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Testing"));

                ImGui::Checkbox(_("View ImGui Demo"), &showDemoWindow);

                // pmods->on_draw_ui<ShaderEditor>(DISPLAY_SYSTEM_A);

                // pmods->on_draw_ui<CustomProjectile>(DISPLAY_SYSTEM_A);
                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(_("Hotkeys"))) {
                ImGui::BeginChild("HotkeysChild");

                ImGui::Spacing();

                pmods->on_hotkey_tab(*g_framework->get_input_struct());

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(_("About"))) {
                ImGui::BeginChild("AboutChild");

                ImGui::SeparatorText(_("dmc4_hook"));

                ImGui::Text("Mstislav Capusta");
                ImGui::Text("SSSiyan");
                ImGui::Text("Vieris");
                ImGui::Text("D-module");
                ImGui::Text("CrazyMelody");
                ImGui::Text("Dlupx");
                ImGui::Text("cheburrat0r");
                ImGui::Text("endneo");

                ImGui::Spacing();
                ImGui::Text(_("Translation:"));
                ImGui::Text(_("ttttchr"));
                ImGui::Text(_("raika87"));

                ImGui::Spacing();
                ImGui::Text(_("Special Thanks:"));
                ImGui::Text("socks");
                ImGui::Text("Whirling");
                ImGui::Text("Terrutas");
                ImGui::Text("Boey");
                ImGui::Text("DelusionaryKiller");
                ImGui::Text("DJMalice");
                ImGui::Text("GarudaKK");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text(_("For more info and updates visit the github:"));

                ImGuiURL repo{ "https://github.com/muhopensores/dmc4_hook", "https://github.com/muhopensores/dmc4_hook" };
                repo.draw();

                if (ImGui::CollapsingHeader(_("Frequently Asked Questions"))) {
                    ImGui::Indent();
                    if (ImGui::CollapsingHeader(_("What mods do you recommend most?"))) {
                        ImGui::TextWrapped(_("\"Fast Game Load\" and neighbouring options in the \"System\" tab. Get from your Desktop to BP in 2 seconds"));
                    }
                    if (ImGui::CollapsingHeader(_("Old mods I installed have suddenly turned on"))) {
                        ImGui::TextWrapped(_("\"HDD File Priority\" is ticked on the Debug page. If your DMC4 install contains any files left over from old mods, "
                            "this will load them. To clean up your directory you'll need to delete files manually as Steam verification does not check "
                            "newly added files"));
                    }
                    if (ImGui::CollapsingHeader(_("My combo points are stuck at 0"))) {
                        ImGui::TextWrapped(_("\"Respawn Enemies when visiting the same room multiple times\" is ticked on the Debug page. "
                            "If you didn't turn this on manually, it would have been auto ticked when you used the reload current room hotkey"));
                    }
                    if (ImGui::CollapsingHeader(_("My camera is frozen"))) {
                        ImGui::TextWrapped(_("The default hotkey for camera settings' freeze camera is Numpad 0"));
                    }
                    if (ImGui::CollapsingHeader(_("My HUD disappeared"))) {
                        ImGui::TextWrapped(_("The default hotkey for Hide HUD is -"));
                    }
                    if (ImGui::CollapsingHeader(_("I crash entering mission 1"))) {
                        ImGui::TextWrapped(_("\"Character Switcher\" crashes in this mission"));
                    }
                    if (ImGui::CollapsingHeader(_("Payline is going through enemies"))) {
                        ImGui::TextWrapped(_("Tick \"HDD File Priority\""));
                    }
                    if (ImGui::CollapsingHeader(_("Enemies are white sometimes"))) {
                        ImGui::TextWrapped(_("This is a known issue with \"HDD File Priority\""));
                    }
                    if (ImGui::CollapsingHeader(_("Where are my saves stored?"))) {
                        static char documentsPath[MAX_PATH] = {};
                        static const bool initialized = SHGetFolderPathA(nullptr, CSIDL_MYDOCUMENTS, nullptr, SHGFP_TYPE_CURRENT, documentsPath) == S_OK;
                        if (initialized) {
                            std::string savePath = std::string(documentsPath) + "\\CAPCOM\\DevilMayCry4";
                            // ImGui::TextWrapped("%s", savePath.c_str());
                            if (ImGui::Button(_("Open Save Folder"))) {
                                ShellExecuteA(nullptr, "explore", savePath.c_str(), nullptr, nullptr, 1);
                            }
                        }
                    }
                    ImGui::Unindent();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Text(_("This trainer was made using:"));

                // NOTE(): oops forgot cpp17 has CTAD hehe
                static std::array links{
                    ImGuiURL { "REFramework -> https://github.com/praydog/REFramework", "https://github.com/praydog/REFramework" },
                    ImGuiURL { "GLM -> https://github.com/g-truc/glm", "https://github.com/g-truc/glm"},
                    ImGuiURL { "Dear ImGui -> https://github.com/ocornut/imgui", "https://github.com/ocornut/imgui" },
                    ImGuiURL { "MinHook -> https://github.com/TsudaKageyu/minhook", "https://github.com/TsudaKageyu/minhook" },
                    ImGuiURL { "spdlog -> https://github.com/gabime/spdlog", "https://github.com/gabime/spdlog" },
                    ImGuiURL { "GNU gettext -> https://www.gnu.org/software/gettext/", "https://www.gnu.org/software/gettext/" },
                    ImGuiURL { "mo_file.zip -> http://number-none.com/blow/code/mo_file/index.html", "http://number-none.com/blow/code/mo_file/index.html" },
                    ImGuiURL { "pugixml -> https://pugixml.org/", "https://pugixml.org/" },
                };
                for (auto& link : links) {
                    link.draw();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::CollapsingHeader(_("Licenses"))) {
                    ImGui::TreePush(_("Licenses"));

                    struct License {
                        std::string name;
                        std::string text;
                    };

                    static std::array licenses{
                        License { "REFramework",   license::reframework },
                        License { "GLM",           license::glm },
                        License { "ImGui",         license::imgui },
                        License { "MinHook",       license::minhook },
                        License { "spdlog",        license::spdlog },
                        License { "csys",          license::csys },
                        License { "imgui_console", license::imgui_console },
                        License { "GNU gettext",   license::gnu_gettext },
                        License { "mo_file.zip",   license::naysayer_gettext },
                        License { "pugixml",       license::pugixml_license }
                    };

                    for (const auto& license : licenses) {
                        if (ImGui::CollapsingHeader(license.name.c_str())) {
                            ImGui::TextWrapped(license.text.c_str());
                        }
                    }
                    ImGui::TreePop();
                }

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("Search"))) {
                ImGui::BeginChild("SearchChild");
                static char searchBuf[128] = "";
                ImGui::SetNextItemWidth(-FLT_MIN);
                ImGui::InputText("##SearchBar", searchBuf, IM_ARRAYSIZE(searchBuf));

                const auto& mods = pmods->get_mods();
                std::vector<size_t> results;
                std::string search = searchBuf;
                std::transform(search.begin(), search.end(), search.begin(), ::tolower);

                if (!search.empty()) {
                    for (size_t i = 0; i < mods.size(); ++i) {
                        const auto& mod = mods[i];
                        auto terms = mod->get_search_terms();
                        terms.push_back(mod->get_mod_name());
                        bool match = false;
                        for (const auto& termRaw : terms) {
                            std::string term = termRaw;
                            std::transform(term.begin(), term.end(), term.begin(), ::tolower);
                            if (term.find(search) != std::string::npos) {
                                match = true;
                                break;
                            }
                        }
                        if (match) {
                            results.push_back(i);
                        }
                    }
                }

                ImGui::Separator();

                if (search.empty()) {
                    ImGui::TextDisabled("Search");
                } else if (results.empty()) {
                    ImGui::TextDisabled("No matches.");
                } else {
                    for (size_t idx : results) {
                        const auto& mod = mods[idx];
                        for (int player = 0; player < DISPLAY_MAX; ++player) {
                            ImGui::PushID((int)idx);
                            ImGui::PushID(player);
                            ImVec4 col;
                            if (player >= DISPLAY_NERO_A && player < DISPLAY_DANTE_A)
                                col = ImVec4(100 / 255.f, 150 / 255.f, 255 / 255.f, 1.0f); // n
                            else if (player >= DISPLAY_DANTE_A && PLAYER < DISPLAY_MAX)
                                col = ImVec4(255 / 255.f, 100 / 255.f, 100 / 255.f, 1.0f); // d
                            else
                                col = ImGui::GetStyleColorVec4(ImGuiCol_Text); // system
                            ImGui::PushStyleColor(ImGuiCol_Text, col);
                            pmods->on_draw_ui(pmods->get_hash(idx), player);
                            ImGui::PopStyleColor();
                            ImGui::PopID(); // player
                            ImGui::PopID(); // mod
                        }
                    }
                }

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
        endHeight = std::min(uiHeight + tabHeight, maxUIHeight);
        
        if (MessageDisplayMod::enable_scroll_transitions) {
            static float currentHeight = 0.0f;
            static float transitionSpeed = 10.0f;
            const float transitionEpsilon = 1.0f;
            float deltaTime = ImGui::GetIO().DeltaTime;
            if (currentHeight == 0.0f)
                 currentHeight = uiHeight;
            currentHeight = glm::lerp(currentHeight, endHeight, deltaTime * transitionSpeed);

            // if coming from a smaller tab we are slightly short of a full pixel
            if (std::abs(currentHeight - endHeight) < transitionEpsilon) {
                currentHeight = endHeight;
            }

            // tab height is set to 0 when tabbing in. if() will keep old dimensions
            if (tabHeight > 0.0f)
                ImGui::SetWindowSize(ImVec2(uiWidth, currentHeight));
        }
        else {
            ImGui::SetWindowSize(ImVec2(uiWidth, endHeight));
        }
        ImGui::End();
        /*ImGui::Begin("Window Height Check");
        ImGui::SetWindowPos(ImVec2(1920.f * 0.7f, 1080.0f * 0.2f));
        ImGui::Text("tabHeight %.1f", tabHeight);
        ImGui::Text("uiHeight %.1f", uiHeight);
        ImGui::End();*/
        if (showDemoWindow)
            ImGui::ShowDemoWindow(&showDemoWindow);
    }
}
