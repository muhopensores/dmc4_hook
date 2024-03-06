#include "CharSwitcher.hpp"

bool CharSwitcher::mod_enabled{false};
bool CharSwitcher::inertia_enabled{true};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
constexpr uintptr_t sArea               = 0x00E552C8;
constexpr uintptr_t sSave               = 0x00E558C8;
uintptr_t primaryActor                  = NULL;
uintptr_t secondaryActor                = NULL;
uintptr_t primaryHUD                    = NULL;
uintptr_t secondaryHUD                  = NULL;
float swapGrav                          = -1.0f;
float ySpawn                            = -9000.0f;
int16_t desiredInput1                   = 0;
int16_t desiredInput2                   = 0;
int16_t prevInput                       = 0;

uintptr_t CharSwitcher::jmp_ret2{NULL};
    constexpr uintptr_t detour2_call1 = 0x008DF530;
uintptr_t CharSwitcher::jmp_ret3{NULL};
uintptr_t CharSwitcher::jmp_ret4{NULL};
    // Dante
    constexpr uintptr_t detour4_call1  = 0x00503240;
    constexpr uintptr_t detour4_call2  = 0x0076F200;
    constexpr uintptr_t detour4_call3  = 0x005079D0;
    constexpr uintptr_t detour4_call4  = 0x007B2150;
    // Nero                            
    constexpr uintptr_t detour4_call5  = 0x00507A90;
    constexpr uintptr_t detour4_call6  = 0x0076E790;
    constexpr uintptr_t detour4_call7  = 0x005079D0;
    constexpr uintptr_t detour4_call8  = 0x007E1B30;
    constexpr uintptr_t detour4_call9  = 0x008DC540;
    // Originalcode
    constexpr uintptr_t detour4_call10 = 0x004A5AA0;
uintptr_t CharSwitcher::jmp_ret5{NULL};
uintptr_t CharSwitcher::jmp_ret6{NULL};
    constexpr uintptr_t detour6_call1 = 0x007ACEE0;
uintptr_t CharSwitcher::jmp_ret7{NULL};
uintptr_t CharSwitcher::jmp_ret8{NULL};


void CharSwitcher::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x094B55, patch1, "\xEB\x19", 2); // Load Dante's Save

    } else {
        patch1.reset();
    }
}

