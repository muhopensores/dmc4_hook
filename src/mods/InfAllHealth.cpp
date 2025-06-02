#include "InfAllHealth.hpp"
#include "iostream"
#include "../sdk/Devil4.hpp"
#include "glm/gtx/compatibility.hpp"

#if 1
bool InfAllHealth::all_invincible = false;
bool InfAllHealth::player_invincible = false;
bool InfAllHealth::enemy_invincible = false;
bool InfAllHealth::damageMultiplierToggle = false;
bool InfAllHealth::mustStyleToggle = false;
bool InfAllHealth::mightStyleToggle = false;
float InfAllHealth::enemy_hp_display = 0.0f;
float InfAllHealth::outgoingDamageMultiplier = 1.0f;
float InfAllHealth::incomingDamageMultiplier = 1.0f;

float InfAllHealth::mightStyleMultiplier = 0.0f;

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
    InfAllHealth::mightStyleMultiplier = (float)std::min(currentRank, 5) / 5.0f;
}

uintptr_t InfAllHealth::jmp_return{NULL};

constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

naked void detour() {
    _asm {
        // quick check before we go looking through ptrs that any cheat is enabled
            push ecx
            mov cl, [InfAllHealth::all_invincible]
            or cl, [InfAllHealth::player_invincible]
            or cl, [InfAllHealth::enemy_invincible]
            or cl, [InfAllHealth::damageMultiplierToggle]
            or cl, [InfAllHealth::mustStyleToggle]
            or cl, [InfAllHealth::mightStyleToggle]
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
            cmp byte ptr [InfAllHealth::player_invincible], 1
            je retcode
            cmp byte ptr [InfAllHealth::damageMultiplierToggle], 1
            je IncomingMult
            jmp code

        EnemyDamaged:
            cmp byte ptr [InfAllHealth::enemy_invincible], 1
            je retcode
            cmp byte ptr [InfAllHealth::mustStyleToggle], 1
            je MustStyleCheck
            cmp byte ptr [InfAllHealth::damageMultiplierToggle], 1
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
            mulss xmm1, [InfAllHealth::outgoingDamageMultiplier]
        MightStyleCheck:
            cmp byte ptr [InfAllHealth::mightStyleToggle], 1
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
            mulss xmm1, [InfAllHealth::mightStyleMultiplier]
            jmp code

        IncomingMult:
            mulss xmm1, [InfAllHealth::incomingDamageMultiplier]
            jmp code

        popcode:
            pop ecx
        code:
            cmp byte ptr [InfAllHealth::all_invincible], 1
            je retcode
            subss xmm0,xmm1
        retcode:
            movss [InfAllHealth::enemy_hp_display], xmm0
            comiss xmm2,xmm0
            jmp dword ptr [InfAllHealth::jmp_return]
    }
}

std::optional<std::string> InfAllHealth::on_initialize() {
    if (!install_hook_offset(0x11BFD9, hook, &detour, &jmp_return, 7)) {
        spdlog::error("Failed to init InfAllHealth mod\n");
        return "Failed to init InfAllHealth mod";
    }

    console->system().RegisterCommand("damagemult", "Set outgoing damage multiplier", 
        [](float value) {
            InfAllHealth::damageMultiplierToggle = true;
            InfAllHealth::outgoingDamageMultiplier = value;
        }, 
        csys::Arg<float>("1.0 default"));    

    utility::create_keyboard_hotkey(InfAllHealth::m_hotkeys, {VK_F1}, __("Infinite Health (All)"), "inf_hp_key");

    console->system().RegisterCommand("god", "Infinite player HP", [/*this*/]() {
        player_invincible = !player_invincible;
    });

    console->system().RegisterCommand("enemygod", "Infinite enemy HP", [/*this*/]() {
        enemy_invincible = !enemy_invincible;
    });

    return Mod::on_initialize();
}

void InfAllHealth::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::BeginGroup();
        if (ImGui::Checkbox(_("Infinite Health (All)"), &all_invincible)) {
            if (all_invincible) {
                player_invincible = true;
                enemy_invincible = true;
            }
            else {
                player_invincible = false;
                enemy_invincible = false;
            }
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
            ImGui::InputFloat(_("Outgoing"), &InfAllHealth::outgoingDamageMultiplier, 0.1f, 1.0f, "%.1f");
            ImGui::InputFloat(_("Incoming"), &InfAllHealth::incomingDamageMultiplier, 0.1f, 1.0f, "%.1f");
            ImGui::PopItemWidth();
            ImGui::Unindent(lineIndent);
        }
        ImGui::EndGroup();
    }

    if (display == 2) {
        ImGui::Checkbox(_("Must style mode"), &mustStyleToggle);
        ImGui::SameLine();
        help_marker(_("Damage is disabled until reaching S rank"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Might style mode"), &mightStyleToggle);
        ImGui::SameLine();
        help_marker(_("Damage scales with Style\n0 damage at no rank to 1.0 damage at S"));
    }
}

void InfAllHealth::on_update_input(utility::Input& input) {
    if (InfAllHealth::m_hotkeys[0]->check(input)) {
        if (all_invincible) {
            DISPLAY_MESSAGE("Infinite Health (All) Off");
            all_invincible = false;
            player_invincible = false;
            enemy_invincible = false;
        }
        else {
            DISPLAY_MESSAGE("Infinite Health (All) On");
            all_invincible = true;
            player_invincible = true;
            enemy_invincible = true;
        }
    }
}

void InfAllHealth::on_config_load(const utility::Config& cfg) {
    player_invincible = cfg.get<bool>("infinite_health_player").value_or(false);
    enemy_invincible  = cfg.get<bool>("infinite_health_enemy").value_or(false);
    if (player_invincible && enemy_invincible) {
        all_invincible = true;
    }
    outgoingDamageMultiplier = cfg.get<float>("outgoing_damage_multiplier").value_or(1.0f);
    incomingDamageMultiplier = cfg.get<float>("incoming_damage_multiplier").value_or(1.0f);
    mustStyleToggle = cfg.get<bool>("must_style").value_or(false);
    mightStyleToggle = cfg.get<bool>("might_style").value_or(false);

};

void InfAllHealth::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("infinite_health_player", player_invincible);
    cfg.set<bool>("infinite_health_enemy", enemy_invincible);
    cfg.set<float>("outgoing_damage_multiplier", outgoingDamageMultiplier);
    cfg.set<float>("incoming_damage_multiplier", incomingDamageMultiplier);
    cfg.set<bool>("must_style", mustStyleToggle);
    cfg.set<bool>("might_style", mightStyleToggle);
};

#endif
