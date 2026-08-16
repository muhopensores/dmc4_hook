#include "AnimationOverlay.hpp"

#include "sdk/Devil4.hpp"
#include "sdk/uPlayer.hpp"
#include "imgui_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>

bool AnimationOverlay::mod_enabled = false;
bool AnimationOverlay::show_advanced = false;
ImVec2 AnimationOverlay::window_pos{ 20.0f, 230.0f };

namespace {
constexpr float DEFAULT_WINDOW_POS_X = 20.0f;
constexpr float OLD_DEFAULT_WINDOW_POS_Y = 120.0f;
constexpr float DEFAULT_WINDOW_POS_Y = 230.0f;

struct AnimationOverlayState {
    uint16_t anim_id = 0;
    float anim_frame = 0.0f;
    float anim_frame_max = 0.0f;
    uint32_t move_bank = 0;
    uint32_t move_part = 0;
    uint32_t move_id2 = 0;
    bool grounded = false;
    uint8_t grounded_raw = 0;
    bool grounded2 = false;
    bool collision_land = false;
    bool has_collision_land = false;
    bool hitstop = false;
    float hitstop_timer = 0.0f;
    bool locked_on = false;
    uint32_t character_role = 0xFFFFFFFFu;
};

struct AnimationOverlayPalette {
    ImVec4 title{};
    ImVec4 accent{};
    ImVec4 highlight{};
    ImVec4 body{};
    ImVec4 muted{};
    ImVec4 good{};
    ImVec4 warning{};
};

bool has_valid_frame_max(float value) {
    return std::isfinite(value) && value > 0.0f && value < 100000.0f;
}

bool read_player_grounded(uPlayer* player) {
    if (player->grounded == 2) {
        return false;
    }
    if (player->grounded == 1) {
        return true;
    }

    if (player->grounded2) {
        return true;
    }

    return player->collisionSettings && player->collisionSettings->mLand != 0;
}

bool read_animation_overlay_state(uPlayer* player, AnimationOverlayState& state) {
    if (!player) {
        return false;
    }

    __try {
        state.anim_id = player->animID;
        state.anim_frame = player->animFrame;
        state.anim_frame_max = player->animFrameMax;
        state.move_bank = player->moveBank;
        state.move_part = player->movePart;
        state.move_id2 = player->moveID2;
        state.grounded = read_player_grounded(player);
        state.grounded_raw = player->grounded;
        state.grounded2 = player->grounded2;
        state.has_collision_land = player->collisionSettings != nullptr;
        state.collision_land = state.has_collision_land && player->collisionSettings->mLand != 0;
        state.hitstop = player->hitstop;
        state.hitstop_timer = player->hitstopTimer;
        state.locked_on = player->lockedOn;
        state.character_role = player->controllerID == 1 ? 0 : (player->controllerID == 0 ? 1 : 0xFFFFFFFFu);
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }

    return std::isfinite(state.anim_frame);
}

uPlayer* get_animation_overlay_player_safe() {
    __try {
        return devil4_sdk::get_local_player();
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return nullptr;
    }
}

std::string format_state_line(const AnimationOverlayState& state) {
    std::string result = state.grounded ? _("grounded") : _("airborne");
    if (state.hitstop) {
        result += _(", hitstop");
    }
    if (state.locked_on) {
        result += _(", locked-on");
    }
    return result;
}

AnimationOverlayPalette overlay_palette_for_character(uint32_t character_role) {
    if (character_role == 1) {
        return {
            ImVec4(1.0f, 0.30f, 0.22f, 0.98f),
            ImVec4(1.0f, 0.68f, 0.40f, 0.98f),
            ImVec4(1.0f, 0.92f, 0.58f, 0.98f),
            ImVec4(1.0f, 0.96f, 0.92f, 0.96f),
            ImVec4(1.0f, 0.72f, 0.62f, 0.88f),
            ImVec4(0.66f, 1.0f, 0.72f, 0.94f),
            ImVec4(1.0f, 0.86f, 0.34f, 0.96f),
        };
    }

    if (character_role == 0) {
        return {
            ImVec4(0.24f, 0.72f, 1.0f, 0.98f),
            ImVec4(0.68f, 0.94f, 1.0f, 0.98f),
            ImVec4(0.95f, 1.0f, 1.0f, 0.98f),
            ImVec4(0.93f, 0.98f, 1.0f, 0.96f),
            ImVec4(0.66f, 0.84f, 1.0f, 0.88f),
            ImVec4(0.66f, 1.0f, 0.72f, 0.94f),
            ImVec4(1.0f, 0.86f, 0.34f, 0.96f),
        };
    }

    return {
        ImVec4(0.85f, 0.92f, 1.0f, 0.98f),
        ImVec4(0.65f, 0.78f, 1.0f, 0.96f),
        ImVec4(1.0f, 1.0f, 1.0f, 0.98f),
        ImVec4(0.96f, 0.98f, 1.0f, 0.94f),
        ImVec4(0.78f, 0.84f, 0.94f, 0.88f),
        ImVec4(0.66f, 1.0f, 0.72f, 0.94f),
        ImVec4(1.0f, 0.86f, 0.34f, 0.96f),
    };
}

void draw_shadow_text(const std::string& text, const ImVec4& color) {
    const ImVec2 pos = ImGui::GetCursorPos();
    ImGui::SetCursorPos(ImVec2(pos.x + 3.0f, pos.y + 3.0f));
    ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, color.w * 0.75f), "%s", text.c_str());
    ImGui::SetCursorPos(pos);
    ImGui::TextColored(color, "%s", text.c_str());
}

