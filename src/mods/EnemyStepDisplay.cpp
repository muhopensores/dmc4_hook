#include "EnemyStepDisplay.hpp"
#include "../sdk/Devil4.hpp"

#if 1
bool EnemyStepDisplay::mod_enabled = false;
uintptr_t EnemyStepDisplay::jmp_ret = NULL;
bool EnemyStepDisplay::jc_possible = false;
float EnemyStepDisplay::jc_possible_timer = 0.0f;
bool EnemyStepDisplay::showExtraStats = false;
ImVec2 EnemyStepDisplay::windowPos{ 0.0f, 0.0f };
static constexpr uintptr_t sUnit = 0xE552CC;
static constexpr uintptr_t sMediator = 0xE558B8;
static constexpr uintptr_t detour1_getEnemies = 0x402BD0;
static constexpr uintptr_t detour1_getCanJC = 0x4AB170;

naked void call1(void) {
    _asm {
        pushad
        movzx ebx, byte ptr [EnemyStepDisplay::jc_possible]
        
        mov esi, [sUnit]
        mov esi, [esi]
        mov edx, [esi+0xD4] // get delta time
        test edx, edx
        je popcode
        sub esp, 4
        movss xmm0, [edx+0x10]
        movss [esp], xmm0 // Store delta time
        mov esi, [esi+0x194] // get enemies
        test esi, esi
        je cleanup
        jmp CheckCanJC

    GetNextEnemy:
        mov esi, [esi+0x8]
        test esi, esi
        je save_and_exit_false
    CheckCanJC:
        push esi // arg
        call dword ptr [detour1_getCanJC] // call 0x4AB170
        
        mov cl, al
        cmp cl, 1
        jne GetNextEnemy
        
        mov esi, [sMediator]
        mov esi, [esi]
        test esi, esi
        je save_and_exit_false
        
        mov esi, [esi+0x24]
        test esi, esi
        je save_and_exit_false
        
        cmp dword ptr [esi+0x1FF4], 0 // has jc timer expired 
        jne save_and_exit_false

        cmp byte ptr [esi+0x2005], 0 // is enemy step banned
        je save_and_exit_false
        
        mov esi, [esi+0x1e8c]
        cmp dword ptr [esi+0x1c], 0 // groundedActual
        jne save_and_exit_false
        
        cmp ebx, 0
        jne just_update_timer
        
        mov dword ptr [EnemyStepDisplay::jc_possible_timer], 0
        
    just_update_timer:
        movss xmm0, [EnemyStepDisplay::jc_possible_timer]
        movss xmm1, [esp]
        addss xmm0, xmm1
        movss [EnemyStepDisplay::jc_possible_timer], xmm0
        
        mov al, 1
        jmp save_and_exit
        
    save_and_exit_false:
        xor al, al
        
    save_and_exit:
        mov [EnemyStepDisplay::jc_possible], al
        
    cleanup:
        add esp, 4
        
    popcode:
        popad
        ret
    }
}

/*naked void detour1(void) {
    _asm {
        cmp byte ptr [EnemyStepDisplay::mod_enabled], 0
        je originalcode
        pushad
        mov esi, [sUnit]
        mov esi, [esi]
        mov esi, [esi+0x194]
        test esi, esi
        je popcode
        push esi
        call dword ptr [detour1_getCanJC]
        mov [EnemyStepDisplay::jc_possible], al
    popcode:
        popad
    originalcode:
        mov eax, [edi+0x00001410]
        jmp dword ptr [EnemyStepDisplay::jmp_ret]
    }
}*/

void EnemyStepDisplay::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        uPlayer* player = devil4_sdk::get_local_player();
        if (!player) { return; }
        call1();
        static constexpr int WindowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoBackground;
        ImGui::Begin("Enemy Step Possible UI", NULL, WindowFlags);
        ImGui::SetWindowPos(windowPos, ImGuiCond_Once);
        windowPos = ImGui::GetWindowPos();
        ImGui::PushItemWidth(sameLineItemWidth);
        ImGui::Checkbox("##Enemy Step Possible Checkbox", &jc_possible);
        if (showExtraStats) {
            ImGui::SameLine();
            help_marker(_("Is enemy step possible?"));
            ImGui::SameLine();
            ImGui::Checkbox("##Enemy Step Enabled Checkbox", &player->enemyStepEnabled);
            ImGui::SameLine();
            help_marker(_("Is enemy step enabled?\n(Nero's Buster moves and possibly other things disable enemy step)"));
            ImGui::SameLine();
            ImGui::Checkbox("##Grounded", (bool*)player->collisionSettings->mLand);
            ImGui::SameLine();
            help_marker(_("Is player grounded?"));
            ImGui::InputFloat("##Enemy Step Possible InputFloat", &jc_possible_timer);
            ImGui::SameLine();
            help_marker(_("Enemy step was possible for this number of 60fps frames"));
            ImGui::SliderFloat("##Jump Cooldown SliderFloat", &player->jcTimer, 0.0f, 20.0f);
            ImGui::SameLine();
            help_marker(_("Jump Cooldown"));
        }
        ImGui::PopItemWidth();
        ImGui::End();
    }
}

std::optional<std::string> EnemyStepDisplay::on_initialize() {
    /*if (!install_hook_offset(0x4049C7, hook, &detour1, &EnemyStepDisplay::jmp_ret, 6)) {
        spdlog::error("Failed to init EnemyStepDisplay mod\n");
        return "Failed to init EnemyStepDisplay mod";
    }*/
    return Mod::on_initialize();
}

void EnemyStepDisplay::on_gui_frame(int display) {
    ImGui::BeginGroup();
    ImGui::Checkbox(_("Enemy Step Display"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("See if it was possible to enemy step in that combo after all"));
    if (mod_enabled) {
        ImGui::Indent(lineIndent);
        ImGui::Checkbox(_("Show Extra Info"), &showExtraStats);
        ImGui::Unindent(lineIndent);
    }
    ImGui::EndGroup();
}

void EnemyStepDisplay::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("enemy_step_display").value_or(false);
    showExtraStats = cfg.get<bool>("enemy_step_display_extra").value_or(false);
    windowPos.x = cfg.get<float>("enemy_step_display_pos_x").value_or(0.0f);
    windowPos.y = cfg.get<float>("enemy_step_display_pos_y").value_or(0.0f);
}

void EnemyStepDisplay::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("enemy_step_display", mod_enabled);
    cfg.set<bool>("enemy_step_display_extra", showExtraStats);
    cfg.set<float>("enemy_step_display_pos_x", windowPos.x);
    cfg.set<float>("enemy_step_display_pos_y", windowPos.y);
}

#endif
