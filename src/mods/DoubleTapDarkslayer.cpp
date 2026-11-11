#include "DoubleTapDarkslayer.hpp"
#include "../sdk/Players.hpp"

#if 1
uintptr_t DoubleTapDarkslayer::jmp_ret1 = NULL;

uintptr_t DoubleTapDarkslayer::jmp_ret2 = NULL;
uintptr_t DoubleTapDarkslayer::jmp_jb2 = 0x007B6CA1;

uintptr_t DoubleTapDarkslayer::jmp_ret3 = NULL;
uintptr_t DoubleTapDarkslayer::jmp_jb3 = 0x007B6D00;

uintptr_t DoubleTapDarkslayer::jmp_ret4 = NULL;
uintptr_t DoubleTapDarkslayer::jmp_jb4 = 0x007B6D5B;


uintptr_t DoubleTapDarkslayer::jmp_ret5 = NULL;
uintptr_t DoubleTapDarkslayer::jmp_jb5 = 0x007B6DB3;


bool DoubleTapDarkslayer::mod_enabled = false;

float DoubleTapDarkslayer::darkslayerTimerFill = 0.0f;
float DoubleTapDarkslayer::zeroFloat = 0.0f;

// The double-tap timer, one per player (player_info's index). Every hook below runs in uPlayerDante::checkStyleChange
// with the player in esi; a Dante who isn't a player (the boss) has no timer and keeps the game's own style change.
static float darkslayer_timers[4] = {};
static float* cur_timer = nullptr;   // load_timer's answer: esi's timer, or null

static float* __stdcall darkslayer_timer_of(uintptr_t player) {
    const int i = player_info::index_of(player);
    return i >= 0 && i < 4 ? &darkslayer_timers[i] : nullptr;
}

// esi = the player -> cur_timer; every register and xmm0 / xmm1 kept
naked void load_timer(void) {
    _asm {
            sub esp, 32
            movdqu [esp], xmm0
            movdqu [esp+16], xmm1
            pushad
            push esi
            call darkslayer_timer_of
            mov [cur_timer], eax
            popad
            movdqu xmm0, [esp]
            movdqu xmm1, [esp+16]
            add esp, 32
            ret
    }
}

naked void detour1(void) { // ticks and resets the timer // player in esi
    _asm {
            cmp byte ptr [DoubleTapDarkslayer::mod_enabled], 0
            je originalcode

            call load_timer
            cmp dword ptr [cur_timer], 0
            je originalcode
            push eax
            mov eax, [cur_timer]

            test dword ptr [esi+0x00001410], 0x00001000
            jne timerreset
            test dword ptr [esi+0x00001410], 0x00002000
            jne timerreset
            test dword ptr [esi+0x00001410], 0x00004000
            jne timerreset
            test dword ptr [esi+0x00001410], 0x00008000
            jne timerreset
            jmp timertick
        timerreset:
            movss xmm1, [DoubleTapDarkslayer::darkslayerTimerFill]
            movss [eax],xmm1

        timertick:
            movss xmm0, [eax]
            comiss xmm0, [DoubleTapDarkslayer::zeroFloat]
            jna popeax
            subss xmm0, [esi+0x10] // delta
            movss [eax],xmm0
            xorps xmm0, xmm0
        popeax:
            pop eax

	    originalcode:
		    comiss xmm0, [esi+0x00014D9C]
		    jmp dword ptr [DoubleTapDarkslayer::jmp_ret1]
    }
}

naked void detour2(void) { // trickster
    _asm {
            cmp byte ptr [DoubleTapDarkslayer::mod_enabled], 0
            je originalcode

            call load_timer
            cmp dword ptr [cur_timer], 0
            je originalcode
            push eax
            mov eax, [cur_timer]
            movss xmm0, [eax]
            pop eax
            comiss xmm0, [DoubleTapDarkslayer::zeroFloat]
            jbe jbjmp // dont run

	    originalcode:
		    cmp dword ptr [esi+0x00014D98], 02
		    jmp dword ptr [DoubleTapDarkslayer::jmp_ret2]

        jbjmp:
            jmp dword ptr [DoubleTapDarkslayer::jmp_jb2] // DevilMayCry4_DX9.exe+3B6CA1
    }
}

