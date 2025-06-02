#include "KnockbackEdits.hpp"

bool KnockbackEdits::mod_enabled = false;
uintptr_t KnockbackEdits::jmp_return0 = NULL;
uintptr_t KnockbackEdits::jmp_return1 = NULL;
uintptr_t KnockbackEdits::jmp_return2 = NULL;

static bool release_stuns = false;
static bool volcano_launches = false;

static bool showAttackStatus = false;
static bool getMelee = true;
static bool getProjectiles = true;
static bool hideNoAttacks = true;
static bool hideNoUsed = true;

static uintptr_t previousAttackStatus = NULL;

naked void detour0() { // get all projectiles
    _asm {
        cmp byte ptr [showAttackStatus], 1
        jne code
        cmp byte ptr [getProjectiles], 1
        jne code
        mov [previousAttackStatus], edx
        code:
        mov esi,edx // edx = string of move at the start of info
        repe movsd // esi incs 4 every repe movsd starting at edx
        mov byte ptr [eax+0x000000B3],00
		jmp dword ptr [KnockbackEdits::jmp_return0]
    }
}

naked void detour1() { // most projectile hits
    _asm {
        mov esi, edx // edx = string of move at the start of info
        repe movsd // esi incs 4 every repe movsd starting at edx
        pop edi

        cmp dword ptr [eax+0xA4+0x00], 1162626386 // RELE(ASE) // get any release
        je release
        //cmp dword ptr [eax+0xA4+0x00], 0x5F4F5754 // TWO_ // get twosome time
        //je twosomeTime
        jmp retcode

    release:
        cmp byte ptr [release_stuns], 1
        jne retcode
        cmp dword ptr [eax+0xA4+0x08], 3145793 // A 0 // get level 1 release
        jne retcode
        mov word ptr [eax+0xA4+0x1C], 2 // make it hit
        jmp retcode

    twosomeTime:
        // cmp byte ptr [twosome_knocksback], 1
        // jne retcode
        cmp dword ptr [eax+0xA4+0x0C], 0x004C5F00 // ._L.
        jne retcode
        mov word ptr [eax+0xA4+0x34], 2 // make it knock back
        jmp retcode

    retcode:
		jmp dword ptr [KnockbackEdits::jmp_return1]
    }
}

naked void detour2() { // melee
    _asm {
        cmp byte ptr [showAttackStatus], 1
        jne dontGet
        cmp byte ptr [getMelee], 1
        jne dontGet

        cmp byte ptr [hideNoAttacks], 1
        jne checkNoUsed
        cmp dword ptr [eax], 0x612D6F6E // no-a(attack)
        je dontGet

        checkNoUsed:
        cmp byte ptr [hideNoUsed], 1
        jne get
        cmp dword ptr [eax], 0x755F6F4E // No_u(sed)
        je dontGet

        get:
        mov [previousAttackStatus], eax
        dontGet:
        cmp byte ptr [volcano_launches], 1
        jne originalcode

        cmp dword ptr [edx+0xA4+0x8], 0x346F6E72 // GIL-infe"rno4"751
        jne originalcode
        mov word ptr [edx+0xA4+0x34], 4 // make it launch
    originalcode:
        mov byte ptr [edx+0x000000B3],00
		jmp dword ptr [KnockbackEdits::jmp_return2]
    }
}

std::optional<std::string> KnockbackEdits::on_initialize() {
    if (!install_hook_offset(0x10CAD7, hook0, &detour0, &jmp_return0, 11)) { // get all projectiles
        spdlog::error("Failed to init KnockbackEdits mod 0\n");
        return "Failed to init KnockbackEdits mod 0";
    }
    if (!install_hook_offset(0x1099F8, hook1, &detour1, &jmp_return1, 5)) { // projectile hits
        spdlog::error("Failed to init KnockbackEdits mod 1\n");
        return "Failed to init KnockbackEdits mod 1";
    }
    if (!install_hook_offset(0x10CA35, hook2, &detour2, &jmp_return2, 7)) { // melee
        spdlog::error("Failed to init KnockbackEdits mod 2\n");
        return "Failed to init KnockbackEdits mod 2";
    }
    return Mod::on_initialize();
}

