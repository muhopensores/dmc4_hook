#include "PinTrick.hpp"

bool PinTrick::mod_enabled = false;
static bool PinTrickFlag = false;
uintptr_t PinPtr = NULL;

uintptr_t PinTrick::jmp_ret1 = NULL;
uintptr_t PinTrick::jmp_ret2 = NULL;
uintptr_t PinTrick::jmp_ret3 = NULL;
    constexpr uintptr_t detour3_call1 = 0x007AD570;
    constexpr uintptr_t detour3_call2 = 0x00458210;
    constexpr float Y_adjustment = 200.0f;
    constexpr float TurnRate = 0.35f;
    constexpr uintptr_t detour3_jmp = 0x007CAFC3;
uintptr_t PinTrick::jmp_ret4 = NULL;
    constexpr uintptr_t detour4_jmp = 0x7CB011;
uintptr_t PinTrick::jmp_ret5 = NULL;
    constexpr float DefaultTrickRange = 500.0f;
    float TrickRange = 5000.0f;
uintptr_t PinTrick::jmp_ret6 = NULL;

bool __stdcall CheckValidPin(uPlayer *player) {
    if (!player->luciferPins[0])
        return false;
    for (int i = 15 - 1; i >= 0; i--) {
        if (player->luciferPins[i])
            if (!player->luciferPins[i]->PinnedTarget) {
                PinPtr = (uintptr_t)player->luciferPins[i];
                return true;
            }
    }
    return false;
}

naked void detour1() { //ground trickster+back
    _asm {
            cmp byte ptr [PinTrick::mod_enabled],1
            jne handler1
            cmp byte ptr [esi+0x21E0], 01
            jne handler1

            pushad
            push esi
            call CheckValidPin
            test al,1
            je handler2
            popad

            push 0x26
            mov byte ptr [PinTrickFlag],01

        originalcode:
            mov ecx,esi
            mov [esi+0x1564],ebp
            jmp [PinTrick::jmp_ret1]
        handler1:
            push 0x24
            jmp originalcode
        handler2:
            popad
            jmp handler1
    }
}

naked void detour2() { //air trickster+back
    _asm {
            cmp byte ptr [PinTrick::mod_enabled],1
            jne handler1
            cmp byte ptr [esi+0x21E0], 01
            jne handler1

            pushad
            push esi
            call CheckValidPin
            test al,1
            je handler2
            popad

            push 0x26
            mov byte ptr [PinTrickFlag],01

        originalcode:
            mov [esi+0x00014D4C],eax
            jmp [PinTrick::jmp_ret2]
        handler1:
            push 0x25
            jmp originalcode
        handler2:
            popad
            jmp handler1
    }
}

naked void detour3() {
    _asm {
            cmp byte ptr [PinTrick::mod_enabled],1
            jne originalcode

            cmp byte ptr [PinTrickFlag],01
            jne originalcode

            pushad
            mov eax,[PinPtr]
            lea ecx,[esi+0x14D60]
            fld [eax+0x30]
            fstp [ecx]
            fld [eax+0x34]
            fstp [ecx+0x4]
            fld [eax+0x38]
            fstp [ecx+0x8]

            push ecx
            mov eax,esi
            fld [TurnRate]
            fstp [esp]

            call detour3_call1
            lea eax,[esi+0x1704]
            movaps xmm3,xmm0
            call detour3_call2

            mov eax,[PinPtr]
            fld [esi+0x30]
            fstp [eax+0x30]
            movss xmm6,[esi+0x34]
            addss xmm6,Y_adjustment
            movss [eax+0x34],xmm6
            //fstp [eax+0x34]
            fld [esi+0x38]
            fstp [eax+0x38]
            popad
            
            mov byte ptr  [esi+0x1766],1
            sub [esi+0x14D4C],1 //prevent consuming sky star use
            jmp detour3_jmp
        originalcode:
            mov eax,[esi]
            mov edx,[eax+0x2A8]
            jmp [PinTrick::jmp_ret3]
    }
}

naked void detour4() {
    _asm {
            mov ebx,00000001
            cmp byte ptr [PinTrick::mod_enabled],1
            jne originalcode

            cmp byte ptr [PinTrickFlag],01
            jne originalcode

            jmp detour4_jmp

            originalcode:
            jmp [PinTrick::jmp_ret4]
    }
}

naked void detour5() {
    _asm {
            movss xmm1,[DefaultTrickRange] // doesn't screw inf trick range cheat
            cmp byte ptr [PinTrick::mod_enabled],1
            jne originalcode

            cmp byte ptr [PinTrickFlag],01
            jne originalcode

            mov byte ptr [PinTrickFlag],0
            movss xmm1,[TrickRange]
        originalcode:
            jmp [PinTrick::jmp_ret5]
    }
}


std::optional<std::string> PinTrick::on_initialize() {
    if (!install_hook_offset(0x3B746F, hook1, &detour1, &jmp_ret1, 10))
    {
        spdlog::error("Failed to init PinTrick mod1\n");
        return "Failed to init PinTrick mod1";
    }
    if (!install_hook_offset(0x3B74AF, hook2, &detour2, &jmp_ret2, 8))
    {
        spdlog::error("Failed to init PinTrick mod2\n");
        return "Failed to init PinTrick mod2";
    }
    if (!install_hook_offset(0x3CAF65, hook3, &detour3, &jmp_ret3, 8))
    {
        spdlog::error("Failed to init PinTrick mod3\n");
        return "Failed to init PinTrick mod3";
    }
    if (!install_hook_offset(0x3CB000, hook4, &detour4, &jmp_ret4, 5))
    {
        spdlog::error("Failed to init PinTrick mod4\n");
        return "Failed to init PinTrick mod4";
    }
    if (!install_hook_offset(0x3CB094, hook5, &detour5, &jmp_ret5, 8))
    {
        spdlog::error("Failed to init PinTrick mod5\n");
        return "Failed to init PinTrick mod5";
    }
    return Mod::on_initialize();
}

void PinTrick::on_gui_frame(int display) {
    ImGui::Checkbox(_("Pin Trick"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Trick to the earliest valid pin with trickster back+style"));
}

void PinTrick::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("pin_trick").value_or(false);
};

void PinTrick::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("pin_trick", mod_enabled);
};
