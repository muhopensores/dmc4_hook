#include "Players.hpp"
#include "Devil4.hpp"
#include "../dev/coop/Coop.hpp"
#include "../dev/coop/CoopPads.hpp"
#include "../dev/coop/CoopHud.hpp"
#include "../mods/ViewportWindow.hpp"
#include "Cam.hpp"
#include <algorithm>

namespace player_info {

int index_of(uintptr_t player) {
    if (!player) {
        return -1;
    }
    if (Coop::mod_enabled) {
        for (int i = 0; i < coop::kMaxPlayers; i++) {
            if (coop::players[i].unit == player) {
                return i;
            }
        }
        if (coop::players[0].unit) {
            return -1;
        }
    }
    // co-op off, or turned on mid-stage before player 1 was registered: the game's player
    return player == (uintptr_t)devil4_sdk::get_local_player() ? 0 : -1;
}

bool __stdcall is_player(uintptr_t unit) {
    return index_of(unit) >= 0;
}

#pragma push_macro("naked")   // mod.hpp's naked is `static __declspec(naked)`; this one is declared in the header
#undef naked
__declspec(naked) void asm_is_player() {
    __asm {
        pushad
        sub esp, 128
        movdqu [esp], xmm0
        movdqu [esp+16], xmm1
        movdqu [esp+32], xmm2
        movdqu [esp+48], xmm3
        movdqu [esp+64], xmm4
        movdqu [esp+80], xmm5
        movdqu [esp+96], xmm6
        movdqu [esp+112], xmm7
        push eax
        call is_player
        movdqu xmm0, [esp]
        movdqu xmm1, [esp+16]
        movdqu xmm2, [esp+32]
        movdqu xmm3, [esp+48]
        movdqu xmm4, [esp+64]
        movdqu xmm5, [esp+80]
        movdqu xmm6, [esp+96]
        movdqu xmm7, [esp+112]
        add esp, 128
        test al, al
        popad
        ret
    }
}
#pragma pop_macro("naked")

bool has_keyboard(uintptr_t player) {
    const int i = index_of(player);
    if (i < 0) {
        return false;
    }
    return Coop::mod_enabled ? i == coop_pads::keyboard_owner : i == 0;
}

uintptr_t keyboard_player() {
    if (Coop::mod_enabled && coop::players[0].unit) {
        const int k = coop_pads::keyboard_owner;
        return k >= 0 && k < coop::kMaxPlayers ? coop::players[k].unit : 0;
    }
    return (uintptr_t)devil4_sdk::get_local_player();
}

namespace {
bool coop_players() {
    return Coop::mod_enabled && coop::players[0].unit;
}
} // namespace

int count() {
    if (coop_players()) {
        return (int)std::min<unsigned int>(Coop::player_num, coop::kMaxPlayers);
    }
    return devil4_sdk::get_local_player() ? 1 : 0;
}

uintptr_t player(int index) {
    if (coop_players()) {
        return index >= 0 && index < count() ? coop::players[index].unit : 0;
    }
    return index == 0 ? (uintptr_t)devil4_sdk::get_local_player() : 0;
}

bool view_rect(int index, float rect[4]) {
    if (!Coop::mod_enabled || !coop_hud::split_shown) {   // one view: player 1's, the whole screen
        if (index != 0) {
            return false;
        }
        rect[0] = rect[1] = 0.0f;
        rect[2] = rect[3] = 1.0f;
        return true;
    }
    if (index < 0 || index >= coop::kMaxPlayers || index == vpwin::detached_viewport()) {
        return false;
    }
    // player i's view is viewport i (Coop's layout); its region is in the screen's pixels
    const sCamera* cams = *reinterpret_cast<sCamera* const*>(0xE552D4);   // sCamera::mpInstance
    if (!cams || !cams->viewports[index].mActive || !cams->viewports[index].mpCamera) {
        return false;
    }
    const MtRect& s = cams->ScreenRect;
    const MtRect& r = cams->viewports[index].mRegion;
    const float w = (float)(int32_t)(s.r - s.l), h = (float)(int32_t)(s.b - s.t);
    if (w <= 0.0f || h <= 0.0f || (int32_t)(r.r - r.l) <= 0 || (int32_t)(r.b - r.t) <= 0) {
        return false;
    }
    rect[0] = (float)(int32_t)(r.l - s.l) / w;
    rect[1] = (float)(int32_t)(r.t - s.t) / h;
    rect[2] = (float)(int32_t)(r.r - s.l) / w;
    rect[3] = (float)(int32_t)(r.b - s.t) / h;
    return true;
}

} // namespace player_info