naked void detour3(void) { // royal guard
    _asm {
            cmp byte ptr [DoubleTapDarkslayer::mod_enabled], 0
            je originalcode

            call load_timer
            cmp dword ptr [cur_timer], 0
            je originalcode
            push eax
            mov eax, [cur_timer]
            movss xmm0, [eax]
            pop eax
            comiss xmm0, [DoubleTapDarkslayer::zeroFloat]
            jbe jbjmp // dont run

	    originalcode:
		    cmp dword ptr [esi+0x00014D98], 03
		    jmp dword ptr [DoubleTapDarkslayer::jmp_ret3]

        jbjmp:
            jmp dword ptr [DoubleTapDarkslayer::jmp_jb3] // DevilMayCry4_DX9.exe+3B6D00
    }
}

naked void detour4(void) { // gunslinger
    _asm {
            cmp byte ptr [DoubleTapDarkslayer::mod_enabled], 0
            je originalcode

            call load_timer
            cmp dword ptr [cur_timer], 0
            je originalcode
            push eax
            mov eax, [cur_timer]
            movss xmm0, [eax]
            pop eax
            comiss xmm0, [DoubleTapDarkslayer::zeroFloat]
            jbe jbjmp // dont run

	    originalcode:
		    cmp dword ptr [esi+0x00014D98], ebx
		    jmp dword ptr [DoubleTapDarkslayer::jmp_ret4]

        jbjmp:
            jmp dword ptr [DoubleTapDarkslayer::jmp_jb4] // DevilMayCry4_DX9.exe+3B6D5B
    }
}

naked void detour5(void) { // swordmaster
    _asm {
            cmp byte ptr [DoubleTapDarkslayer::mod_enabled], 0
            je originalcode

            call load_timer
            cmp dword ptr [cur_timer], 0
            je originalcode
            push eax
            mov eax, [cur_timer]
            movss xmm0, [eax]
            pop eax
            comiss xmm0, [DoubleTapDarkslayer::zeroFloat]
            jbe jbjmp // dont run

	    originalcode:
		    cmp dword ptr [esi+0x00014D98], 00
		    jmp dword ptr [DoubleTapDarkslayer::jmp_ret5]

        jbjmp:
            jmp dword ptr [DoubleTapDarkslayer::jmp_jb5] // DevilMayCry4_DX9.exe+3B6DB3
    }
}

std::optional<std::string> DoubleTapDarkslayer::on_initialize() {
    if (!install_hook_offset(0x03B6DC0, hook1, &detour1, &jmp_ret1, 7)) {
        spdlog::error("Failed to init DoubleTapDarkslayer mod\n");
        return "Failed to init DoubleTapDarkslayer mod";
    }
    if (!install_hook_offset(0x03B6C89, hook2, &detour2, &jmp_ret2, 7)) { // trickster
        spdlog::error("Failed to init DoubleTapDarkslayer mod\n");
        return "Failed to init DoubleTapDarkslayer mod";
    }
    if (!install_hook_offset(0x03B6CE8, hook3, &detour3, &jmp_ret3, 7)) { // royal guard
        spdlog::error("Failed to init DoubleTapDarkslayer mod\n");
        return "Failed to init DoubleTapDarkslayer mod";
    }
    if (!install_hook_offset(0x03B6D47, hook4, &detour4, &jmp_ret4, 6)) { // gunslinger
        spdlog::error("Failed to init DoubleTapDarkslayer mod\n");
        return "Failed to init DoubleTapDarkslayer mod";
    }
    if (!install_hook_offset(0x03B6D9E, hook5, &detour5, &jmp_ret5, 7)) { // swordmaster
        spdlog::error("Failed to init DoubleTapDarkslayer mod\n");
        return "Failed to init DoubleTapDarkslayer mod";
    }
    return Mod::on_initialize();
}

void DoubleTapDarkslayer::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::BeginGroup();
        ImGui::Checkbox(_("Double Tap Darkslayer"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Darkslayer is accessed by quickly pressing one style twice"));
        if (mod_enabled) {
            ImGui::Indent(lineIndent);
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::SliderFloat(_("Double Tap Timer"), &darkslayerTimerFill, 6.0f, 32.0f, "%.0f");
            ImGui::PopItemWidth();
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }
}

void DoubleTapDarkslayer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("double_tap_darkslayer").value_or(false);
    darkslayerTimerFill = cfg.get<float>("darkslayer_timer").value_or(16.0f);
}

void DoubleTapDarkslayer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("double_tap_darkslayer", mod_enabled);
    cfg.set<float>("darkslayer_timer", darkslayerTimerFill);
}

#endif