void CharSwitcher::toggle2(bool enable) {
    if (enable)
    {
        install_patch_offset(0x3AA5D6, patch2, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 1
        install_patch_offset(0x3AA5E0, patch3, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 2
        install_patch_offset(0x3AA61E, patch4, "\x90\x90\x90\x90\x90\x90\x90\x90", 8); // Disable transitioning inertia dampener 3
    } else {
        patch2.reset();
        patch3.reset();
        patch4.reset();
    }
}

// Arc file load mk4
naked void detour2(void) {
    _asm {
			cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [0x00E552C8]
            mov ecx, [ecx] //
            mov ecx, [ecx+0x3834]
            mov ecx, [ecx+0x28]
            test cl, cl
            pop ecx
            je NeroArc
        // DanteArc:
            pushad
            push 0x00008002 
            push 0x00B90CE8 
            mov eax, [0x00E552D0]
            mov eax, [eax] //
            push 0x00EAD4A0
            call dword ptr [detour2_call1]
            popad
            jmp originalcode

        NeroArc:
            pushad
            push 0x00008002
            push 0x00B90CD0
            mov eax, [0x00E552D0]
            mov eax, [eax] //
            push 0x00EAD4A0
            call dword ptr [detour2_call1]
            popad
        originalcode:
            push ebx
            push esi
            push edi
            mov edi, [0x00E558B8]
            mov edi, [edi] //
            jmp dword ptr [CharSwitcher::jmp_ret2]
    }
}

// Get primary actor
naked void detour3(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            mov [primaryActor], eax

        originalcode:
            mov eax, [0x00E552CC]
            mov eax, [eax] //
            jmp dword ptr [CharSwitcher::jmp_ret3]
    }
}

// Spawn Secondary Actor and HUD
naked void detour4(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            pushad
            mov eax, [primaryActor]
            cmp eax, ecx
            jne handler
            mov ebp, [0x00E552CC]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x284]
            push dword ptr [ebp+0x18]
            pop [primaryHUD]
            mov ebp, [0x00E552C8]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x3834]
            mov ecx, [ebp+0x28]
            test ecx, ecx
            je NeroInit
        // DanteInit:
            // Create HUD
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov edx, [ecx]
            mov eax, [edx+0x14]
            push 0x10
            push 0x0000025C
            call eax
            mov esi, eax
            call dword ptr [detour4_call1]
            // Lock-on
            mov [secondaryHUD], eax
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov edx, [ecx]
            mov eax, [edx+0x14]
            push 0x10
            push 0x58
            call eax
            call dword ptr [detour4_call2]
            mov ecx, [secondaryHUD]
            mov [ecx+0x28], eax
            mov edx, [eax]
            mov ecx, eax
            mov eax, [edx+0x14]
            call eax
            call [detour4_call3]
            // Spawn character
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov eax, [ecx]
            mov edx, [eax+0x14]
            push 0x10
            push 0x000152F0
            call edx
            call dword ptr [detour4_call4]
            jmp Spawn

        NeroInit:
            // Create HUD
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov eax, [ecx]
            mov edx, [eax+0x14]
            push 0x10
            push 0x0000023C
            call edx
            mov esi, eax
            call dword ptr [detour4_call5]
            mov [secondaryHUD], eax
            // Lock-on
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov edx, [ecx]
            mov eax, [edx+0x14]
            push 0x10
            push 0x70
            call eax
            call dword ptr [detour4_call6]
            mov ecx, [secondaryHUD]
            mov [ecx+0x28], eax
            mov edx, [eax]
            mov ecx, eax
            mov eax, [edx+0x14]
            call eax
            call dword ptr [detour4_call7]
            //Spawn character
            mov ecx, [0x00E1434C]
            mov ecx, [ecx] //
            mov eax, [ecx]
            mov edx, [eax+0x14]
            push 0x10
            push 0x0000D680
            call edx
            mov esi, eax
            call dword ptr [detour4_call8]
        Spawn:
            mov esi, eax
            mov eax, [0x00E552CC]
            mov eax, [eax] //
            push 0x0F
            call dword ptr [detour4_call9]
            mov [secondaryActor], esi
            popad
            jmp code

        handler:
            popad
        code:
            mov ecx, [primaryActor]
        originalcode:
            mov [eax+0x24], ecx
            call dword ptr [detour4_call10]
        // jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret4]
    }                   
}

// Initial 2nd char conditions
naked void detour5(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ecx
            mov ecx, [secondaryActor]
            cmp ecx, esi
            jne handler
            push esi
            push eax
            push ebp
            mov eax, esi
            mov ebp, [0x00E558B8]
            mov ebp, [ebp] //
            mov dword ptr [eax+0x1509], 0 // input
            mov esi, [eax+0x1E8C]
            mov dword ptr [esi+0xD4], 1 // collision
        // Position
            fld dword ptr [ebp+0x60]
            fstp dword ptr [eax+0x30]
            fld dword ptr [ySpawn]
            fstp dword ptr [eax+0x34]
            fld dword ptr [ebp+0x68]
            fstp dword ptr [eax+0x38]
            mov dword ptr [eax+0x1550], 1
            mov dword ptr [esi+0x1C], 0
            xorps xmm0, xmm0
            movss [eax+0x1E1C], xmm0
            movss [eax+0xEC4], xmm0
            movss [eax+0xED0], xmm0
            movss [eax+0xED4], xmm0
            movss [eax+0xED8], xmm0
            pop ebp
            pop eax
            pop esi
            pop ecx
            jmp originalcode

        handler:
            pop ecx
        originalcode:
            pop edi
            pop esi
            pop ebx
            mov esp, ebp
        // jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret5]
    }
}

// Suspend inactive actor
naked void detour6(void) {
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            push ebp
            mov ebp, [0x00E558B8]
            mov ebp, [ebp] //
            cmp [ebp+0x24], edi
            jne handler1
            mov ebp, [ebp+0x24]
            mov byte ptr [ebp+0x150C], 1
            pop ebp
        originalcode:
            call dword ptr [detour6_call1]
            jmp jmp_ret

        handler1:
            cmp [ebp+0xB0], edi
            jne handler2
            pop ebp
            jmp originalcode

        handler2:
            pop ebp
        jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret6]
    }
}