float key_value_column(std::initializer_list<const char*> labels) {
    float label_width = 0.0f;
    for (const char* label : labels) {
        label_width = std::max(label_width, ImGui::CalcTextSize(label).x);
    }
    return ImGui::GetCursorPosX() + label_width + ImGui::GetStyle().ItemSpacing.x;
}

void draw_key_value_line(
    const char* label,
    const std::string& value,
    float value_column,
    const ImVec4& label_color,
    const ImVec4& value_color) {
    ImGui::TextColored(label_color, "%s", label);
    ImGui::SameLine(value_column);
    draw_shadow_text(value, value_color);
}

std::string format_hex16(uint16_t value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "0x%04X", value);
    return buffer;
}

std::string format_hex32(uint32_t value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "0x%08X", value);
    return buffer;
}

std::string format_frame_line(const AnimationOverlayState& state) {
    char buffer[64]{};
    if (has_valid_frame_max(state.anim_frame_max)) {
        std::snprintf(buffer, sizeof(buffer), "%.1f / %.1f", state.anim_frame, state.anim_frame_max);
    }
    else {
        std::snprintf(buffer, sizeof(buffer), "%.1f", state.anim_frame);
    }
    return buffer;
}

std::string format_part_line(const AnimationOverlayState& state) {
    char buffer[64]{};
    std::snprintf(buffer, sizeof(buffer), "%u", state.move_part);
    return buffer;
}

void draw_frame_progress_bar(const AnimationOverlayState& state, const AnimationOverlayPalette& palette) {
    if (!has_valid_frame_max(state.anim_frame_max)) {
        return;
    }

    const float progress = std::clamp(state.anim_frame / state.anim_frame_max, 0.0f, 1.0f);
    const ImVec2 pos = ImGui::GetCursorScreenPos();
    const float width = std::max(1.0f, ImGui::GetContentRegionAvail().x);
    const ImVec2 size(width, 12.0f);
    auto* draw_list = ImGui::GetWindowDrawList();

    const ImVec2 end(pos.x + size.x, pos.y + size.y);
    const ImVec2 fill_end(pos.x + size.x * progress, pos.y + size.y);

    draw_list->AddRectFilled(pos, end, ImColor(0, 0, 0, 155), 5.0f);
    draw_list->AddRect(pos, end, ImColor(255, 255, 255, 58), 5.0f);
    if (progress > 0.0f) {
        draw_list->AddRectFilled(pos, fill_end, ImColor(palette.title), 5.0f);
        draw_list->AddRectFilled(
            ImVec2(pos.x, pos.y + 1.0f),
            ImVec2(fill_end.x, pos.y + 4.0f),
            ImColor(palette.highlight),
            5.0f);
    }

    ImGui::Dummy(size);
}
}

