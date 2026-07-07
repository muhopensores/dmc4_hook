#include "AnimationOverlay.hpp"

#include "sdk/Devil4.hpp"
#include "sdk/uPlayer.hpp"

#include <cmath>
#include <string>

bool AnimationOverlay::mod_enabled = false;
bool AnimationOverlay::show_advanced = false;
ImVec2 AnimationOverlay::window_pos{ 20.0f, 120.0f };

namespace {
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
    }
    __except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return false;
    }

    return std::isfinite(state.anim_frame);
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
}

void AnimationOverlay::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) {
        return;
    }

    AnimationOverlayState state{};
    if (!read_animation_overlay_state(devil4_sdk::get_local_player(), state)) {
        return;
    }

    constexpr ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_AlwaysAutoResize |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoSavedSettings;

    ImGui::SetNextWindowBgAlpha(0.58f);
    ImGui::Begin("Animation Overlay", nullptr, window_flags);
    ImGui::SetWindowPos(window_pos, ImGuiCond_Once);
    window_pos = ImGui::GetWindowPos();

    ImGui::Text(_("Anim: 0x%04X"), state.anim_id);
    ImGui::Text(_("MoveID2: 0x%08X"), state.move_id2);
    ImGui::Text(_("Part: %u"), state.move_part);
    if (has_valid_frame_max(state.anim_frame_max)) {
        ImGui::Text(_("Frame: %.1f / %.1f"), state.anim_frame, state.anim_frame_max);
    }
    else {
        ImGui::Text(_("Frame: %.1f"), state.anim_frame);
    }
    const auto state_line = format_state_line(state);
    ImGui::Text(_("State: %s"), state_line.c_str());
    
    if (show_advanced) {
        ImGui::Separator();
        ImGui::Text(_("Bank: 0x%X"), state.move_bank);
        ImGui::Text(_("Ground Raw: %u"), state.grounded_raw);
        ImGui::Text(_("Ground2: %u"), state.grounded2 ? 1 : 0);
        ImGui::Text(_("Land Flag: %s"), state.has_collision_land ? (state.collision_land ? "1" : "0") : _("n/a"));
        ImGui::Text(_("Hitstop Timer: %.1f"), state.hitstop_timer);
    }

    ImGui::End();
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
    window_pos.x = cfg.get<float>("animation_overlay_pos_x").value_or(20.0f);
    window_pos.y = cfg.get<float>("animation_overlay_pos_y").value_or(120.0f);
}

void AnimationOverlay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("animation_overlay", mod_enabled);
    cfg.set<bool>("animation_overlay_advanced", show_advanced);
    cfg.set<float>("animation_overlay_pos_x", window_pos.x);
    cfg.set<float>("animation_overlay_pos_y", window_pos.y);
}
