#include "FloorIsLava.hpp"
#include "..\sdk\Devil4.hpp"
#include "EnemyTracker.hpp"

bool FloorIsLava::mod_enabled = false;
static bool grounded_allowed = false;
static utility::Timer* timer{};
static glm::ivec2 screen_res{ 0, 0 };

static const uint8_t bytes1[] = { // fake ebx
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x55, 0x55, 0x55,
    0x00, 0x00, 0x80, 0xBF, 0x0B, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0xC5, 0x0C, 0xC1, 0xFA, 0x73, 0x31, 0x43,
    0x80, 0x53, 0x4A, 0xBF, 0x00, 0x00, 0x00, 0x00,
    0x3F, 0xC5, 0x0C, 0xC1, 0xFA, 0x73, 0x31, 0x43,
    0x80, 0x53, 0x4A, 0xBF, 0x00, 0x00, 0x00, 0x00,
    0xC0, 0x01, 0xCA, 0x1C, 0x80, 0x27, 0x82, 0xC0,
    0x00, 0x00, 0xC8, 0xC3, 0x00, 0x00, 0x80, 0x3F,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x84, 0xA9, 0x74, 0xC2, 0x7E, 0x72, 0x94, 0x42,
    0xC0, 0x6B, 0x8F, 0xC2, 0x00, 0x00, 0x00, 0x00,
    0xF9, 0xAC, 0xE6, 0x41, 0xD4, 0xE2, 0x45, 0x43,
    0xFF, 0x50, 0x92, 0x41, 0xFF, 0xFF, 0xFF, 0xFF,
    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x90, 0x52, 0xC5, 0x1C,
};

static const uint8_t bytes2[] = { // fake edi
    0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x22, 0x20, 0x20, 0x22,
    0x00, 0x00, 0x00, 0x00, 0x1A, 0x00, 0x00, 0x00,
    0x00, 0xD0, 0x36, 0x45, 0x00, 0x30, 0x2C, 0x45,
    0x01, 0x23, 0x23, 0xC1, 0x14, 0x8D, 0x30, 0x43,
    0xC8, 0x0B, 0xAE, 0xC0, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x23, 0x23, 0xC1, 0x14, 0x8D, 0x30, 0x43,
    0xC8, 0x0B, 0xAE, 0xC0, 0x00, 0x00, 0x80, 0x3F,
    0x00, 0x90, 0xCA, 0x1C, 0x00, 0x00, 0x80, 0x3F,
    0x00, 0x00, 0x80, 0x3F, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x90, 0xCA, 0x1C,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0xFE, 0x7E, 0x46, 0xC2, 0xE6, 0x4B, 0xE3, 0x42,
    0x68, 0x3C, 0x79, 0xC1, 0x00, 0x00, 0x80, 0x3F,
    0x81, 0x51, 0x0D, 0x43, 0xD2, 0x11, 0x83, 0x43,
    0x62, 0x4E, 0xFC, 0x42, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x80, 0x3F, 0x02, 0x00, 0x00, 0x00,
    0xD0, 0xC1, 0xD2, 0x1C,
};

static const uint8_t* attack1 = bytes1;
static const uint8_t* attack2 = bytes2;

static constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
naked void KillPlayer() { // vtable+BC (DevilMayCry4_DX9.exe+3B7AD0) is die with damage IDs, vtable+1A4 (DevilMayCry4_DX9.exe+3AAB80) is die but it crashes if I do it while aerial???
    _asm {
        pushad
        // get player
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        test ecx, ecx
        je retcode

        // copy here in case it matters
        mov ebp, ecx
        mov esi, ecx

        // lie about incoming attack
        mov ebx, [attack1]
        mov edi, [attack2]
        push ebx
        push edi

        //push ecx // only do this if trying vtable+1A4
        mov eax, [ecx]
        mov eax, [eax+0xBC]
        call eax
        retcode:
        popad
        ret
    }
}

static int get_enemy_count(uEnemy* enemy) {
    int enemies_alive = 0;
    while (enemy) {
        uDamage* currentEnemyDamage = (uDamage*)((char*)enemy + EnemyTracker::get_enemy_specific_damage_offset(enemy->ID));
        if (currentEnemyDamage->HP > 0.0f) {
            enemies_alive++;
        }
        enemy = enemy->nextEnemy;
    }
    return enemies_alive;
}

void FloorIsLava::on_frame(fmilliseconds& dt) {
    if (!mod_enabled) { return; }
    SMediator* sMed = devil4_sdk::get_sMediator();
    sArea* s_area_ptr = devil4_sdk::get_sArea();
    uPlayer* player = devil4_sdk::get_local_player();
    int enemyCount = get_enemy_count(devil4_sdk::get_uEnemies());
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
            ImGui::SetWindowFontScale(3.0f);
            screen_res = sRen->screenRes;
            ImVec2 window_size = ImGui::GetWindowSize();
            ImVec2 window_pos((screen_res.x - window_size.x) * 0.5f, 0.0f);
            ImGui::SetWindowPos(window_pos);
            static ImColor texCol = { 1.0f, 0.0f, 0.0f, 1.0f };
            ImGui::TextColored(texCol, "%.2f", timer->m_time.count());
            ImGui::End();
        }

        if (!grounded_allowed && player->collisionSettings->mLand == 1) {
            player->damageStruct.HP = 0.0f;
            KillPlayer();
        }
    }

    player_existed_last_frame = player_exists_now && player_is_alive;
}

void on_timer_trigger() {}

void FloorIsLava::on_gui_frame(int display) {
    ImGui::Checkbox(_("Floor Is Lava"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Upon spawning, you have 5 seconds. Once them 5 seconds are up, if you touch the floor you die"));
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