void AnimationOverlay::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) {
        return;
    }

    AnimationOverlayState state{};
    if (!read_animation_overlay_state(get_animation_overlay_player_safe(), state)) {
        return;
    }

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    const auto palette = overlay_palette_for_character(state.character_role);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16.0f, 12.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.0f, 6.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.025f, 0.035f, 0.52f));
    ImGui::Begin("Animation Overlay", nullptr, window_flags);
    ImGui::SetWindowPos(window_pos, ImGuiCond_Once);
    window_pos = ImGui::GetWindowPos();

    const auto state_line = format_state_line(state);
    const auto state_color = state.hitstop ? palette.warning : (state.grounded ? palette.good : palette.accent);

    ImGui::UpdateCurrentFontSize(1.45f * ImGui::GetStyle().FontSizeBase);
    draw_shadow_text(_("Animation"), palette.title);
    ImGui::SameLine();
    draw_shadow_text(state_line, state_color);

    ImGui::UpdateCurrentFontSize(1.90f * ImGui::GetStyle().FontSizeBase);
    draw_shadow_text(std::string(_("Frame  ")) + format_frame_line(state), palette.highlight);
    draw_frame_progress_bar(state, palette);

    ImGui::UpdateCurrentFontSize(1.36f * ImGui::GetStyle().FontSizeBase);
    const float primary_value_column = key_value_column({ _("MoveID2"), _("Anim"), _("Move Part") });
    draw_key_value_line(_("MoveID2"), format_hex32(state.move_id2), primary_value_column, palette.muted, palette.body);
    draw_key_value_line(_("Anim"), format_hex16(state.anim_id), primary_value_column, palette.muted, palette.body);
    draw_key_value_line(_("Move Part"), format_part_line(state), primary_value_column, palette.muted, palette.body);

    if (show_advanced) {
        ImGui::Separator();
        ImGui::UpdateCurrentFontSize(1.18f * ImGui::GetStyle().FontSizeBase);
        const float advanced_value_column =
            key_value_column({ _("Bank"), _("Ground Raw"), _("Ground2"), _("Land Flag"), _("Hitstop") });
        draw_key_value_line(_("Bank"), format_hex32(state.move_bank), advanced_value_column, palette.muted, palette.body);
        draw_key_value_line(_("Ground Raw"), std::to_string(state.grounded_raw), advanced_value_column, palette.muted, palette.body);
        draw_key_value_line(_("Ground2"), state.grounded2 ? "1" : "0", advanced_value_column, palette.muted, palette.body);
        draw_key_value_line(
            _("Land Flag"),
            state.has_collision_land ? (state.collision_land ? "1" : "0") : _("n/a"),
            advanced_value_column,
            palette.muted,
            palette.body);
        char hitstop_buffer[32]{};
        std::snprintf(hitstop_buffer, sizeof(hitstop_buffer), "%.1f", state.hitstop_timer);
        draw_key_value_line(
            _("Hitstop"),
            hitstop_buffer,
            advanced_value_column,
            palette.muted,
            state.hitstop ? palette.warning : palette.body);
    }

    ImGui::UpdateCurrentFontSize(0.0f);
    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

void AnimationOverlay::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) {
        return;
    }

    ImGui::BeginGroup();
    ImGui::Checkbox(_("Animation Overlay"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Shows the local player's read-only animation state directly on the game screen. Frame max is the runtime max frame value, not a verified resource-file total frame count."));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Show Advanced Animation Fields"), &show_advanced);
        ImGui::SameLine();
        help_marker(_("Adds lower-level values such as animation bank, raw ground flags, and hitstop timer."));
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
}

void AnimationOverlay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("animation_overlay").value_or(false);
    show_advanced = cfg.get<bool>("animation_overlay_advanced").value_or(false);
    window_pos.x = cfg.get<float>("animation_overlay_pos_x").value_or(DEFAULT_WINDOW_POS_X);
    window_pos.y = cfg.get<float>("animation_overlay_pos_y").value_or(DEFAULT_WINDOW_POS_Y);

    if (std::abs(window_pos.x - DEFAULT_WINDOW_POS_X) < 2.0f &&
        std::abs(window_pos.y - OLD_DEFAULT_WINDOW_POS_Y) < 2.0f) {
        window_pos.y = DEFAULT_WINDOW_POS_Y;
    }
}

void AnimationOverlay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("animation_overlay", mod_enabled);
    cfg.set<bool>("animation_overlay_advanced", show_advanced);
    cfg.set<float>("animation_overlay_pos_x", window_pos.x);
    cfg.set<float>("animation_overlay_pos_y", window_pos.y);
}
