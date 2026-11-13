#include "FloorIsLava.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "EnemyTracker.hpp"
#include "imgui_internal.h"
#include "../sdk/Players.hpp"
#include "../sdk/uPlayer.hpp"
#include <cstdio>

bool FloorIsLava::mod_enabled = false;
static constexpr float kGraceSeconds = 5.0f;
static glm::ivec2 screen_res{ 0, 0 };

// Each player's grace (player_info's index): seconds since he appeared or got back up (co-op's revive), counted on his
// own clock; past kGraceSeconds the floor kills him while enemies are alive.
struct Grace {
    uintptr_t player = 0;
    bool alive_last_frame = false;
    bool counting = false;
    bool floor_allowed = true;
    float seconds = 0.0f;
};
static Grace graces[4];

static void kill_player(uPlayer_old* player) { // vtable+BC (DevilMayCry4_DX9.exe+3B7AD0) is die with damage IDs, vtable+1A4 (DevilMayCry4_DX9.exe+3AAB80) is die but it crashes if I do it while aerial???
    _asm {
        pushad
        mov esi, player
        test esi, esi
        je retcode
        mov edx, 0x7AAB80
        push 0 // idk
        call edx
        retcode:
        popad
    }
}

static int get_enemy_count(uEnemy_Old* enemy) {
    int enemies_alive = 0;
    while (enemy) {
        uDamage_Old* currentEnemyDamage = (uDamage_Old*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
        if (currentEnemyDamage->HP > 0.0f) {
            enemies_alive++;
        }
        enemy = enemy->nextEnemy;
    }
    return enemies_alive;
}

// the countdown at the top centre of the player's own view
static void draw_grace(int slot, float seconds) {
    float rect[4];
    if (!player_info::view_rect(slot, rect)) return;
    char name[32];
    std::snprintf(name, sizeof name, slot == 0 ? "FloorIsLavaTimer" : "FloorIsLavaTimer##%d", slot);
    ImGui::Begin(name, NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::UpdateCurrentFontSize(1.0f * ImGui::GetStyle().FontSizeBase);
    ImVec2 window_size = ImGui::GetWindowSize();
    const float vx = rect[0] * screen_res.x, vy = rect[1] * screen_res.y, vw = (rect[2] - rect[0]) * screen_res.x;
    ImVec2 window_pos(vx + (vw - window_size.x) * 0.5f, vy);
    ImGui::SetWindowPos(window_pos);
    static ImColor texCol = { 1.0f, 0.0f, 0.0f, 1.0f };

    const ImVec2 pos = ImGui::GetCursorPos();
    // shadow
    ImGui::SetCursorPos(ImVec2(pos.x + 1.0f, pos.y + 1.0f));
    ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.7f), "%.2f", seconds);
    // text
    ImGui::SetCursorPos(pos);
    ImGui::TextColored(texCol, "%.2f", seconds);
    ImGui::End();
}

void FloorIsLava::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) { return; }
    int enemyCount = get_enemy_count((uEnemy_Old*)devil4_sdk::get_moveline_top(15));
    sRender* sRen = devil4_sdk::get_sRender();
    if (sRen) screen_res = sRen->screenRes;

    const int n = player_info::count();
    for (int i = 0; i < 4; i++) {
        uPlayer_old* player = i < n ? (uPlayer_old*)player_info::player(i) : nullptr;
        Grace& g = graces[i];
        if (g.player != (uintptr_t)player) {
            g = Grace{};
            g.player = (uintptr_t)player;
        }
        const bool player_is_alive = player && player->damageStruct.HP > 0.0f;
        if (!player_is_alive) {
            g.floor_allowed = true;
            g.counting = false;
            g.alive_last_frame = false;
            continue;
        }
        if (!g.alive_last_frame) { // appeared, or back up: his grace starts
            g.counting = true;
            g.seconds = 0.0f;
        }
        g.alive_last_frame = true;

        if (g.counting) {
            g.seconds += player->m_delta_time / 60.0f;
            if (g.seconds >= kGraceSeconds) g.counting = false;
        }
        if (!g.counting) {
            if (enemyCount > 0) {
                g.floor_allowed = false;
            }
        } else {
            g.floor_allowed = true;
            draw_grace(i, g.seconds);
        }

        if (!g.floor_allowed && player->cancels2 & 2 /* player->collisionSettings->mLand == 1*/) {
            player->damageStruct.HP = 0.0f;
            kill_player(player);
        }
    }
}

void on_timer_trigger() {}

void FloorIsLava::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Floor Is Lava"), &mod_enabled)) {
            for (Grace& g : graces) g = Grace{};   // everyone's 5 seconds start again
        }
        ImGui::SameLine();
        help_marker(_("Upon spawning, you have 5 seconds. Once them 5 seconds are up, if you touch the floor you die. In co-op "
                      "every player has his own 5 seconds, also after getting back up"));
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FloorIsLava::on_initialize() {
    return Mod::on_initialize();
}

void FloorIsLava::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FloorIsLava").value_or(false);
}

void FloorIsLava::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FloorIsLava", mod_enabled);
}
