#include "GuiFunctions.hpp"
#include "Config.hpp"
#include "utility/Hash.hpp"
#include <string>
#include "LicenseStrings.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN

#include "./mods/BpPortal.hpp"
#include "./mods/EnemySpawn.hpp"
#include "./mods/EnemyStats.hpp"
#include "./mods/InfAllHealth.hpp"
#include "./mods/InfDT.hpp"
#include "./mods/NoClip.hpp"
#include "./mods/OneHitKill.hpp"
#include "./mods/Quicksilver.hpp"


#include "fw-imgui/imgui_impl_dx9.h"
#include "fw-imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

#define GUI_VERSION "DMC4Hook 1.4.3"

static constexpr char* version{GUI_VERSION};
static float g_window_height_hack{ 1080.0f };
static float g_max{ 0.0f };
static float windowWidth   = 550.0f;
static float sameLineWidth = 250.0f; // another copy of this is in mod.hpp, I was too scared to include that here
namespace gui {

    // visual theme of the gui, can be changed to change the look

    void dark_theme() {
        ImGui::GetStyle().FrameRounding = 2.5f;
        ImGui::GetStyle().GrabRounding = 3.0f;

        auto& style = ImGui::GetStyle();
        style.WindowPadding = ImVec2(6, 4);
        style.WindowRounding = 6.0f;

        ImVec4* colors = ImGui::GetStyle().Colors;
        colors[ImGuiCol_Text] = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        colors[ImGuiCol_TextDisabled] = ImVec4(0.36f, 0.42f, 0.47f, 1.00f);
        colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_ChildBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
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
        colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.20f, 0.25f, 0.29f, 1.00f);
        colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.18f, 0.22f, 0.25f, 1.00f);
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

    void set_window_props() {
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = NULL;
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
        g_max = io.DisplaySize[1] * 0.9f;
        ImGui::SetNextWindowSize(ImVec2(windowWidth, io.DisplaySize[1] * 0.9f));
    }

    // imgui::being seperated into function (required to make gui overlay work, see imgui example and documentation
    void begin_drawing() {
        ImGui::Begin(version, NULL, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    }

    // function that draws the fps onto the gui
    void fps_drawing() {
        ImGui::Text("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    }

    // function for putting credits specific things in the gui
    void credits_drawing() {
        static ImVec4 color1 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        static ImVec4 color2 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        static ImVec4 color3 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        static ImVec4 color4 = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
        if (ImGui::BeginTabItem("About")) {
            ImGui::BeginChild("AboutChild");
            ImGui::Spacing();
            ImGui::Text("DMC4Hook - Devil May Cry 4 Trainer");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("Mstislav Capusta");
            ImGui::Text("SSSiyan");
            ImGui::Spacing();
            ImGui::Text("Special Thanks:");
            ImGui::Text("socks");
            ImGui::Text("Whirling");
            ImGui::Text("Terrutas");
            ImGui::Text("Boey");
            ImGui::Text("DelusionaryKiller");
            ImGui::Text("DJMalice");
            ImGui::Text("cheburrat0r");
            ImGui::Text("endneo");
            ImGui::Text("CrazyMelody");
            ImGui::Text("Dlupx");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::Text("For more info and updates visit the github:");

            ImGuiURL repo{ "https://github.com/muhopensores/dmc4_hook", "https://github.com/muhopensores/dmc4_hook" };
            repo.draw();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("This trainer was made using:");

            static std::array<ImGuiURL, 5> links {
                ImGuiURL { "REFramework https://github.com/praydog/REFramework", "https://github.com/praydog/REFramework" },
                ImGuiURL { "MinHook https://github.com/TsudaKageyu/minhook", "https://github.com/TsudaKageyu/minhook" },
                ImGuiURL { "Dear ImGui https://github.com/ocornut/imgui", "https://github.com/ocornut/imgui" },
                ImGuiURL { "GLM https://github.com/g-truc/glm", "https://github.com/g-truc/glm"},
            };
            for (auto& link: links) {
                link.draw();
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::Text("Boring license stuff");
            if (ImGui::CollapsingHeader("Licenses")) {
                ImGui::TreePush("Licenses");

                struct License {
                    std::string name;
                    std::string text;
                };

                static std::array<License, 5> licenses{
                    License{ "reframework", license::reframework},
                    License{ "glm", license::glm },
                    License{ "imgui", license::imgui },
                    License{ "minhook", license::minhook },
                    License{ "spdlog", license::spdlog },
                };

                for (const auto& license : licenses) {
                    if (ImGui::CollapsingHeader(license.name.c_str())) {
                        ImGui::TextWrapped(license.text.c_str());
                    }
                }

                ImGui::TreePop();
            }
            g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
    }

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
        // specific imgui functions, can be looked up in examples or the documentation
        // references/ points to other functions to apply logic behind the gui toggles/ objects
        {
            ImGui::SetNextWindowSize(ImVec2(windowWidth, g_window_height_hack));
            begin_drawing();
            ImGui::SameLine(0, 0);
            fps_drawing();
            ImGui::Spacing();
            pmods->on_draw_ui("Borderless"_hash);
            ImGui::SameLine(windowWidth-100.0f);
            if (ImGui::Button("Save Config")) {
                pmods->on_config_save();
            }
            if (ImGui::BeginTabBar("Trainer", ImGuiTabBarFlags_FittingPolicyMask_ ^ ImGuiTabBarFlags_FittingPolicyScroll)) {
                if (ImGui::BeginTabItem("General")) {
                    ImGui::BeginChild("GeneralChild");
                    ImGui::Spacing();
                    ImGui::Text("General");
                    ImGui::Spacing();

                    pmods->on_draw_ui("DamageMultiplier"_hash); // needs its own line

                    pmods->on_draw_ui("InfAllHealth"_hash); // needs its own line

                    pmods->on_draw_ui("OneHitKill"_hash); // needs its own line

                    // pmods->onDrawUI("InfPlayerHealth"_hash);

                    pmods->on_draw_ui("InfDT"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InfRevive"_hash);

                    pmods->on_draw_ui("RestoreMaxHp"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Enemy Difficulty");
                    ImGui::Spacing();

                    pmods->on_draw_ui("DifficultySelect"_hash); // needs its own line

                    pmods->on_draw_ui("EnemyDT"_hash);

                    pmods->on_draw_ui("DmdLevelAi"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("LdkWithDmd"_hash);

                    pmods->on_draw_ui("DisableChimeraBlades"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("FrostsDontJump"_hash);

                    pmods->on_draw_ui("EnemyAttackOffScreen"_hash); // needs its own line

                    pmods->on_draw_ui("EnemySlotting"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Bloody Palace");
                    ImGui::Spacing();

                    pmods->on_draw_ui("BpJumpHook"_hash); // needs its own line

                    pmods->on_draw_ui("DmdBloodyPalace"_hash);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Enemy Options");
                    ImGui::Spacing();

                    pmods->on_draw_ui("StunAnything"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("RemoveLaunchArmour"_hash);

                    pmods->on_draw_ui("DtEnemiesDontStun"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InfFaustCloak"_hash);

                    pmods->on_draw_ui("FreezeEnemies"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("BerialDaze"_hash);
                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Character")) {
                    ImGui::BeginChild("CharacterChild");
                    ImGui::Spacing();
                    ImGui::Text("Limit Removal");
                    ImGui::Spacing();

                    pmods->on_draw_ui("LimitAdjust"_hash); // needs its own line

                    pmods->on_draw_ui("HeightRestrictionNero"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("HeightRestrictionDante"_hash);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Shared Abilities");
                    ImGui::Spacing();

                    pmods->on_draw_ui("FastSprint"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("NoHbKnockback"_hash);

                    pmods->on_draw_ui("EasyJc"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("FreeJc"_hash);

                    pmods->on_draw_ui("NoClip"_hash); // own line
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("NoDtCooldown"_hash);
                    
                    pmods->on_draw_ui("InfAirHikes"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("Quicksilver"_hash);

                    pmods->on_draw_ui("BigHeadMode"_hash); // needs its own line

                    pmods->on_draw_ui("ModTwCmdSuperhot"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Nero Abilities");
                    ImGui::Spacing();

                    pmods->on_draw_ui("NeroFullHouse"_hash); // needs its own line

                    pmods->on_draw_ui("NeroSnatchLength"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InfTableHopper"_hash);

                    pmods->on_draw_ui("InfCalibur"_hash);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Dante Abilities");
                    ImGui::Spacing();

                    pmods->on_draw_ui("TrackingFullHouse"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("TrickDown"_hash);

                    pmods->on_draw_ui("SkipWeapons"_hash); // needs its own line

                    pmods->on_draw_ui("ManualTwosomeTime"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("FastPandora"_hash);

                    pmods->on_draw_ui("TimerMem"_hash); // instant honeycomb
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("ChargeChecker"_hash); // currently just faster roundtrip

                    pmods->on_draw_ui("ActiveBlock"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("KnockbackEdits"_hash); // currently just release stun

                    pmods->on_draw_ui("InfTrickRange"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InstantTrick"_hash);

                    pmods->on_draw_ui("InfSkyStars"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InfDreadnaught"_hash);

                    pmods->on_draw_ui("FasterFastDrive"_hash); // needs its own line, has easy fast drive too

                    pmods->on_draw_ui("RgMultiplier"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("TurnSpeedEdits"_hash);

                    pmods->on_draw_ui("TrackingSkyStar"_hash);

                    ImGui::Spacing();
                    ImGui::Text("Lucifer");
                    ImGui::Spacing();

                    pmods->on_draw_ui("RoseRemovesPins"_hash); // needs its own line
                    
                    pmods->on_draw_ui("ForceLucifer"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("InputStates"_hash); // taunt ecstasy

                    ImGui::Spacing();
                    ImGui::Text("Rose");
                    ImGui::Spacing();

                    pmods->on_draw_ui("RoseOptions"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    pmods->on_draw_ui("SelectiveCancels"_hash); // needs its own line

                    pmods->on_draw_ui("DisableDarkslayer"_hash); // needs its own line
                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Environment")) {
                    ImGui::BeginChild("EnvironmentChild");
                    ImGui::Spacing();
                    pmods->on_draw_ui("AreaJump"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    pmods->on_draw_ui("EnemySpawn"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    // pmods->onDrawUI("EnemyReplace"_hash); // needs its own line
                    pmods->on_draw_ui("EnemyReplaceAgain"_hash); // needs its own line

                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("System")) {
                    ImGui::BeginChild("SystemChild");

                    ImGui::Spacing();
                    ImGui::Text("HUD");

                    ImGui::Spacing();

                    pmods->on_draw_ui("InfiniteTime"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("HpInOrbsDisplay"_hash);

                    pmods->on_draw_ui("HideHud"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Cutscenes");

                    ImGui::Spacing();

                    pmods->on_draw_ui("DisableCameraEvents"_hash);

                    pmods->on_draw_ui("CutsceneSkip"_hash);

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("Misc");

                    ImGui::Spacing();

                    pmods->on_draw_ui("CharacterSwap"_hash); // needs its own line

                    pmods->on_draw_ui("NoAutomaticCharacters"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("SlowWalk"_hash);

                    pmods->on_draw_ui("BpPortal"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    ImGui::Text("System");

                    ImGui::Spacing();

                    pmods->on_draw_ui("BackgroundRendering"_hash);
                    ImGui::SameLine(sameLineWidth);
                    pmods->on_draw_ui("MessageDisplayMod"_hash);

                    pmods->on_draw_ui("TwCmdPlayerTransforms"_hash); // empty

                    pmods->on_draw_ui("FastStart"_hash); // needs its own line

                    pmods->on_draw_ui("FpsLimit"_hash); // needs its own line

                    ImGui::Spacing();
                    ImGui::Separator();
                    ImGui::Spacing();

                    pmods->on_draw_ui("WorkRate"_hash); // needs its own line

                    pmods->on_draw_ui("CameraSettings"_hash); // needs its own line

                    pmods->on_draw_ui("TwitchClient"_hash); // needs its own line
                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Debug")) {
                    ImGui::BeginChild("DebugChild");

                    pmods->on_draw_ui("RoomRespawn"_hash);
                    pmods->on_draw_ui("LoadOrder"_hash);

                    ImGui::Spacing();
                    pmods->on_draw_ui("PlayerTracker"_hash); // needs its own line

                    ImGui::Spacing();

                    ImGui::Separator();
                    ImGui::Spacing();

                    pmods->on_draw_ui("EnemyStats"_hash); // needs its own line

                    ImGui::Spacing();

                    // pmods->onDrawUI("ShaderEditor"_hash);
                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Hotkeys")) {
                    ImGui::BeginChild("HotkeysChild");

                    ImGui::Spacing();
                    pmods->on_hotkey_tab(*g_framework->get_input_struct());
                    ImGui::Spacing();

                    g_window_height_hack = std::clamp(ImGui::GetCursorPosY() + 108.0f, 0.0f, g_max);
                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }
                credits_drawing();
                ImGui::EndTabBar();
            }
            ImGui::End();
        }
    }
}