// Prevent crash when switching to Dante during Neros SS
naked void detour7(void) {
    _asm {

            cmp byte ptr [CharSwitcher::mod_enabled], 0
			je originalcode

            cmp byte ptr [esi+0xCDF8], 00
            jne originalcode

            xor eax,eax
            jmp jmp_ret

        originalcode:
            mov eax, [edx+0x00002444]
        jmp_ret:
			jmp dword ptr [CharSwitcher::jmp_ret7]
    }
}

// Swap HUD
naked void SwapHUD(void) {
    _asm {
            pushad
            mov eax, [primaryHUD]
            mov ecx, [secondaryHUD]
            mov ebp, [0x00E552CC]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x284]
            mov [ebp+0x18], ecx
            mov [primaryHUD], ecx
            mov [secondaryHUD], eax
            popad
            ret
    }
}

naked void SaveWrite(void) {
    _asm{
            push eax
            mov eax, [sArea] //sArea
            mov eax, [eax]
            mov eax, [eax+0x3834]
            cmp byte ptr [eax+0x28], 01
            pop eax
            je NeroSave
            jmp DanteSave
        DanteSave:
            pushad
            mov eax, [sSave]
            mov eax, [eax]
            mov edx, [static_mediator_ptr]
            mov edx, [edx]
            lea esi, [eax+0xD8]
            mov ecx, 0x40
            lea edi, [edx+0x368]
            repe movsd
            popad
            ret
        NeroSave:
            pushad
            mov eax, [sSave]
            mov eax, [eax]
            mov edx, [static_mediator_ptr]
            mov edx, [edx]
            lea esi, [eax+0x1D8]
            mov ecx, 0x3A
            lea edi, [edx+0x368]
            repe movsd
            popad
            ret
    }
}

