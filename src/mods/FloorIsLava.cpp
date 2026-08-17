#include "FloorIsLava.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "EnemyTracker.hpp"
#include "imgui_internal.h"

bool FloorIsLava::mod_enabled = false;
static bool grounded_allowed = false;
static utility::Timer* timer{};
static glm::ivec2 screen_res{ 0, 0 };

naked void KillPlayer() { // vtable+BC (DevilMayCry4_DX9.exe+3B7AD0) is die with damage IDs, vtable+1A4 (DevilMayCry4_DX9.exe+3AAB80) is die but it crashes if I do it while aerial???
    _asm {
        pushad
        // get player
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        test ecx, ecx
        je retcode
        mov esi, ecx
        mov edx, 0x7AAB80
        push 0 // idk
        call edx
        retcode:
        popad
        ret
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

void FloorIsLava::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) { return; }
    sMediator* sMed = devil4_sdk::get_sMediator();
    sArea* s_area_ptr = devil4_sdk::get_sArea();
    uPlayer* player = devil4_sdk::get_local_player();
    int enemyCount = get_enemy_count((uEnemy_Old*)devil4_sdk::get_moveline_top(15));
    sRender* sRen = devil4_sdk::get_sRender();

    bool player_exists_now = (player != nullptr);
    bool player_is_alive = player_exists_now && player->damageStruct.HP > 0.0f;
    static bool player_existed_last_frame = false;

    if ((player_exists_now && !player_existed_last_frame) || (player_existed_last_frame && (!player_exists_now || !player_is_alive))) {
        grounded_allowed = true;
    }
    
    if (timer && player_exists_now && player_is_alive) {
        float dante_seconds = player->m_delta_time / 60.0f;
        timer->tick((fmilliseconds)dante_seconds * 1000.0f);

        if (!player_existed_last_frame) {
            timer->start();
        }

        if (!timer->m_active) {
            if (enemyCount > 0) {
                grounded_allowed = false;
            }
        } else {
            grounded_allowed = true;
            ImGui::Begin("FloorIsLavaTimer", NULL, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);
            ImGui::UpdateCurrentFontSize(1.0f * ImGui::GetStyle().FontSizeBase);
            screen_res = sRen->screenRes;
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_pos((screen_res.x - window_size.x) * 0.5f, 0.0f);
            ImGui::SetWindowPos(window_pos);
            static ImColor texCol = { 1.0f, 0.0f, 0.0f, 1.0f };

            const ImVec2 pos = ImGui::GetCursorPos();
            // shadow
            ImGui::SetCursorPos(ImVec2(pos.x + 1.0f, pos.y + 1.0f));
            ImGui::TextColored(ImVec4(0.0f, 0.0f, 0.0f, 0.7f), "%.2f", timer->m_time.count());
            // text
            ImGui::SetCursorPos(pos);
            ImGui::TextColored(texCol, "%.2f", timer->m_time.count());
            ImGui::End();
        }

        if (!grounded_allowed && player->cancels2 & 2 /* player->collisionSettings->mLand == 1*/) {
            player->damageStruct.HP = 0.0f;
            KillPlayer();
        }
    }

    player_existed_last_frame = player_exists_now && player_is_alive;
}

void on_timer_trigger() {}

void FloorIsLava::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        if (ImGui::Checkbox(_("Floor Is Lava"), &mod_enabled)) {
            if (timer)
                timer->start();
        }
        ImGui::SameLine();
        help_marker(_("Upon spawning, you have 5 seconds. Once them 5 seconds are up, if you touch the floor you die"));
    }
}

// void on_game_pause(bool toggle) {}
// bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> FloorIsLava::on_initialize() {
    timer = new utility::Timer(5.0f, on_timer_trigger);
    return Mod::on_initialize();
}

void FloorIsLava::on_config_load(const utility::Config& cfg){
    mod_enabled = cfg.get<bool>("FloorIsLava").value_or(false);
}

void FloorIsLava::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("FloorIsLava", mod_enabled);
}
