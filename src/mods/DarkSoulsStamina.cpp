#include "DarkSoulsStamina.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sRender.hpp"
#include "sdk/uPlayer.hpp"
#include "sdk/Players.hpp"
#include <cstdio>
uintptr_t DarkSoulsStamina::jmp_ret1 = NULL;
static glm::ivec2 screen_res{0, 0};

float DarkSoulsStamina::stamina_regen_rate = 500.0f;
float DarkSoulsStamina::stamina_max = 1000.0f;
float DarkSoulsStamina::stamina_delay_max = 1.0f;

bool DarkSoulsStamina::stamina_enabled = false;
bool DarkSoulsStamina::meme_stamina_enabled = false;

float DarkSoulsStamina::stamina_ghost_delay_max = 0.5f;
float DarkSoulsStamina::stamina_ghost_rate = 500.0f;

uintptr_t DarkSoulsStamina::gauge_player[DarkSoulsStamina::kSlots] = {};
uint8_t DarkSoulsStamina::exhausted[DarkSoulsStamina::kSlots] = {};

namespace {

// one player's gauge; reset to full when the slot's player changes (a new room makes new players)
struct Gauge {
    uintptr_t player     = 0;
    float stamina        = 0.0f;
    float delay          = 0.0f;   // since the last spend; regen starts at stamina_delay_max
    float spent_visual   = 0.0f;   // the ghost bar
    float ghost_delay    = 0.0f;
    int   prev_move      = -1;
    float prev_frame     = -1.0f;
};
Gauge gauges[DarkSoulsStamina::kSlots];

} // namespace

// atm I kill inputs because I don't know how to stop the player from being able to do actions other than movement
naked void detour1() { // gamepad inputs (keyboard is handled by detour2 in InputStates.cpp), player in esi
    _asm {
            cmp byte ptr [DarkSoulsStamina::stamina_enabled], 1
            je newcode
            cmp byte ptr [DarkSoulsStamina::meme_stamina_enabled], 1
            jne originalcode

        newcode:
            push ecx
            xor ecx, ecx
        find:
            cmp esi, DarkSoulsStamina::gauge_player[ecx*4]
            je found
            inc ecx
            cmp ecx, 4 // kSlots
            jb find
            pop ecx
            jmp originalcode
        found:
            cmp byte ptr DarkSoulsStamina::exhausted[ecx], 0
            pop ecx
            jne skip
        originalcode:
            mov [esi+0x00001410], eax
        skip:
            jmp dword ptr [DarkSoulsStamina::jmp_ret1]
    }
}

