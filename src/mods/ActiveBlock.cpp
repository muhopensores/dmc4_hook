#include "ActiveBlock.hpp"

bool ActiveBlock::mod_enabled = false;
uintptr_t ActiveBlock::jmp_return = NULL;
uintptr_t ActiveBlock::alt_ret = 0x007BBAC1;
float ActiveBlock::guardTimerRequirement = 5.0f;

naked void detour() {
    _asm {
        cmp byte ptr [ActiveBlock::mod_enabled], 0
        je code

        push ecx
        mov ecx, [static_mediator_ptr]
        mov ecx, [ecx]
        mov ecx, [ecx+0x24]
        cmp esi, ecx
        pop ecx
        jne code

        sub esp, 8
        movss [esp], xmm0
        movss [esp+4], xmm1

        movss xmm1, [esi+0x14d44]
        movss xmm0, [ActiveBlock::guardTimerRequirement]
        comiss xmm0, xmm1
        jb popcode

        xorps xmm0, xmm0
        comiss xmm0, xmm1
        ja popcode

        movss xmm1, [esp+4]
        movss xmm0, [esp]
        add esp, 8

        cmp dword ptr [esi+0x00014D98], 3 // royal guard
        je alt_ret
        cmp dword ptr [esi+0x00014D98], 4 // darkslayer
        je alt_ret
        jmp code

    popcode:
        movss xmm1, [esp+4]
        movss xmm0, [esp]
        add esp, 8
    code:
        cmp dword ptr [esi+0x000152A0], 00
		jmp dword ptr [ActiveBlock::jmp_return]

    alt_ret:
        jmp dword ptr [ActiveBlock::alt_ret]
    }
}

std::optional<std::string> ActiveBlock::on_initialize() {
    if (!install_hook_offset(0x3BBAAE, hook, &detour, &jmp_return, 7)) { // 007BBAAE
        spdlog::error("Failed to init ActiveBlock mod\n");
        return "Failed to init ActiveBlock mod";
    }
    return Mod::on_initialize();
}

void ActiveBlock::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Active Block"), &mod_enabled);
        ImGui::SameLine();
        help_marker(_("Block during uncancellable frames by pressing style while in Royal Guard or Darkslayer"));
    }
}

void ActiveBlock::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("active_block").value_or(false);
}

void ActiveBlock::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("active_block", mod_enabled);
}
