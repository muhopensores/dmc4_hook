#include "InfDreadnaught.hpp"

#if 1
bool InfDreadnaught::mod_enabled{ false };
bool InfDreadnaught::mod_enabled2{false};
uintptr_t InfDreadnaught::inf_dreadnaught_continue{ NULL };

naked void inf_dreadnaught_proc(void) {
    _asm {
		cmp byte ptr [InfDreadnaught::mod_enabled], 0
        je code

        cmp byte ptr [edi+0x152A0], 0
        je mov1
        jmp code

    mov1:
        mov byte ptr [edi+0x152A0], 1
        jmp code

    code:
        mov eax, [edi+0x152A0]
		jmp dword ptr [InfDreadnaught::inf_dreadnaught_continue]
    }
}

std::optional<std::string> InfDreadnaught::on_initialize() {
    if (!install_hook_offset(0x3BDB76, hook, &inf_dreadnaught_proc, &InfDreadnaught::inf_dreadnaught_continue, 6)) {
        spdlog::error("Failed to init InfDreadnaught mod\n");
        return "Failed to init InfDreadnaught mod";
    }

    return Mod::on_initialize();
}

void InfDreadnaught::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x3BDCE2, patch, "\x90\x90\x90\x90\x90\x90", 6);
    }
    else {
        patch.reset();
    }
}

void InfDreadnaught::toggle2(bool enable) {
    if (enable) {
        install_patch_offset(0x3BA568, patch2, "\xEB\x5A", 2);
    } else {
        patch2.reset();
    }
}

void InfDreadnaught::on_gui_frame() {
    if (ImGui::Checkbox(_("Infinite Dreadnaught"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Run in Dreadnaught"), &mod_enabled2)) {
        toggle2(mod_enabled2);
    }
}

void InfDreadnaught::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("inf_dreadnaught").value_or(false);
    toggle(mod_enabled);
    mod_enabled2 = cfg.get<bool>("run_in_dreadnaught").value_or(false);
    toggle2(mod_enabled2);

}

void InfDreadnaught::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("inf_dreadnaught", mod_enabled);
    cfg.set<bool>("run_in_dreadnaught", mod_enabled2);
}

#endif
