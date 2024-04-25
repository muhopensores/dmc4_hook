#include "ActiveBlock.hpp"
#include "InputStates.hpp"

bool ActiveBlock::mod_enabled{ false };
uintptr_t ActiveBlock::jmp_return{ NULL };
uintptr_t ActiveBlock::alt_ret{ 0x007BBAC1 };
constexpr uintptr_t static_mediator_ptr = 0x00E558B8;

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

        cmp dword ptr [InputStates::input_timer], 0x40c00000 // 6.0f
        ja code
        cmp dword ptr [InputStates::input_timer], 0x00000000
        je code
        // cmp dword ptr [esi+0x00014D98], 3 // [currentStyle], 3
        // jne code
        jmp dword ptr [ActiveBlock::alt_ret]

    code:
        cmp dword ptr [esi+0x000152A0], 00
		jmp dword ptr [ActiveBlock::jmp_return]
    }
}

std::optional<std::string> ActiveBlock::on_initialize() {
    if (!install_hook_offset(0x3BBAAE, hook, &detour, &jmp_return, 7)) { // 007BBAAE
        spdlog::error("Failed to init ActiveBlock mod\n");
        return "Failed to init ActiveBlock mod";
    }
    return Mod::on_initialize();
}

void ActiveBlock::on_gui_frame() {
    ImGui::Checkbox(_("Active Block"), &mod_enabled);
    ImGui::SameLine();
    help_marker("Block during uncancellable frames");
}

void ActiveBlock::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("active_block").value_or(false);
}

void ActiveBlock::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("active_block", mod_enabled);
}
