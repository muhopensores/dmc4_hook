#include "DtKnuckle.hpp"

bool DtKnuckle::mod_enabled{false};
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

uintptr_t DtKnuckle::jmp_ret1{NULL};
int moveID                        = 0x31C;
int inputCheck                    = 0;
bool spectreFlag                  = 0;
constexpr uintptr_t detour1_call1 = 0x00829BE0;

uintptr_t DtKnuckle::jmp_ret2{NULL};

uintptr_t DtKnuckle::jmp_ret3{NULL};
constexpr uintptr_t detour3_conditional1 = 0x0082A9D5;

uintptr_t DtKnuckle::jmp_ret4{NULL};
constexpr uintptr_t detour4_conditional1 = 0x00829671;

void DtKnuckle::toggle(bool enable) {
    if (enable) {
        // dt check nop 1, this nops less than necessary so look here if funky
        install_patch_offset(0x42A84F, patch1, "\x90\x90\x90\x90\x90\x90", 6);

        // dt check nop 2 I assume
        install_patch_offset(0x42A8AC, patch2, "\x90\x90\x90\x90\x90\x90", 6);

        // DT appearance bypass
        install_patch_offset(0x42908B, patch3, "\xEB\x08", 2);
        install_patch_offset(0x4290D0, patch4, "\xEB\x08", 2);
        install_patch_offset(0x428FD9, patch5, "\xEB\x08", 2);
        install_patch_offset(0x428EEE, patch6, "\xEB\x08", 2);
        install_patch_offset(0x428E8B, patch7, "\xEB\x08", 2);
    } else {
        patch1.reset();
        patch2.reset();
        patch3.reset();
        patch4.reset();
        patch5.reset();
        patch6.reset();
        patch7.reset();
    }
}

naked void detour1(void) { // Input check and initialize spectre
    _asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

            pushad
            mov ebp,[static_mediator_ptr]
            mov ebp,[ebp]
            mov ebp,[ebp+0x24]
            cmp dword ptr [ebp],0xBE4FA0 // is Nero ?
            jne handler

            mov eax,[ebp+0x1374]

            test eax,eax
            je handler

            mov edx,[inputCheck]
            test edx,edx
            jne handler2

            and eax,0x0100 // just use test @todosiyan
            cmp eax,0x0100 // Input check, default L1
            jne handler

            mov dword ptr [inputCheck],0x10
            jmp forwardCheck

        code:
            mov [moveID],0x0000031C
        originalcode:
            lea edi,[esp+0x14]
            mov ecx,esi
            jmp jmp_ret

        SpawnSpectre:
            mov edi,[ebp+0xCDF8]
            mov eax,[edi+0x22A8]
            cmp eax,0x7
            je handler
            mov eax,0x00000004
            mov esi,[moveID]
            mov [esp+0x0C],esi
            mov [esp+0x10],0x04
            movss [esp+0x14],xmm0
            mov byte ptr [spectreFlag],0x01
            call detour1_call1
            mov eax,[ebp+0x0000CDF8]
            mov ebx,[eax+0x1370]
            mov [ebx+0x14F0],0x0C
            movq xmm0, qword ptr [esp+0x0C]
            mov ecx,[esp+0x14]
            add eax,0x000022B4
            movq qword ptr [eax],xmm0 // added qword ptr
            mov [eax+0x08],ecx
            popad
            jmp code
            forwardCheck:
            mov eax,[ebp+0x21CC]
            cmp al,01
            jne backCheck
            mov dword ptr [moveID],0x317
            jmp SpawnSpectre
            backCheck:
            mov eax,[ebp+0x21E0]
            cmp al,01
            jne SpawnSpectre
            mov dword ptr [moveID],0x330
            jmp SpawnSpectre

        handler:
            popad
            jmp code

        handler2:
            dec edx
            mov [inputCheck],edx
            jmp handler

        jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret1]
    }
}

naked void detour2(void) { // better flag remove
    _asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

            mov byte ptr [spectreFlag],00
        originalcode:
            mov dword ptr [esi+0x000022C4],0x28
			jmp dword ptr [DtKnuckle::jmp_ret2]
    }
}

naked void detour3(void) { // animation check bypass
    _asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

            cmp byte ptr [spectreFlag],01
            jne originalcode
            mov al,01
        originalcode:
            test al,al
            je jmp_je
        // jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret3]
        jmp_je:
            jmp dword ptr [detour3_conditional1]
    }
}

naked void detour4(void) { // motion check bypass
    _asm {
			cmp byte ptr [DtKnuckle::mod_enabled], 0
			je originalcode

            cmp byte ptr [spectreFlag],01
            jne originalcode
            mov al,01
        originalcode:
            test al,al
            jne jmp_jne
            cmp dword ptr [esi+0x000022A8],00
        // jmp_ret:
			jmp dword ptr [DtKnuckle::jmp_ret4]
        jmp_jne:
            jmp dword ptr [detour4_conditional1]
    }
}

std::optional<std::string> DtKnuckle::on_initialize() {
    if (!install_hook_offset(0x3A92BF, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init DtKnuckle mod\n");
        return "Failed to init DtKnuckle mod";
    }
    if (!install_hook_offset(0x429739, hook2, &detour2, &jmp_ret2, 10)) {
        spdlog::error("Failed to init DtKnuckle mod2\n");
        return "Failed to init DtKnuckle mod2";
    }
    if (!install_hook_offset(0x42A841, hook3, &detour3, &jmp_ret3, 8)) {
        spdlog::error("Failed to init DtKnuckle mod3\n");
        return "Failed to init DtKnuckle mod3";
    }
    if (!install_hook_offset(0x429650, hook4, &detour4, &jmp_ret4, 11)) {
        spdlog::error("Failed to init DtKnuckle mod4\n");
        return "Failed to init DtKnuckle mod4";
    }
    return Mod::on_initialize();
}

void DtKnuckle::on_gui_frame() {
    if (ImGui::Checkbox("DT Knuckle", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Triggers a stand attack when the player presses a keybind (Default L1)");
}

void DtKnuckle::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("dt_knuckle").value_or(false);
    toggle(mod_enabled);
}

void DtKnuckle::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("dt_knuckle", mod_enabled);
}