void KnockbackEdits::on_frame(fmilliseconds& dt) {
    if (showAttackStatus) {
        kAttackStatus_v3* attack = (kAttackStatus_v3*)previousAttackStatus;
        ImGui::Begin("AttackStatusUI", &showAttackStatus, ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Checkbox("Get Melee", &getMelee);
        ImGui::Checkbox("Get Projectiles", &getProjectiles);
        ImGui::Checkbox("Hide \"no-attack\" entries", &hideNoAttacks);
        ImGui::Checkbox("Hide \"No_used\" entries", &hideNoUsed);
        
        if (attack) {
            ImGui::Text(attack->mAsName);
            ImGui::PushItemWidth(sameLineItemWidth);
            ImGui::Combo("mAttackID", (int*)&attack->mAttackID, attack_id_names, IM_ARRAYSIZE(attack_id_names));
            ImGui::InputFloat("mDamageValue", &attack->mDamageValue);
            ImGui::InputInt("mAttackLv", &attack->mAttackLv);
            ImGui::InputInt("mAttackLvI", &attack->mAttackLvI);
            ImGui::InputInt("mAttackLvB", &attack->mAttackLvB);
            ImGui::Combo("mRangeType", (int*)&attack->mRangeType, range_type_names, IM_ARRAYSIZE(range_type_names));
            ImGui::InputInt("mHitStopTimer", &attack->mHitStopTimer);
            ImGui::Combo("mDamageType", (int*)&attack->mDamageType, damage_type_names, IM_ARRAYSIZE(damage_type_names));
            ImGui::InputInt("mDamageTypeI (turn enemy)", &attack->mDamageTypeI);
            ImGui::InputInt("mDamageTypeB (launch n shits)", &attack->mDamageTypeB);
            ImGui::Combo("mHitMarkAngle", (int*)&attack->mHitMarkAngle, hit_mark_angle_names, IM_ARRAYSIZE(hit_mark_angle_names));
            ImGui::InputInt("mHitSE", (int*)&attack->mHitSE);
            ImGui::InputFloat("mStylishPoint", &attack->mStylishPoint);
            ImGui::InputFloat("mStylishTimer", &attack->mStylishTimer);
            ImGui::InputFloat("mDTAdd", &attack->mDTAdd);
            ImGui::Combo("mAttackFlag", (int*)&attack->mAttackFlag, attack_flag_names, IM_ARRAYSIZE(attack_flag_names));
            ImGui::Combo("mBlownAngleType", (int*)&attack->mBlownAngleType, blown_angle_type_names, IM_ARRAYSIZE(blown_angle_type_names));
            ImGui::Combo("mElementType", (int*)&attack->mElementType, element_type_dmc4_names, IM_ARRAYSIZE(element_type_dmc4_names));
            ImGui::PopItemWidth();
        }
        ImGui::End();
    }
}

void KnockbackEdits::on_gui_frame(int display) {
    if (display == 2) {
        ImGui::Checkbox(_("Release Always Stuns"), &release_stuns);
        ImGui::SameLine();
        help_marker(_("Release with no meter can stun the enemy"));
        ImGui::SameLine(sameLineWidth);
        ImGui::Checkbox(_("Shock Launches"), &volcano_launches);
    }
    if (display == 0) {
        ImGui::Checkbox("Previous Attack Data", &showAttackStatus);
    }
}

void KnockbackEdits::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("knockback_edits").value_or(false);
    release_stuns = cfg.get<bool>("release_always_stuns").value_or(false);
    volcano_launches = cfg.get<bool>("volcano_launches").value_or(false);
}

void KnockbackEdits::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("knockback_edits", mod_enabled);
    cfg.set<bool>("release_always_stuns", release_stuns);
    cfg.set<bool>("volcano_launches", volcano_launches);
}
