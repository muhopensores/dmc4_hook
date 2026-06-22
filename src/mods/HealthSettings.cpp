#include "HealthSettings.hpp"
#include "sdk/Devil4.hpp"
#include "sdk/sStylishCount.hpp"
#include "glm/gtx/compatibility.hpp"
#include "Console.hpp"
#include "utility/MessageDisplay.hpp"

#if 1
bool HealthSettings::all_invincible = false;
bool HealthSettings::player_invincible = false;
bool HealthSettings::enemy_invincible = false;
bool HealthSettings::damageMultiplierToggle = false;
bool HealthSettings::mustStyleToggle = false;
bool HealthSettings::mightStyleToggle = false;
float HealthSettings::enemy_hp_display = 0.0f;
float HealthSettings::outgoingDamageMultiplier = 1.0f;
float HealthSettings::incomingDamageMultiplier = 1.0f;
float HealthSettings::mightStyleMultiplier = 0.0f;
uintptr_t HealthSettings::jmp_return = NULL;

bool HealthSettings::cant_die                  = false;
bool HealthSettings::one_hit_kill            = false;
uintptr_t HealthSettings::one_hit_kill_jmp_ret = NULL;
uintptr_t HealthSettings::one_hit_kill_jmp_out = 0x0051C129;

#include <xmmintrin.h> // simd microsoft specific thingies
// should be zero initialized due to static init
static __m128 xxmm0;
static __m128 xxmm1;
static __m128 xxmm2;
static __m128 xxmm3;
static __m128 xxmm4;

static void might_style_multiplier() {
    int currentRank = devil4_sdk::get_stylish_count()->current_style_tier;
    // E = 0.0, D = 0.2, C = 0.4, B = 0.6, A = 0.8, S and above = 1.0
    HealthSettings::mightStyleMultiplier = (float)std::min(currentRank, 5) / 5.0f;
}

naked void health_detour() {
    _asm {
        // quick check before we go looking through ptrs that any cheat is enabled
            push ecx
            mov cl, [HealthSettings::all_invincible]
            or cl, [HealthSettings::player_invincible]
            or cl, [HealthSettings::enemy_invincible]
            or cl, [HealthSettings::damageMultiplierToggle]
            or cl, [HealthSettings::mustStyleToggle]
            or cl, [HealthSettings::mightStyleToggle]
            test cl, cl
            je popcode
        // playercheck:
            mov ecx, [static_mediator_ptr]
            mov ecx, [ecx]
            test ecx, ecx
            je popcode
            mov ecx, [ecx+0x24]
            test ecx, ecx
            je popcode
            lea ecx, [ecx+0x15B4]
            cmp ecx, esi
            pop ecx
            jne EnemyDamaged
        // playerDamaged:
            cmp byte ptr [HealthSettings::player_invincible], 1
            je retcode
            cmp byte ptr [HealthSettings::damageMultiplierToggle], 1
            je IncomingMult
            jmp code

        EnemyDamaged:
            cmp byte ptr [HealthSettings::enemy_invincible], 1
            je retcode
            cmp byte ptr [HealthSettings::mustStyleToggle], 1
            je MustStyleCheck
            cmp byte ptr [HealthSettings::damageMultiplierToggle], 1
            je OutgoingMult
            jmp MightStyleCheck

        MustStyleCheck:
            push ecx
            mov ecx, [0x00E558CC]
            mov ecx, [ecx]
            test ecx, ecx
            je popcode
            cmp [ecx+0x20], 5 // S rank
            pop ecx
            jl retcode
            // jmp OutgoingMult
        OutgoingMult:
            mulss xmm1, [HealthSettings::outgoingDamageMultiplier]
        MightStyleCheck:
            cmp byte ptr [HealthSettings::mightStyleToggle], 1
            jne code
            movdqa [xxmm0], xmm0        // simd i
            movdqa [xxmm1], xmm1
            movdqa [xxmm2], xmm2
            movdqa [xxmm3], xmm3
            movdqa [xxmm4], xmm4
            pushad
            call might_style_multiplier  // moved might style update here for micro-optimizations because DamageMultiplier::OnFrame()
            popad                        // showed up on luke stackwalker once, fucks eax+ecx on debug
            movdqa xmm0, [xxmm0]         // simd o
            movdqa xmm1, [xxmm1]
            movdqa xmm2, [xxmm2]
            movdqa xmm3, [xxmm3]
            movdqa xmm4, [xxmm4]
            mulss xmm1, [HealthSettings::mightStyleMultiplier]
            jmp code

        IncomingMult:
            mulss xmm1, [HealthSettings::incomingDamageMultiplier]
            jmp code

        popcode:
            pop ecx
        code:
            cmp byte ptr [HealthSettings::all_invincible], 1
            je retcode
            subss xmm0,xmm1
        retcode:
            movss [HealthSettings::enemy_hp_display], xmm0
            comiss xmm2,xmm0
            jmp dword ptr [HealthSettings::jmp_return]
    }
}