// Swap actor
naked void SwapActor(void) {
    _asm {
        // loop1:
            pushad
            mov ebp, [0x00E558B8]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x24]
            test ebp,ebp
            je loopend
            call SaveWrite
            cmp byte ptr [ebp+0xCDF8], 00
            je loop3
            mov ebp, [ebp+0xCDF8]
            mov edx, [ebp+0x22A8]
            cmp edx, 07 // DT hold
            je loopend
            cmp edx, 03
            je loopend
            jmp loop3

        loop3:
            mov ebx, [0x00E559C4]
            mov ebx, [ebx] //
            mov ecx, [ebx+0x5E4] // inputs
            xor edx, edx //
            mov dx, [desiredInput1]
            add dx, [desiredInput2]
            and ecx, edx
            cmp ecx, edx
            je loop2
            mov [prevInput], cx
            jmp loopend

        loop2:
            xor esi, esi //
            mov si, [prevInput]
            xor esi, ecx
            test esi, esi
            je loopend
            mov [prevInput], dx
            jmp swapActor

        swapActor:
            pushad
            mov eax, [primaryActor]
            mov ecx, [secondaryActor]
            mov ebp, [0x00E552C8]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x3834]
            xor dword ptr [ebp+0x28], 1
            // To-be main actor
            mov ebp, [0x00E558B8]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x24]
            mov dword ptr [ecx+0x1509], 1 // input
            // Position, rotation
            fld dword ptr [ebp+0x1350] //X pos
            fstp dword ptr [ecx+0x30]
            fld dword ptr [ebp+0x1354] // Y pos
            fstp dword ptr [ecx+0x34]
            fld dword ptr [ebp+0x1358] // Z pos
            fstp dword ptr [ecx+0x38]
            fld dword ptr [ecx+0x30]
            fstp dword ptr [ecx+0x000014A0]
            fld dword ptr [ecx+0x34]
            fstp dword ptr [ecx+0x000014A4]
            fld dword ptr [ecx+0x38]
            fstp dword ptr [ecx+0x000014A8]
            fld dword ptr [ecx+0x30]
            fstp dword ptr [ecx+0x00002170]
            fld dword ptr [ecx+0x34]
            fstp dword ptr [ecx+0x00002174]
            fld dword ptr [ecx+0x38]
            fstp dword ptr [ecx+0x00002178]
            fld [eax+0x44] // facing direction
            fstp [ecx+0x44]
            fld [eax+0x4C]
            fstp [ecx+0x4C]
            fld [eax+0x1710]
            fstp [ecx+0x1710]
            // Collision
            mov byte ptr [ecx+0x150C], 0
            mov byte ptr [ecx+0x150D], 1
            mov edi, [eax+0x1E8C]
            mov esi, [ecx+0x1E8C]
            mov dword ptr [esi+0xD4], 0
            push dword ptr [edi+0x1C]
            pop dword ptr [esi+0x1C]
            // mov dword ptr [esi+0x1C], 1
            // Inertia, gravity
            xorps xmm0, xmm0
            movss [ecx+0xEC4], xmm0
            movss xmm0, [swapGrav]
            movss [ecx+0xED4], xmm0
            // fld [eax+0xED4]
            // fstp [ecx+0xED4]
            mov byte ptr [ecx+0x2A57], 00
            fld [eax+0x1E1C] // inertia
            fstp [ecx+0x1E1C]
            fld [eax+0x1E30] // X-axis inertia
            fstp [ecx+0x1E30]
            fld [eax+0x1E38] // Z-axis inertia
            fstp [ecx+0x1E38]
            fld [eax+0x1E50]
            fstp [ecx+0x1E50]
            fld [eax+0x1E54]
            fstp [ecx+0x1E54]
            fld [eax+0x1E58]
            fstp [ecx+0x1E58]
            fld [eax+0x1E60]
            fstp [ecx+0x1E60]
            fld [eax+0x1E64]
            fstp [ecx+0x1E64]
            fld [eax+0x16C0]
            fstp [ecx+0x16C0]
            fld [eax+0x170C]
            fstp [ecx+0x170C]
            //Lockon
            push dword ptr [eax+0x3080]
            pop dword ptr [ecx+0x3080]
            //Motion
            // mov dword ptr [ecx+0x1550], 1
            // mov dword ptr [ecx+0x2008], 1
            // mov dword ptr [ecx+0x1500], 1
            
            //Previous main actor
            mov [secondaryActor], eax
            mov dword ptr [eax+0x1509], 0 // input
            mov esi, [eax+0x1E8C]
            mov dword ptr [esi+0xD4], 1 // collision
            //Position
            mov ebp, [0x00E552C8]
            mov ebp, [ebp] //
            mov ebp, [ebp+0x3830]
            // fld dword ptr [ebp+0x50]
            // fstp dword ptr [eax+0x30]
            movss xmm0, [eax+0x34]
            addss xmm0, [ySpawn]
            movss dword ptr [eax+0x34], xmm0
            // fld dword ptr [ebp+0x58]
            // fstp dword ptr [eax+0x38]
            mov dword ptr [eax+0x1550], 1
            mov dword ptr [esi+0x1C], 1
            // mov dword ptr [eax+0x1500], 0
            // mov dword ptr [eax+0x1504], 5
            // mov dword ptr [eax+0x1505], 0
            mov byte ptr [eax+0x150C], 0
            mov byte ptr [eax+0x150D], 0
            xorps xmm0, xmm0
            movss [eax+0x1E1C],xmm0
            movss [eax+0xEC4], xmm0
            movss [eax+0xED0], xmm0
            movss [eax+0xED4], xmm0
            movss [eax+0xED8], xmm0
            mov ebp, [0x00E558B8]
            mov ebp, [ebp] //
            mov [ebp+0x24], ecx
            mov [primaryActor], ecx
            call SwapHUD
            popad
        loopend:
            popad
            ret
    }
}
// Reload Dante Save
naked void detour8(void){
    _asm {
            cmp byte ptr [CharSwitcher::mod_enabled], 0
            je originalcode
            pushad
            mov eax, [sSave]
            mov eax, [eax]
            mov edx, [static_mediator_ptr]
            mov edx, [edx]
            lea esi, [eax+0xD8]
            mov ecx, 0x40
            lea edi, [edx+0x368]
            repe movsd
            popad
        originalcode:
            mov ecx, [esi+0x000000E8]
            jmp [CharSwitcher::jmp_ret8]
    }
}

