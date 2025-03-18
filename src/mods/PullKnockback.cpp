#include "PullKnockback.hpp"

uintptr_t PullKnockback::jmp_ret1{ NULL };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;
constexpr uintptr_t detour1_jmp = 0x4A9D47;

naked void detour1(void) {//Knock enemies towards player if kAttackStatus+0x50(mBlownAngleType) > 1 
    _asm {
            je originalcode
            //cmp eax,2
            //jne originalcode
            pushad
            mov eax,[static_mediator_ptr]
            mov eax,[eax]
            mov eax,[eax+0x24]
            mov ecx,eax
            movss xmm3,[edi+0xD0]
            movss xmm4,[edi+0xD4]
            movss xmm0,[ecx+0xD0]
            movss xmm1,[ecx+0xD4]
            movss xmm5,[edi+0xD8]
            movss xmm2,[ecx+0xD8]
            popad
            jmp detour1_jmp
        originalcode:
            jmp [PullKnockback::jmp_ret1]
    }
}

std::optional<std::string> PullKnockback::on_initialize() {
    if (!install_hook_offset(0xA9C2C, hook1, &detour1, &jmp_ret1, 6)) {
        spdlog::error("Failed to init PullKnockback mod1\n");
        return "Failed to init PullKnockback mod1";
    }

    return Mod::on_initialize();
}