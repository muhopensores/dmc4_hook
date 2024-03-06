#include "TargetChangeDarkslayer.hpp"

#if 1
bool TargetChangeDarkslayer::mod_enabled{false};
uintptr_t TargetChangeDarkslayer::jmp_ret{NULL};
uintptr_t TargetChangeDarkslayer::jmp_conditional{0x007B6CF1};
uintptr_t TargetChangeDarkslayer::jmp_ret2{NULL};

// add new requirement to entering darkslayer
naked void detour(void) {
    _asm {
			cmp byte ptr [TargetChangeDarkslayer::mod_enabled], 0
			je originalcode

			test dword ptr [esi+0x00001410], 0x00010000
            jne jmp_conditional

		originalcode:
            test dword ptr [esi+0x00001410], 0x00002000
			jmp dword ptr [TargetChangeDarkslayer::jmp_ret]

        jmp_conditional:
            jmp dword ptr [TargetChangeDarkslayer::jmp_conditional]
    }
}

// disable target switch if dante
naked void detour2(void) {
    _asm {
			cmp byte ptr [TargetChangeDarkslayer::mod_enabled], 0
			je originalcode

            cmp byte ptr [edi+0x00001494], 0 // dante controller id
            jne originalcode

            cmp byte ptr [TargetChangeDarkslayer::mod_enabled], 1 // force conditional
            jmp jmp_return

		originalcode:
            test byte ptr [edi+0x00001410], 0x20
        jmp_return:
			jmp dword ptr [TargetChangeDarkslayer::jmp_ret2]
    }
}

std::optional<std::string> TargetChangeDarkslayer::on_initialize() {
    if (!install_hook_offset(0x003B6CC6, hook, &detour, &jmp_ret, 10)) {
        spdlog::error("Failed to init TargetChangeDarkslayer mod\n");
        return "Failed to init TargetChangeDarkslayer mod";
    }

    if (!install_hook_offset(0x4208B4, hook2, &detour2, &jmp_ret2, 7)) {
        spdlog::error("Failed to init TargetChangeDarkslayer mod2\n");
        return "Failed to init TargetChangeDarkslayer mod2";
    }
    
    return Mod::on_initialize();
}

void TargetChangeDarkslayer::on_gui_frame() {
    ImGui::Checkbox("Target Change Darkslayer", &mod_enabled);
    ImGui::SameLine();
    help_marker("Darkslayer is accessed by pressing the target change button");
}

void TargetChangeDarkslayer::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("target_change_darkslayer").value_or(false);
}

void TargetChangeDarkslayer::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("target_change_darkslayer", mod_enabled);
}
#endif
