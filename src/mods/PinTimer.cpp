#include "PinTimer.hpp"
#include "../sdk/Devil4.hpp"

#include "utility/Compressed.hpp"
#include "utility/TextureAtlas.hpp"
#include "utility/Dx9Utils.hpp"

#include "misc/PinTimerAtlas.cpp"

// hopefully mister compiler calculates those at compile time, we dont have consteval from c++20
// cba to switch to newer standards
namespace pui {
    static PDIRECT3DTEXTURE9 texture_handle = nullptr;
    static constexpr auto atlas_size = glm::vec2(1024.0f, 256.0f);
    static constexpr utility::Texture wing_header(glm::vec2(10.0f, 10.0f), glm::vec2(307.0f, 84.0f), atlas_size);
    static constexpr utility::Texture buttplug_bottom(glm::vec2(337.0f, 10.0f), glm::vec2(231.0f, 15.0f), atlas_size);
    static constexpr utility::Texture backdrop(glm::vec2(588.0f, 10.0f), glm::vec2(210.0f, 184.0f), atlas_size);
    static constexpr utility::Texture pin_timer_text(glm::vec2(337.0f, 45.0f), glm::vec2(120.0f, 26.0f), atlas_size);
    static constexpr utility::Texture red_highlight(glm::vec2(477.0f, 85.0f), glm::vec2(35.0f, 16.0f), atlas_size);
    static constexpr utility::Texture pin_text(glm::vec2(534.0f, 45.0f), glm::vec2(29.0f, 14.0f), atlas_size);
}

void PinTimer::on_reset() {
    spdlog::info("PinTimer::on_reset()");
    if (pui::texture_handle != nullptr) {
        pui::texture_handle->Release();
        spdlog::info("PinTimer::m_texture_handle->Release()");
        pui::texture_handle = nullptr;
    }
}
void PinTimer::after_reset() {
    auto [data, size] = utility::decompress_file_from_memory_with_size(pintimer_atlas_compressed_data, pintimer_atlas_compressed_size);
    int width, height;
    if (!utility::dx9::load_texture_from_file(data, size, &pui::texture_handle, &width, &height)) {
        spdlog::error("Failed to unpack and load compressed texture");
    }
    free(data);
}

static float G_WINDOW_HEIGHT_HACK_IDK{ 270.0f }; // fresh from my ass
static bool CON_PIN_UI_DEBUG{ false };
bool PinTimer::mod_enabled{ false };

std::optional<std::string> PinTimer::on_initialize() {
    auto [data, size] = utility::decompress_file_from_memory_with_size(pintimer_atlas_compressed_data, pintimer_atlas_compressed_size);
    int width, height;
    if (!utility::dx9::load_texture_from_file(data, size, &pui::texture_handle, &width, &height)) {
        spdlog::error("Failed to load pin timer texture");
        free(data);
        return "failed to load pin timer texture";
    }
    free(data);

    IM_ASSERT(width  == (int)pui::atlas_size.x);
    IM_ASSERT(height == (int)pui::atlas_size.y);
    IM_ASSERT(pui::texture_handle != nullptr);
#ifndef NDEBUG
    console->system().RegisterVariable("pin_ui_debug", CON_PIN_UI_DEBUG, csys::Arg<bool>("bool"));
#endif

    console->system().RegisterCommand("pintimer", "Enable pin timer HUD", []() {
        mod_enabled = !mod_enabled;
    });

	return Mod::on_initialize();
}

