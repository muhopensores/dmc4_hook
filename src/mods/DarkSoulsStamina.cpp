#include "DarkSoulsStamina.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/uPlayer.hpp"
uintptr_t DarkSoulsStamina::jmp_ret1 = NULL;
static glm::ivec2 screen_res{0, 0};

float DarkSoulsStamina::stamina_regen_rate = 500.0f;
float DarkSoulsStamina::stamina_max = 1000.0f;
float DarkSoulsStamina::stamina_delay_max = 1.0f;

bool DarkSoulsStamina::stamina_enabled = false;
float DarkSoulsStamina::stamina_delay_current = 0.0f;
float DarkSoulsStamina::stamina = stamina_max;

float DarkSoulsStamina::stamina_spent_visual = DarkSoulsStamina::stamina_max;
float DarkSoulsStamina::stamina_ghost_delay_current = 0.0f;
float DarkSoulsStamina::stamina_ghost_delay_max = 0.5f;
float DarkSoulsStamina::stamina_ghost_rate = 500.0f;

static int currentMoveID = 0; // for imgui debug
static float xmm0backup  = 0.0f;

// atm I kill inputs because I don't know how to stop the player from being able to do actions other than movement
naked void detour1() { // gamepad inputs (keyboard is handled by detour2 in InputStates.cpp)
    _asm {
            cmp byte ptr [DarkSoulsStamina::stamina_enabled], 1
            jne originalcode

            movss [xmm0backup], xmm0
            xorps xmm0, xmm0
            comiss xmm0, [DarkSoulsStamina::stamina]
            movss xmm0, [xmm0backup]
            ja retcode

        originalcode:
            mov [esi+0x00001410], eax
        retcode:
            jmp dword ptr [DarkSoulsStamina::jmp_ret1]
    }
}

float DarkSoulsStamina::get_move_cost(int moveID) {
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) return 0.0f;
    if ((moveID >= 0x000 && moveID <= 0x003) || // movement
        (moveID == 0x004 && !player->collisionSettings->mLand) || // falling anim
        (moveID >= 0x96 && moveID <= 0xAF)  ||  // taunts
        moveID == 0x006 || // stop from run
        moveID == 0x00D || // stop from sprint
        moveID == 0x200 || // reb idle
        moveID == 0x300 || // gilg idle
        moveID == 0x400 || // luci idle
        moveID == 0x500) {
        return 0.0f;
    }
    if (moveID > 0x3 && moveID < 0x200)
        return 100.0f; // movement
    if (moveID > 0x200 && moveID < 0x300)
        return 50.0f; // reb
    if (moveID > 0x300 && moveID < 0x400)
        return 100.0f; // gilg
    if (moveID > 0x400 && moveID < 0x500)
        return 75.0f; // luci
    if ((moveID > 0x528 && moveID < 0x600) || moveID == 0x51E) // air e+i
        return 10.0f; // e+i
    if (moveID > 0x61E && moveID < 0x700)
        return 25.0f; // coyote
    if (moveID > 0x715 && moveID < 0x800)
        return 50.0f; // pandora
    if (moveID > 0x900 && moveID < 0xA00)
        return 100.0f; // yamato
    return 0.0f;
}

void DarkSoulsStamina::stamina_regen(float seconds) {
    if (stamina_delay_current < stamina_delay_max) {
        stamina_delay_current += seconds;
    } else {
        if (stamina < stamina_max) {
            stamina += stamina_regen_rate * seconds;
            if (stamina > stamina_max)
                stamina = stamina_max;
        }
    }
}

void DarkSoulsStamina::check_for_stamina_use(uPlayer* player) {
    if (player->movePart == 1) {
    int moveID    = player->moveIDBest;
    float moveFrame = player->animFrame;
    static int prevMoveID    = -1;
    static float prevAnimFrame = -1;
        bool moveReset = (moveID != prevMoveID) || (moveID == prevMoveID && moveFrame < prevAnimFrame);
        if (moveReset) {
            float cost = get_move_cost(moveID);
            if (cost > 0.0f) {
                stamina -= cost;
                stamina_ghost_delay_current = 0.0f;
                stamina_delay_current       = 0.0f;
            }
        }
    prevMoveID    = moveID;
    prevAnimFrame = moveFrame;
    }
}