naked void one_hit_kill_detour(void) {
    _asm {
            cmp byte ptr [HealthSettings::one_hit_kill], 1
            je playercheck
			cmp byte ptr [HealthSettings::cant_die], 1
			je jmpout
        code:
			comiss xmm2, [esi+0x18]
			jb jmpout
        jmpret:
			jmp dword ptr [HealthSettings::one_hit_kill_jmp_ret]

		jmpout:
			jmp dword ptr [HealthSettings::one_hit_kill_jmp_out]

        playercheck:
            push ecx
            push edx
            mov ecx, [static_mediator_ptr]
            mov ecx, [ecx]
            mov ecx, [ecx+0x24]
            mov edx, [esi+0xC0]
            cmp edx, ecx
            pop edx
            pop ecx
            je code
            jmp jmpret
    }
}

void HealthSettings::no_death_toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x11C694, omen_patch, "\xEB", 1);
    } else {
        omen_patch.reset();
    }
}

std::optional<std::string> HealthSettings::on_initialize() {
    if (!install_hook_offset(0x11BFD9, health_hook, &health_detour, &jmp_return, 7)) {
        spdlog::error("Failed to init HealthSettings mod\n");
        return "Failed to init HealthSettings mod";
    }

    console->system().RegisterCommand("damagemult", "Set outgoing damage multiplier", 
        [](float value) {
            HealthSettings::damageMultiplierToggle = true;
            HealthSettings::outgoingDamageMultiplier = value;
        }, 
        csys::Arg<float>("1.0 default"));    

    utility::create_keyboard_hotkey(HealthSettings::m_hotkeys, {VK_F1}, __("Infinite Health (All)"), "inf_hp_key");

    console->system().RegisterCommand("god", "Infinite player HP", [/*this*/]() {
        player_invincible = !player_invincible;
    });

    console->system().RegisterCommand("enemygod", "Infinite enemy HP", [/*this*/]() {
        enemy_invincible = !enemy_invincible;
    });

    if (!install_hook_offset(0x011C117, one_hit_kill_hook, &one_hit_kill_detour, &one_hit_kill_jmp_ret, 6)) {
        spdlog::error("Failed to init HealthSettings mod 2\n");
        return "Failed to init HealthSettings mod 2";
    }

    MutatorRegistry::define("OneHitKill").description("hehe").on_init([&] { one_hit_kill = true; }).set_timer(15.0, [&] {
        one_hit_kill = false;
    });

    console->system().RegisterCommand("onehitkill", "Enables one hit kill", [this]() { HealthSettings::one_hit_kill = !HealthSettings::one_hit_kill; });

    utility::create_keyboard_hotkey(HealthSettings::m_hotkeys, {VK_F3}, __("One Hit Kill"), "one_hit_kill_key");

    return Mod::on_initialize();
}