void PinTimer::custom_imgui_window() {
    if (mod_enabled || CON_PIN_UI_DEBUG ) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            if (player->controllerID == 0) { // 0 = dante, some were crashing with nero and this might:tm: fix it
                 if (player->luciferPins[0]) {
                    // this should be played every time someone 
                    // scrolls through the section below
                    // https://youtu.be/nmEX1rlt4pk

                    ImGuiIO& io = ImGui::GetIO();
                    ImGuiWindowFlags window_flags = 
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoBackground;

                    // bandaid
                    const float text_height = ImGui::GetTextLineHeightWithSpacing();
                    const glm::vec2 margin = glm::vec2(text_height * 1.666f, text_height * 5.6f);
                    glm::vec2 window_size = glm::vec2(pui::backdrop.size_.x + margin.x, G_WINDOW_HEIGHT_HACK_IDK/*pui::backdrop.size_.y + margin.y*/);
                    glm::vec2 window_pos  = glm::vec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);
                    const float fucktor = glm::smoothstep(window_pos.y, window_pos.y + window_size.y + 150.0f, G_WINDOW_HEIGHT_HACK_IDK);

                    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
                    ImGui::SetNextWindowSize(window_size);
                    ImGui::Begin("Lucifer Pin Timers", NULL, window_flags);
                    ImGui::NewLine();

                    //const glm::vec2 window_size = glm::vec2(pui::backdrop.size_.x + margin.x, pui::backdrop.size_.y + margin.y);
                    //window_size = (max_region - min_region) + glm::vec2(0.0f, ImGui::GetTextLineHeight() * 2.22f);
                    window_size = ImGui::GetWindowSize();
                    //const glm::vec2 window_pos  = glm::vec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);
                    glm::vec2 newpos = ImGui::GetWindowPos();
                    window_pos = newpos;
                    //window_pos = (newpos - min_region) - (ImGui::GetTextLineHeight() * 1.0f);

                    // cba to use window draw list since buttplugs would get cut off
                    auto* dl = ImGui::GetBackgroundDrawList();
#ifndef NDEBUG
                    if (CON_PIN_UI_DEBUG) {
                        // testing
                        int alpha = (int)(glm::smoothstep(0.0, 0.5, glm::sin(ImGui::GetTime()*10.0)) * 255.0);
                        dl->AddRectFilled(window_pos, window_pos + window_size, ImColor::ImColor(255,0,0,alpha));
                    }
#endif
                    // background
                    const glm::vec2 bgsize = glm::vec2(window_size.x * 1.1f, (window_size.y + (pui::buttplug_bottom.size_.y / 2.0f) + text_height * fucktor) );
                    const glm::vec2 bgpos =  glm::vec2(window_pos.x - text_height * 1.2f, window_pos.y);
                    dl->AddImage(pui::texture_handle, bgpos, bgpos + bgsize, pui::backdrop.uv0, pui::backdrop.uv1);

                    // header buttplug
                    const glm::vec2 header_pos = glm::vec2(window_pos.x - (window_size.x * 0.38f), window_pos.y - (pui::wing_header.size_.y / 2));
                    dl->AddImage(pui::texture_handle, header_pos, header_pos + (pui::wing_header.size_ * 1.11f), pui::wing_header.uv0, pui::wing_header.uv1);

                    // pin timer text
                    const glm::vec2 htxt_pos = glm::vec2(window_pos.x + (pui::pin_timer_text.size_.x / 2.0f), window_pos.y - pui::pin_timer_text.size_.y);
                    dl->AddImage(pui::texture_handle, htxt_pos, htxt_pos + (pui::pin_timer_text.size_), pui::pin_timer_text.uv0, pui::pin_timer_text.uv1);

                    // bottom buttplug
                    const glm::vec2 bottom_buttplug_pos = glm::vec2(window_pos.x, window_pos.y + window_size.y - (pui::buttplug_bottom.size_.y / 2.0f));
                    dl->AddImage(pui::texture_handle, bottom_buttplug_pos, bottom_buttplug_pos + (pui::buttplug_bottom.size_), pui::buttplug_bottom.uv0, pui::buttplug_bottom.uv1);

                    ImGui::BeginTable("pin_table", 2);
                    for (int i = 0; i < 15; i++) {
                        if (!player->luciferPins[i]) { continue; }
                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if (player->luciferPins[i]->penetrated) {
                            glm::vec2 pos = ImGui::GetCursorScreenPos();
                            const float window_content_region_width = (window_size.x);
                            dl->AddImage(pui::texture_handle, glm::vec2(pos.x,pos.y), glm::vec2(pos.x + (window_content_region_width * 0.8), pos.y + (ImGui::GetTextLineHeight())), pui::red_highlight.uv0, pui::red_highlight.uv1);
                        }
                        ImGui::Image(pui::texture_handle, ImVec2(pui::pin_text.size_.x, ImGui::GetTextLineHeight()), pui::pin_text.uv0, pui::pin_text.uv1); ImGui::SameLine();
                        ImGui::Text("%d", i + 1);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.0f / %.0f", player->luciferPins[i]->timer, player->luciferPins[i]->timerMax);
#if 0 // old menu
                        if (!player->luciferPins[i]) {
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
#endif
                    }
                    ImGui::EndTable();
                    ImGui::NewLine();
                    G_WINDOW_HEIGHT_HACK_IDK = ImGui::GetCursorPosY();
                    ImGui::End();
                }
            }
        }
    }
}

void PinTimer::on_gui_frame() {
    ImGui::Checkbox(_("Show Pin Timers"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Opens up a UI when pins are placed that shows the time remaining on each pin"));
}

void PinTimer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pin_imgui_enabled", mod_enabled);
}

void PinTimer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("pin_imgui_enabled").value_or(false);
}