std::optional<std::string> CharSwitcher::on_initialize() {
    if (!install_hook_offset(0x007580, hook2, &detour2, &jmp_ret2, 9)) { // Arc file load mk4
        spdlog::error("Failed to init CharSwitcher2 mod\n");
        return "Failed to init CharSwitcher2 mod";
    }
    if (!install_hook_offset(0x00D90E, hook3, &detour3, &jmp_ret3, 5)) { // Get primary actor
        spdlog::error("Failed to init CharSwitcher3 mod\n");
        return "Failed to init CharSwitcher3 mod";
    }
    if (!install_hook_offset(0x3A91CC, hook4, &detour4, &jmp_ret4, 8)) { // Spawn Secondary Actor and HUD
        spdlog::error("Failed to init CharSwitcher4 mod\n");
        return "Failed to init CharSwitcher4 mod";
    }
    if (!install_hook_offset(0x3A7C12, hook5, &detour5, &jmp_ret5, 5)) { // Initial 2nd char conditions
        spdlog::error("Failed to init CharSwitcher5 mod\n");
        return "Failed to init CharSwitcher5 mod";
    }
    if (!install_hook_offset(0x3A853F, hook6, &detour6, &jmp_ret6, 5)) { // Suspend inactive actor
        spdlog::error("Failed to init CharSwitcher6 mod\n");
        return "Failed to init CharSwitcher6 mod";
    }
    if (!install_hook_offset(0x42F0A6, hook7, &detour7, &jmp_ret7, 6)) { // Prevent crash when switching to Dante during Neros SS
        spdlog::error("Failed to init CharSwitcher6 mod\n");
        return "Failed to init CharSwitcher6 mod";
    }
    if (!install_hook_offset(0x00DCE0, hook8, &detour8, &jmp_ret8, 6)) { // Reload Dante Save
        spdlog::error("Failed to init CharSwitcher7 mod\n");
        return "Failed to init CharSwitcher7 mod";
    }
    return Mod::on_initialize();
}

void CharSwitcher::on_frame(fmilliseconds& dt) {
    if (mod_enabled) {
        SwapActor();
    }
}

void CharSwitcher::on_gui_frame() { 
    if (mod_enabled) {
        ImGui::Separator();
    }
    if (ImGui::Checkbox("Character Switcher", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Enable before loading into a stage");
    if (mod_enabled) {
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::BeginCombo("Input 1", devil4_sdk::getButtonInfo(desiredInput1).second)) {
            for (const auto& buttonPair : buttonPairs) {
                bool is_selected = (desiredInput1 == buttonPair.first);
                if (ImGui::Selectable(buttonPair.second, is_selected)) {
                    desiredInput1 = buttonPair.first;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        help_marker("Set a button combo to trigger the switch");
        ImGui::SameLine(sameLineWidth);
        if (ImGui::BeginCombo("Input 2", devil4_sdk::getButtonInfo(desiredInput2).second)) {
            for (const auto& buttonPair : buttonPairs) {
                bool is_selected = (desiredInput2 == buttonPair.first);
                if (ImGui::Selectable(buttonPair.second, is_selected)) {
                    desiredInput2 = buttonPair.first;
                }
                if (is_selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        // ImGui::Separator(); // uncomment if something comes after this
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::Checkbox("Inertia carryover", &inertia_enabled)) {
            toggle2(inertia_enabled);
        }
        ImGui::SameLine();
        help_marker("Enable inertia carryover on switching");
        ImGui::PopItemWidth();
    }
}

void CharSwitcher::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("char_switcher").value_or(false);
    toggle(mod_enabled);
    desiredInput1 = cfg.get<int16_t>("char_swap_input1").value_or(0x8000);
    desiredInput2 = cfg.get<int16_t>("char_swap_input2").value_or(0x1000);
}

void CharSwitcher::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("char_switcher", mod_enabled);
    cfg.set<int16_t>("char_swap_input1", desiredInput1);
    cfg.set<int16_t>("char_swap_input2", desiredInput2);
}