void HealthSettings::on_gui_frame(int display) {
    if (display == DISPLAY_SYSTEM_A) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Infinite Health (All)"), &all_invincible)) {
            player_invincible = all_invincible;
            enemy_invincible  = all_invincible;

            cant_die          = all_invincible;
            no_death_toggle(cant_die);
        }
        ImGui::SameLine();
        help_marker(_("Disable any entity's health decreasing. By default this is hotkeyed to F1"));
        ImGui::Indent(lineIndent);
        if (ImGui::Checkbox(_("Infinite Health (Player)"), &player_invincible)) {
            if (player_invincible && enemy_invincible)
                all_invincible = true;
            else
                all_invincible = false;
        }
        ImGui::SameLine();
        help_marker(_("Disable the player's health decreasing"));

        if (ImGui::Checkbox(_("Infinite Health (Enemy)"), &enemy_invincible)) {
            if (player_invincible && enemy_invincible)
                all_invincible = true;
            else
                all_invincible = false;
            cant_die = enemy_invincible;
            no_death_toggle(cant_die);
        }
        ImGui::SameLine();
        help_marker(_("Disable enemy's health decreasing"));
        ImGui::Unindent(lineIndent);
        ImGui::EndGroup();

        ImGui::SameLine(sameLineWidth);

        ImGui::BeginGroup();
        ImGui::Checkbox(_("Damage Multipliers"), &damageMultiplierToggle);
        if (damageMultiplierToggle) {
            ImGui::Indent(lineIndent);
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::InputFloat(_("Outgoing"), &HealthSettings::outgoingDamageMultiplier, 0.1f, 1.0f, "%.1f");
            ImGui::InputFloat(_("Incoming"), &HealthSettings::incomingDamageMultiplier, 0.1f, 1.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();

        ImGui::Checkbox(_("No Death (All)"), &cant_die);
        ImGui::SameLine();
        help_marker(
            _("Disable any entity dying while still allowing HP to drop.\nFor practicing against enemies who have AI changes at "
                "different levels of HP\nAlso stops Omen killing enemies."));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("One Hit Kill"), &one_hit_kill);
        ImGui::SameLine();
        help_marker(_("Enable one hit kill. This overrides the damage multiplier. By default this is hotkeyed to F3"));
    }

    if (display == DISPLAY_SYSTEM_B) {
        ImGui::Checkbox(_("Must style mode"), &mustStyleToggle);
        ImGui::SameLine();
        help_marker(_("Damage is disabled until reaching S rank"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Might style mode"), &mightStyleToggle);
        ImGui::SameLine();
        help_marker(_("Damage scales with Style\n0 damage at no rank to 1.0 damage at S"));
    }
}

void HealthSettings::on_update_input(utility::Input& input) {
    if (HealthSettings::m_hotkeys[0]->check(input)) {
        if (all_invincible) {
            DISPLAY_MESSAGE("Infinite Health (All) Off");
            all_invincible    = false;
            player_invincible = false;
            enemy_invincible  = false;

            cant_die = false;
            no_death_toggle(cant_die);
        } else {
            DISPLAY_MESSAGE("Infinite Health (All) On");
            all_invincible    = true;
            player_invincible = true;
            enemy_invincible  = true;

            cant_die = true;
            no_death_toggle(cant_die);
        }
    }
    if (HealthSettings::m_hotkeys[1]->check(input)) {
        if (one_hit_kill) {
            DISPLAY_MESSAGE("One Hit Kill Off");
        } else {
            DISPLAY_MESSAGE("One Hit Kill On");
        }
        one_hit_kill = !one_hit_kill;
    }
}

void HealthSettings::on_config_load(const utility::Config& cfg) {
    player_invincible = cfg.get<bool>("infinite_health_player").value_or(false);
    enemy_invincible  = cfg.get<bool>("infinite_health_enemy").value_or(false);
    if (player_invincible && enemy_invincible) {
        all_invincible = true;
    }
    outgoingDamageMultiplier = cfg.get<float>("outgoing_damage_multiplier").value_or(1.0f);
    incomingDamageMultiplier = cfg.get<float>("incoming_damage_multiplier").value_or(1.0f);
    mustStyleToggle = cfg.get<bool>("must_style").value_or(false);
    mightStyleToggle = cfg.get<bool>("might_style").value_or(false);

    cant_die = cfg.get<bool>("no_death").value_or(false);
    one_hit_kill = cfg.get<bool>("one_hit_kill").value_or(false);
    if (cant_die) no_death_toggle(cant_die);

};

void HealthSettings::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_health_player", player_invincible);
    cfg.set<bool>("infinite_health_enemy", enemy_invincible);
    cfg.set<float>("outgoing_damage_multiplier", outgoingDamageMultiplier);
    cfg.set<float>("incoming_damage_multiplier", incomingDamageMultiplier);
    cfg.set<bool>("must_style", mustStyleToggle);
    cfg.set<bool>("might_style", mightStyleToggle);

    cfg.set<bool>("no_death", cant_die);
    cfg.set<bool>("one_hit_kill", one_hit_kill);
};

#endif