float DarkSoulsStamina::get_move_cost(uPlayer_old* player, int moveID) {
    if (!player) return 0.0f;

    if (player->controllerID == 0) {// dante
        if ((moveID >= 0x000 && moveID <= 0x003) || // movement
            (moveID == 0x004 && !player->collisionSettings->mLand) || // falling anim // landing anim still drains stamina :(
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
            return 100.0f; // reb + nero
        if (moveID > 0x300 && moveID < 0x400)
            return 150.0f; // gilg
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
    }
    else { // nero
        if ((moveID >= 0x000 && moveID <= 0x003) || // movement
            (moveID == 0x004 && !player->collisionSettings->mLand) || // falling anim // landing anim still drains stamina :(
            (moveID >= 0x96 && moveID <= 0xA1)  ||  // taunts
            moveID == 0x006 || // stop from run
            moveID == 0x00D || // stop from sprint
            moveID == 0x300) { // rq idle
            //(moveID >= 0x502 && moveID <= 0x51E) || // lock on idle, strafing, shooting
            //moveID == 0x528) { // ground basic shot
            return 0.0f;
        }
        if (moveID == 0x525) // air basic shot
            return 20.0f;
        if (moveID == 0x52B || // ground charge shot
            moveID == 0x52E) // air charge shot
            return 150.0f;
        if (moveID == 0x100 || // ground snatch
            moveID == 0x10F) // air snatch
            return 100.0f;
        if (moveID == 0x11E || // ground buster
            moveID == 0x123) // air buster
            return 150.0f;
        if (moveID > 0x3 && moveID < 0x200)
            return 100.0f; // movement
        if (moveID > 0x300 && moveID < 0x400)
            return 100.0f; // rq
        // if (moveID > 0x500 && moveID < 0x600)
            // return 0.0f; // blue rose
    }
    return 0.0f;
}

static void stamina_regen(Gauge& g, float seconds) {
    if (g.delay < DarkSoulsStamina::stamina_delay_max) {
        g.delay += seconds;
    } else {
        if (g.stamina < DarkSoulsStamina::stamina_max) {
            g.stamina += DarkSoulsStamina::stamina_regen_rate * seconds;
            if (g.stamina > DarkSoulsStamina::stamina_max)
                g.stamina = DarkSoulsStamina::stamina_max;
        }
    }
}

static void ghost_drain(Gauge& g, float seconds) {
    if (g.ghost_delay < DarkSoulsStamina::stamina_ghost_delay_max) {
        g.ghost_delay += seconds;
    } else {
        float diff = g.spent_visual - g.stamina;
        if (diff > 0.0f) {
            float drain = DarkSoulsStamina::stamina_ghost_rate * seconds;
            g.spent_visual -= std::min(drain, diff);
        } else {
            // snap when stamina refills past ghost
            g.spent_visual = g.stamina;
        }
    }
}

static void check_for_stamina_use(uPlayer_old* player, Gauge& g) {
    int moveID    = player->moveIDBest;
    float moveFrame = player->animFrame;
    if ((player->movePart == 1) ||
        (player->movePart == 2 && (moveID == 0x20B || moveID == 0x31)) ||  // dante+nero high time have move part 2 :harold:
        (player->movePart == 0 && (moveID == 0x525 || moveID == 0x52E || moveID == 0x52B))) { // nero air normal shot + charge shots have move part 0 :harold:
        bool moveReset = (moveID != g.prev_move) || (moveID == g.prev_move && moveFrame < g.prev_frame);
        if (moveReset) {
            float cost = DarkSoulsStamina::get_move_cost(player, moveID);
            if (cost > 0.0f) {
                g.stamina -= cost;
                g.ghost_delay = 0.0f;
                g.delay       = 0.0f;
            }
        }
        g.prev_move  = moveID;
        g.prev_frame = moveFrame;
    }
}

// the bar at the same place in the player's own view (the whole screen without a split)
static void draw_bar(const Gauge& g, int slot, const float rect[4]) {
    const float vx = rect[0] * screen_res.x, vy = rect[1] * screen_res.y;
    const float vw = (rect[2] - rect[0]) * screen_res.x;
    const float vh = (rect[3] - rect[1]) * screen_res.y;
    char name[32];
    std::snprintf(name, sizeof name, slot == 0 ? "StaminaGUI" : "StaminaGUI##%d", slot);
    ImGui::Begin(name, nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_AlwaysAutoResize);

    // pos
    float bar_width  = vw * 0.250f;
    float bar_height = vw * 0.0075f;
    ImVec2 window_size = ImGui::GetWindowSize();
    ImVec2 window_pos(vx + vw * 0.16f, vy + vh * 0.2375f);
    ImGui::SetWindowPos(window_pos);
    float wideLeftX = (window_size.x - bar_width) * 0.75f;
    ImGui::SetCursorPosX(wideLeftX);

    // bar
    ImDrawList* draw = ImGui::GetWindowDrawList();
    ImVec2 pos       = ImGui::GetCursorScreenPos();
    float stamina_frac       = g.stamina / DarkSoulsStamina::stamina_max;
    stamina_frac             = std::clamp(stamina_frac, 0.0f, 1.0f);
    float stamina_spent_frac = g.spent_visual / DarkSoulsStamina::stamina_max;
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

    ImGui::End();
}

void DarkSoulsStamina::on_frame(fmilliseconds& dt) {
    if (!stamina_enabled && !meme_stamina_enabled) {
        for (int i = 0; i < kSlots; i++) {
            gauge_player[i] = 0;
            exhausted[i] = 0;
        }
        return;
    }
    sRender* sRen = devil4_sdk::get_sRender();
    if (!sRen) return;
    screen_res = sRen->screenRes;

    const float seconds = dt.count() / 1000.0f;
    const bool paused = devil4_sdk::is_paused();
    const int n = player_info::count();
    for (int i = 0; i < kSlots; i++) {
        const uintptr_t pl = i < n ? player_info::player(i) : 0;
        Gauge& g = gauges[i];
        if (g.player != pl) {
            g = Gauge{};
            g.player = pl;
            g.stamina = g.spent_visual = stamina_max;
        }
        gauge_player[i] = pl;
        if (!pl) {
            exhausted[i] = 0;
            continue;
        }
        if (!paused) {
            stamina_regen(g, seconds);
            ghost_drain(g, seconds);
        }
        float rect[4];
        if (player_info::view_rect(i, rect)) {
            draw_bar(g, i, rect);
        }
        check_for_stamina_use((uPlayer_old*)pl, g);
        exhausted[i] = g.stamina < 0.0f;
    }
}

void DarkSoulsStamina::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::Checkbox(_("Stamina"), &stamina_enabled);
        ImGui::SameLine();
        help_marker(_("I played a lot of Dark Souls recently and thought this would be funny. In co-op each player has his "
                      "own stamina, shown in his own view"));
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
