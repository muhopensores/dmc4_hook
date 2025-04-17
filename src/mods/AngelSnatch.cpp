#include "AngelSnatch.hpp"

bool AngelSnatch::mod_enabled{false};
bool snatchFlag = false;

uintptr_t AngelSnatch::jmp_ret1{false};
    constexpr uintptr_t detour1_call1 = 0x007FA190;
uintptr_t AngelSnatch::jmp_ret2{false};
    constexpr uintptr_t detour2_call1 = 0x007F9FA0;
uintptr_t AngelSnatch::jmp_ret3{false};
uintptr_t AngelSnatch::jmp_ret4{false};
uintptr_t AngelSnatch::jmp_ret5{false};
uintptr_t AngelSnatch::jmp_ret6{false};

// Air Angel Snatch flag
//naked void detour1(void) {
//    _asm {
//			cmp byte ptr [AngelSnatch::mod_enabled], 0
//			je originalcode
//
//            push ecx
//            mov ecx, [eax+0x21CC]
//            test cl, cl
//            je handle
//            mov byte ptr [snatchFlag], 1
//            pop ecx
//            jmp originalcode
//
//        handle:
//            mov byte ptr [snatchFlag], 0
//            pop ecx
//		originalcode:
//            push eax
//            call detour1_call1
//        // jmp_ret:
//			jmp dword ptr [AngelSnatch::jmp_ret1]
//    }
//}
//
//// Ground Angel Snatch flag
//naked void detour2(void) {
//    _asm {
//			cmp byte ptr [AngelSnatch::mod_enabled], 0
//			je originalcode
//
//            push ecx
//            mov ecx, [eax+0x21CC]
//            test cl, cl
//            je handle
//            mov byte ptr [snatchFlag], 1
//            pop ecx
//            jmp originalcode
//
//        handle:
//            mov byte ptr [snatchFlag], 0
//            pop ecx
//		originalcode:
//            push eax
//            call detour2_call1
//        // jmp_ret:
//			jmp dword ptr [AngelSnatch::jmp_ret2]
//    }
//}

//Alternate trigger straight from the input check
naked void detour1(void) {
    _asm {
            cmp byte ptr [esi+0x21CC],1
            jne originalcode

            mov byte ptr [snatchFlag],1

        originalcode:
            mov edx,[esi]
            mov eax,[ebp+0x1C]
            jmp dword ptr [AngelSnatch::jmp_ret1]
    }
}

// Invalidate enemy pull, change snatch reaction
naked void detour3(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push eax
            mov al, [snatchFlag]
            test al,al
            je handle
            pop eax
            pop ecx
            pop ebx
            pop eax
            mov eax, 0x4
            mov byte ptr [snatchFlag], 0
            jmp code

        handle:
            pop eax
        originalcode:
            call edx
        code:
            mov edi,eax
            cmp edi, 3
        // jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret3]
    }
}

// Angel Snatch on Dante
naked void detour4(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push eax
            xor eax, eax // for al
            mov al, [snatchFlag]
            test eax, eax
            je handler
            pop eax
            mov edi, 0x00000004
            jmp jmp_ret
        handler:
            pop eax
        originalcode:
            mov edi, 0x00000003
        jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret4]
    }
}

// Change Dante's reaction to Angel Snatch
naked void detour5(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push eax
            xor eax, eax // for al
            mov al, [snatchFlag]
            test eax, eax
            je handler
            pop eax
            push 0x32
            jmp skipcode

        handler:
            pop eax
            
        originalcode:
            push 0x36
        skipcode:
            mov ecx,esi
            mov [esi+0x0001616C], ebx
        // jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret5]
    }
}

// Enable Dante's AI during Angel Snatch
naked void detour6(void) {
    _asm {
			cmp byte ptr [AngelSnatch::mod_enabled], 0
			je originalcode

            push eax
            xor eax, eax // for al
            mov al, [snatchFlag]
            test eax, eax
            je handler
            pop eax
            mov dword ptr [esi+0x000160B4], 01
            jmp jmp_ret

        handler:
            pop eax
        originalcode:
            mov [esi+0x000160B4], ebx
        jmp_ret:
			jmp dword ptr [AngelSnatch::jmp_ret6]
    }
}

std::optional<std::string> AngelSnatch::on_initialize() {
    //if (!install_hook_offset(0x3F9CF2, hook1, &detour1, &jmp_ret1, 6)) {
    //    spdlog::error("Failed to init AngelSnatch mod\n");
    //    return "Failed to init AngelSnatch mod";
    //}
    if (!install_hook_offset(0x3E5BD5, hook1, &detour1, &jmp_ret1, 5)) { //Alternate angel snatch trigger at input check
        spdlog::error("Failed to init AngelSnatch mod\n");
        return "Failed to init AngelSnatch mod";
    }
    //if (!install_hook_offset(0x3F9CEB, hook2, &detour2, &jmp_ret2, 6)) {
    //    spdlog::error("Failed to init AngelSnatch mod2\n");
    //    return "Failed to init AngelSnatch mod2";
    //}
    if (!install_hook_offset(0x334F9D, hook3, &detour3, &jmp_ret3, 7)) {
        spdlog::error("Failed to init AngelSnatch mod3\n");
        return "Failed to init AngelSnatch mod3";
    }
    if (!install_hook_offset(0x3C5D98, hook4, &detour4, &jmp_ret4, 5)) {
        spdlog::error("Failed to init AngelSnatch mod4\n");
        return "Failed to init AngelSnatch mod4";
    }
    if (!install_hook_offset(0x3C5D33, hook5, &detour5, &jmp_ret5, 10)) {
        spdlog::error("Failed to init AngelSnatch mod5\n");
        return "Failed to init AngelSnatch mod5";
    }
    if (!install_hook_offset(0x3C5D65, hook6, &detour6, &jmp_ret6, 6)) {
        spdlog::error("Failed to init AngelSnatch mod6\n");
        return "Failed to init AngelSnatch mod6";
    }
    return Mod::on_initialize();
}

void AngelSnatch::on_gui_frame(int display) {
    ImGui::Checkbox(_("Angel Snatch"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Mapped to forward + snatch"));
}

void AngelSnatch::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("angel_snatch").value_or(false);
}

void AngelSnatch::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("angel_snatch", mod_enabled);
}
