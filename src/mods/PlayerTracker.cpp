#include "PlayerTracker.hpp"
#include "WorkRate.hpp"

#include "utility/Compressed.hpp"
#include "utility/TextureAtlas.hpp"
#include "utility/Dx9Utils.hpp"

#include "misc/PinTimerAtlas.cpp"

static float G_WINDOW_HEIGHT_HACK_IDK{ 270.0f }; // fresh from my ass

#ifndef NDEBUG
static bool CON_PIN_UI_DEBUG{ false };
#endif

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

    return Mod::on_initialize();
}

void PlayerTracker::on_gui_frame() {
    ImGui::Checkbox(_("Disable Game Pause When Opening The Trainer"), &WorkRate::disable_trainer_pause);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Checkbox(_("Show Pin Timers"), &PlayerTracker::pin_imgui_enabled);
    ImGui::SameLine();
    help_marker(_("Opens up a UI when pins are placed that shows the time remaining on each pin"));

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

    ImGui::Checkbox(_("Display Player Stats"), &display_player_stats);
    ImGui::SameLine();
    help_marker(_("View various details about the player character"));
    if (display_player_stats) {
        ImGui::Indent(lineIndent);
        uPlayer* player = devil4_sdk::get_local_player();
        if (player) {
            ImGui::InputFloat(_("HP ##1"), &player->HP);
            ImGui::InputFloat(_("Max HP ##1"), &player->HPMax);
            ImGui::InputScalar(_("Move ID Debug ##1"), ImGuiDataType_U8, &player->moveIDBest);
            ImGui::InputFloat3(_("XYZ Position ##1"), (float*)&player->m_pos);
            ImGui::InputFloat4(_("Rotation ##1"), &player->rotation2);
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
            ImGui::InputScalar(_("Move Part ##1"), ImGuiDataType_U8, &player->movePart);
            ImGui::InputScalar(_("Move Bank ##1"), ImGuiDataType_U8, &player->moveBank);
            ImGui::InputScalar(_("Move ID ##1"), ImGuiDataType_U8, &player->moveID2);
            static int8_t tempMoveBank = 0;
            static int8_t tempMoveID   = 0;
            if (ImGui::Button(_("Save Current Move"))) {
                tempMoveBank = player->moveBank;
                tempMoveID   = player->moveID2;
            }
            ImGui::InputScalar(_("Saved Move Bank ##1"), ImGuiDataType_U8, &tempMoveBank);
            ImGui::InputScalar(_("Saved Move ID ##1"), ImGuiDataType_U8, &tempMoveID);
            if (ImGui::Button(_("Play Saved Move"))) {
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
    if (pin_imgui_enabled || CON_PIN_UI_DEBUG ) {
        SMediator* s_med_ptr  = *(SMediator**)static_mediator_ptr;
        uintptr_t* player_ptr = (uintptr_t*)((uintptr_t)s_med_ptr + 0x24);
        uintptr_t player_base = *player_ptr;
        if (player_base) {
            int& controller_id = *(int*)(player_base + 0x1494);
            if (controller_id == 0) { // 0 = dante, some were crashing with nero and this might:tm: fix it
                uintptr_t* pin_ptr = (uintptr_t*)((uintptr_t)player_base + 0x14DBC); // 0x14DBC
                uintptr_t pin_base = *pin_ptr;
                 if (pin_base) {
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
                        pin_ptr = (uintptr_t*)((uintptr_t)player_base + 0x14DBC + i * 4); // 0x14DBC
                        pin_base = *pin_ptr;
                        if(!pin_base) { continue; }

                        float& pin_timer      = *(float*)(pin_base + 0x1790);
                        float& pin_timer_max  = *(float*)(pin_base + 0x1794);
                        bool&  pin_penetrated = *(bool*) (pin_base + 0x17B4);

                        ImGui::TableNextRow();
                        ImGui::TableNextColumn();
                        if (pin_penetrated) {
                            glm::vec2 pos = ImGui::GetCursorScreenPos();
                            const float window_content_region_width = (window_size.x);
                            dl->AddImage(pui::texture_handle, glm::vec2(pos.x,pos.y), glm::vec2(pos.x + (window_content_region_width * 0.8), pos.y + (ImGui::GetTextLineHeight())), pui::red_highlight.uv0, pui::red_highlight.uv1);
                        }
                        ImGui::Image(pui::texture_handle, ImVec2(pui::pin_text.size_.x, ImGui::GetTextLineHeight()), pui::pin_text.uv0, pui::pin_text.uv1); ImGui::SameLine();
                        ImGui::Text("%d", i + 1);
                        ImGui::TableNextColumn();
                        ImGui::Text("%.0f / %.0f", pin_timer, pin_timer_max);
#if 0 // old menu
                        uintptr_t* pin_ptr = (uintptr_t*)((uintptr_t)player_base + 0x14DBC + i * 4); // 0x14DBC
                        uintptr_t pin_base = *pin_ptr;
                        if (pin_base) {
                            float& pin_timer     = *(float*)(pin_base + 0x1790);
                            float& pin_timer_max = *(float*)(pin_base + 0x1794);
                            bool& pin_penetrated = *(bool*)(pin_base + 0x17B4);
                            if (pin_penetrated) {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 0, 0, 255));
                            } else {
                                ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
                            }
                            ImGui::Text("Pin %i = %.0f", i + 1, pin_timer);
                            ImGui::SameLine();
                            ImGui::Text("/ %.0f", pin_timer_max);
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


void PlayerTracker::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("pin_imgui_enabled", pin_imgui_enabled);
}

void PlayerTracker::on_config_load(const utility::Config& cfg) {
    pin_imgui_enabled = cfg.get<bool>("pin_imgui_enabled").value_or(false);
}
#endif
