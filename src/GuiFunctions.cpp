#include "Windows.h"
#include "GuiFunctions.hpp"
#include "Config.hpp"
#include "utility/Hash.hpp"
#include "utility/Locales.hpp"
#include <string>
#include "LicenseStrings.hpp"
#include "mods/MessageDisplay.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/ReClass_Internal.hpp"

#include "fw-imgui/imgui_impl_dx9.h"
#include "fw-imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#include <shlobj.h>

#define GUI_VERSION "dmc4_hook 1.6.1"

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
        pmods->on_draw_ui("LocalizationManager"_hash);
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

                pmods->on_draw_ui("HealthSettings"_hash, DISPLAY_SYSTEM_A); // needs its own line // damage modifier

                pmods->on_draw_ui("InfDT"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("InfRevive"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("RestoreMaxHp"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("StunAnything"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("RemoveLaunchArmour"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("DtEnemiesDontStun"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("BerialDaze"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("InfFaustCloak"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("InfFrostHeal"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("FreezeEnemies"_hash, DISPLAY_SYSTEM_A);
                
                pmods->on_draw_ui("InfiniteTime"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Difficulty"));

                pmods->on_draw_ui("DifficultySelect"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("EnemySlotting"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("DmdBloodyPalace"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("DmdLevelAi"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("LdkWithDmd"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("FrenziedBossAI"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EnemyDT"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("EnemyAttackOffScreen"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("DisableChimeraBlades"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("FrostsDontJump"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("AssaultsDontJump"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("GhostWalls"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EnvironmentalHazards"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Game Mode"));

                pmods->on_draw_ui("Survival"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("HealthSettings"_hash, DISPLAY_SYSTEM_B); // needs its own line // must style

                pmods->on_draw_ui("AreaJump"_hash, DISPLAY_SYSTEM_B); // needs its own line // random bp & boss rush

                pmods->on_draw_ui("FloorIsLava"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("DarkSoulsStamina"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EnemyReplace"_hash, DISPLAY_SYSTEM_B); // enemy randomizer
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("GermanWord"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("ArcadeMode"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("CharSwitcher"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EnvironmentalHazards"_hash, DISPLAY_SYSTEM_B); // temp platforms

                pmods->on_draw_ui("Macro"_hash, DISPLAY_SYSTEM_A); // needs its own line
                
                // pmods->on_draw_ui("TestMod"_hash, DISPLAY_SYSTEM_A);

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

                        pmods->on_draw_ui("HeightRestriction"_hash, DISPLAY_NERO_A); // needs its own line

                        pmods->on_draw_ui("LimitAdjust"_hash, DISPLAY_NERO_A); // needs its own line

                        ImGui::SeparatorText(_("Abilities"));

                        pmods->on_draw_ui("RevFlying"_hash, DISPLAY_NERO_A); // needs its own line

                        pmods->on_draw_ui("NeroSnatchLength"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("AngelSnatch"_hash, DISPLAY_NERO_A);
                        
                        pmods->on_draw_ui("Quicksilver"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("WitchTime"_hash, DISPLAY_NERO_A);

                        pmods->on_draw_ui("RunWhileHolding"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("NoHbKnockback"_hash, DISPLAY_NERO_A);

                        pmods->on_draw_ui("ExceedLock"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("ExceedLimiter"_hash, DISPLAY_NERO_A);

                        pmods->on_draw_ui("Payline"_hash, DISPLAY_NERO_A); // needs its own line // keep on the last line, it has a download button

                        pmods->on_draw_ui("DtKnuckle"_hash, DISPLAY_NERO_A); // keep on the last line, it has a download button

                        ImGui::SeparatorText(_("Speedups"));

                        pmods->on_draw_ui("FastSprint"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("ChargeChecker"_hash, DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Infinite"));

                        pmods->on_draw_ui("InfAirHikes"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("FreeJc"_hash, DISPLAY_NERO_A);

                        pmods->on_draw_ui("InfTableHopper"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("InfCalibur"_hash, DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Accessibility"));

                        pmods->on_draw_ui("EasyJc"_hash, DISPLAY_NERO_A);

                        ImGui::SeparatorText(_("Selective Cancels"));

                        pmods->on_draw_ui("SelectiveCancels"_hash, DISPLAY_NERO_A); // needs its own line

                        ImGui::SeparatorText(_("Taunt Select"));
                        
                        pmods->on_draw_ui("TauntSelect"_hash, DISPLAY_NERO_A); // needs its own line
                        
                        ImGui::SeparatorText(_("Memes"));
                        
                        pmods->on_draw_ui("BigHeadMode"_hash, DISPLAY_NERO_A); // needs its own line
                        
                        pmods->on_draw_ui("MutatorSuperhot"_hash, DISPLAY_NERO_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("PlayerRotation"_hash, DISPLAY_NERO_A);

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

                        pmods->on_draw_ui("HeightRestriction"_hash, DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui("LimitAdjust"_hash, DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Abilities"));

                        pmods->on_draw_ui("NoLockonRestriction"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("SwordSpin"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("NoHbKnockback"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("Quicksilver"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("TrackingFullHouse"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("TrickDown"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("ManualTwosomeTime"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("LiveMoveTableSwaps"_hash, DISPLAY_DANTE_A); // instant honeycomb

                        pmods->on_draw_ui("ActiveBlock"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("RgMultiplier"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("KnockbackEdits"_hash, DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui("TurnSpeedEdits"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("TrackingSkyStar"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("DisableDTStinger"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("GunStingerDistance"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("InfDreadnaught"_hash, DISPLAY_DANTE_B); // run in dread
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("CrossGunCharge"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("PandoraCharge"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("TrackingJealousy"_hash, DISPLAY_DANTE_A);

                        // pmods->on_draw_ui("FireworksTweaks"_hash, DISPLAY_DANTE_A);
                        pmods->on_draw_ui("LongerJealousy"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("ReversableDraw"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("MustangWalls"_hash, DISPLAY_DANTE_A); 
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("WitchTime"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("BackForwardQuickDrive"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("DanteJdc"_hash, DISPLAY_DANTE_A); // keep on the last line, it has a download button
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("GroundedGunCancels"_hash, DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Lucifer"));

                        pmods->on_draw_ui("ForceLucifer"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("InputStates"_hash, DISPLAY_DANTE_A); // taunt ecstasy

                        pmods->on_draw_ui("InfinitePinTimer"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("LuciAirThrow"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("PinTrick"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("ManualLuciferAim"_hash, DISPLAY_DANTE_A);

                        ImGui::Spacing();
                        ImGui::Text(_("Rose"));
                        ImGui::Spacing();

                        pmods->on_draw_ui("RoseOptions"_hash, DISPLAY_DANTE_A); // 1.5 lines
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("RoseRemovesPins"_hash, DISPLAY_DANTE_A);

                        ImGui::Spacing();
                        ImGui::Text(_("Pins"));
                        ImGui::Spacing();

                        pmods->on_draw_ui("PinProperties"_hash, DISPLAY_DANTE_A); // needs its own line // Gunslinger passive, Consistent embedding

                        ImGui::SeparatorText(_("Aerial grounded moves"));

                        pmods->on_draw_ui("RisingSun"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("AerialStinger"_hash, DISPLAY_DANTE_A);

                        // pmods->on_draw_ui("AirMustang"_hash, DISPLAY_DANTE_A);
                        // ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("AerialDrive"_hash, DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Accessibility"));

                        pmods->on_draw_ui("SkipWeapons"_hash, DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui("EasyJc"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("HighTimeWeaponSwitch"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("FasterFastDrive"_hash, DISPLAY_DANTE_B); // easier quickdrive
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("TricksterDashDT"_hash, DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Speedups"));

                        pmods->on_draw_ui("FastSprint"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("InstantTrick"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("FastPandora"_hash, DISPLAY_DANTE_A); // needs its own line
                        
                        pmods->on_draw_ui("ChargeChecker"_hash, DISPLAY_DANTE_A); // fast round trip charge
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("FastRoundTrip"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("FasterFastDrive"_hash, DISPLAY_DANTE_A); // faster quickdrive

                        ImGui::SeparatorText(_("Infinite"));

                        pmods->on_draw_ui("InfTrickRange"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("InfDreadnaught"_hash, DISPLAY_DANTE_A); // inf dread
                        
                        pmods->on_draw_ui("InfSkyStars"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("AlwaysRoyalRelease"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("InfAirHikes"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("FreeJc"_hash, DISPLAY_DANTE_A);

                        ImGui::SeparatorText(_("Darkslayer"));

                        pmods->on_draw_ui("DoubleTapDarkslayer"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("TargetChangeDarkslayer"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("DisableDarkslayer"_hash, DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Selective Cancels"));

                        pmods->on_draw_ui("SelectiveCancels"_hash, DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Taunt Select"));

                        pmods->on_draw_ui("TauntSelect"_hash, DISPLAY_DANTE_A); // needs its own line

                        ImGui::SeparatorText(_("Memes"));

                        pmods->on_draw_ui("BigHeadMode"_hash, DISPLAY_DANTE_A); // needs its own line

                        pmods->on_draw_ui("MutatorSuperhot"_hash, DISPLAY_DANTE_A);
                        ImGui::SameLine(sameLineWidth);
                        pmods->on_draw_ui("PlayerRotation"_hash, DISPLAY_DANTE_A);

                        pmods->on_draw_ui("GuardSteer"_hash, DISPLAY_DANTE_A);

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
                    static int numTabs = 2;
                    float tabWidth = windowWidth / numTabs;

                    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(tabWidth / 2, ImGui::GetStyle().FramePadding.y));
                    if (ImGui::BeginTabItem(_("Stage"))) {
                        ImGui::PopStyleVar(2);
            
                        ImGui::BeginChild("StageChild");
                        pmods->on_draw_ui("AreaJump"_hash, DISPLAY_SYSTEM_A); // needs its own line
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
                        pmods->on_draw_ui("EnemySpawn"_hash, DISPLAY_SYSTEM_A); // needs its own line
                        pmods->on_draw_ui("SpawnedEnemiesAttack"_hash, DISPLAY_SYSTEM_A);
                        ImGui::SeparatorText(_("Enemy Replacement"));
                        pmods->on_draw_ui("EnemyReplace"_hash, DISPLAY_SYSTEM_A); // needs its own line
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
                
                pmods->on_draw_ui("DisableKeyboard"_hash, DISPLAY_SYSTEM_B);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("TrainerPause"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("MessageDisplayMod"_hash, DISPLAY_SYSTEM_A); // needs its own line
                
                pmods->on_draw_ui("WorkRate"_hash, DISPLAY_SYSTEM_B);

                ImGui::SeparatorText(_("System"));

                pmods->on_draw_ui("BackgroundRendering"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("DisableKeyboard"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("FpsLimit"_hash, DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("Borderless"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("FastStart"_hash, DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("FpsFixes"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Misc"));

                pmods->on_draw_ui("CharacterSwap"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("NoAutomaticCharacters"_hash, DISPLAY_SYSTEM_A); // needs its own line
                    
                pmods->on_draw_ui("CustomModelFix"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("BpPortal"_hash, DISPLAY_SYSTEM_A); // needs to be on the left
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("PsychoMantis"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("ForceInCombat"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("SlowWalk"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Graphics"));

                pmods->on_draw_ui("GraphicsSettings"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Cutscenes"));

                pmods->on_draw_ui("CutsceneSkip"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("CutscenePause"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("DisableCameraEvents"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("HUD"));

                pmods->on_draw_ui("HideHud"_hash, DISPLAY_SYSTEM_A); // 1.5
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("HpInOrbsDisplay"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Custom HUD elements"));

                pmods->on_draw_ui("PinTimer"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("RedOrbCompletion"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EnemyStepDisplay"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("ExceedDisplay"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("GuardTimer"_hash, DISPLAY_SYSTEM_A); // guard timer
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("GuardTimer"_hash, DISPLAY_SYSTEM_B); // guard chart

                pmods->on_draw_ui("StylePoints"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("AnimationOverlay"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Camera"));

                pmods->on_draw_ui("CameraSettings"_hash, DISPLAY_SYSTEM_A);
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("NoclipCam"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("PhotoMode"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("CameraSettings"_hash, DISPLAY_SYSTEM_B);

                pmods->on_draw_ui("WorkRate"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("TwitchClient"_hash, DISPLAY_SYSTEM_A); // needs its own line

                tabHeight += ImGui::GetCursorPosY();
                ImGui::EndChild();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem(_("Debug"))) {
                float startCursorY = ImGui::GetCursorPosY();
                ImGui::BeginChild("DebugChild");

                ImGui::SeparatorText(_("File Loading"));

                pmods->on_draw_ui("LoadOrder"_hash, DISPLAY_SYSTEM_A);
                    
                ImGui::SeparatorText(_("Noclip"));

                pmods->on_draw_ui("NoClip"_hash, DISPLAY_SYSTEM_A); // 1.5 lines
                ImGui::SameLine(sameLineWidth);
                pmods->on_draw_ui("InfAirHikes"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Stats"));

                pmods->on_draw_ui("MoveTable"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("KnockbackEdits"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("PlayerTracker"_hash, DISPLAY_SYSTEM_A); // needs its own line, also contains game pause
                
                pmods->on_draw_ui("EnemyTracker"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("FlyingStats"_hash, DISPLAY_SYSTEM_A); // needs its own line

                ImGui::SeparatorText(_("Misc"));

                pmods->on_draw_ui("VisualizeHitbox"_hash, DISPLAY_SYSTEM_A); // needs its own line

                pmods->on_draw_ui("DebugCam"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("RoomRespawn"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("MutatorSelfAdvertisement"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("EffectColours"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("PlayerTracker"_hash, DISPLAY_SYSTEM_B); // shadow settings
#ifndef PUBLIC_BUILD
                pmods->on_draw_ui("SpiderTwerk"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("FriendlyFire"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("Cock"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("RayTest"_hash, DISPLAY_SYSTEM_A);

                pmods->on_draw_ui("Coop"_hash, DISPLAY_SYSTEM_A);
#endif
                ImGui::SeparatorText(_("Multi-character Fixes"));

                pmods->on_draw_ui("MultiCharFixes"_hash, DISPLAY_SYSTEM_A);

                ImGui::SeparatorText(_("Testing"));

                ImGui::Checkbox(_("View ImGui Demo"), &showDemoWindow);

                // pmods->onDrawUI("ShaderEditor"_hash, DISPLAY_SYSTEM_A);

                // pmods->on_draw_ui("CustomProjectile"_hash, DISPLAY_SYSTEM_A);
#ifndef PUBLIC_BUILD
                pmods->on_draw_ui("AfterImage"_hash, DISPLAY_SYSTEM_A);
#endif
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
                        License{ "REFramework",   license::reframework },
                        License{ "GLM",           license::glm },
                        License{ "ImGui",         license::imgui },
                        License{ "MinHook",       license::minhook },
                        License{ "spdlog",        license::spdlog },
                        License{ "csys",          license::csys },
                        License{ "imgui_console", license::imgui_console },
                        License{ "GNU gettext",   license::gnu_gettext },
                        License{ "mo_file.zip",   license::naysayer_gettext },
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