void DarkSoulsStamina::on_frame(fmilliseconds& dt) {
    if (!stamina_enabled) return;
    uPlayer* player = devil4_sdk::get_local_player();
    if (!player) return;
    sRender* sRen = devil4_sdk::get_sRender();
    if (!sRen) return;
    screen_res = sRen->screenRes;
    ImGui::Begin("StaminaGUI", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);

    // pos
    float bar_width  = screen_res.x * 0.250f;
    float bar_height = screen_res.x * 0.0075f;
    ImVec2 window_size = ImGui::GetWindowSize();
    ImVec2 window_pos(screen_res.x * 0.16f, screen_res.y * 0.2375f);
    ImGui::SetWindowPos(window_pos);
    float wideLeftX = (window_size.x - bar_width) * 0.75f;
    ImGui::SetCursorPosX(wideLeftX);

    // time
    float seconds = dt.count() / 1000.0f;
    if (!devil4_sdk::is_paused()) {
        stamina_regen(seconds);
        // ghost delay
        if (stamina_ghost_delay_current < stamina_ghost_delay_max) {
            stamina_ghost_delay_current += seconds;
        } else {
            // ghost drain
            float diff = stamina_spent_visual - stamina;
            if (diff > 0.0f) {
                float drain = stamina_ghost_rate * seconds;
                stamina_spent_visual -= std::min(drain, diff);
            } else {
                // snap when stamina refills past ghost
                stamina_spent_visual = stamina;
            }
        }
    }

    // bar
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos       = ImGui::GetCursorScreenPos();
    float stamina_frac       = stamina / stamina_max;
    stamina_frac             = std::clamp(stamina_frac, 0.0f, 1.0f);
    float stamina_spent_frac = stamina_spent_visual / stamina_max;
    stamina_spent_frac       = std::clamp(stamina_spent_frac, 0.0f, 1.0f);

    ImU32 col_bg      = IM_COL32(15, 15, 15, 220);
    ImU32 col_border  = IM_COL32(0, 0, 0, 255);
    ImU32 col_spent   = IM_COL32(255, 180, 60, 220);
    ImU32 col_current = IM_COL32(0, 200, 0, 255);

    draw->AddRectFilled(pos, ImVec2(pos.x + bar_width, pos.y + bar_height), col_bg, 2.0f); // bg
    draw->AddRectFilled(pos, ImVec2(pos.x + bar_width * stamina_spent_frac, pos.y + bar_height), col_spent, 2.0f); // spent stamina
    draw->AddRectFilled(pos, ImVec2(pos.x + bar_width * stamina_frac, pos.y + bar_height), col_current, 2.0f); // stamina
    draw->AddRect(pos, ImVec2(pos.x + bar_width, pos.y + bar_height), col_border, 2.0f); // border
    ImGui::Dummy(ImVec2(bar_width, bar_height));
    // ImGui::InputScalar("MoveID", ImGuiDataType_S32, &currentMoveID, NULL, NULL, "%8X"); // debug

    check_for_stamina_use(player);

    ImGui::End();
}

void DarkSoulsStamina::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Stamina"), &stamina_enabled);
        ImGui::SameLine();
        help_marker(_("I played a lot of Dark Souls recently and thought this would be funny"));
    }
}

// void DarkSoulsStamina::on_game_pause(bool toggle) {}
// bool DarkSoulsStamina::on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {}

std::optional<std::string> DarkSoulsStamina::on_initialize() {
    if (!install_hook_offset(0x3A9329, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init DarkSoulsStamina mod 1\n");
        return "Failed to init DarkSoulsStamina mod 1";
	}

    return Mod::on_initialize();
}

void DarkSoulsStamina::on_config_load(const utility::Config& cfg){
    stamina_enabled = cfg.get<bool>("DarkSoulsStamina").value_or(false);
}

void DarkSoulsStamina::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("DarkSoulsStamina", stamina_enabled);
}
