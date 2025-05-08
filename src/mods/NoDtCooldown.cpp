#include "NoDtCooldown.hpp"

#if 1
bool NoDtCooldown::no_dt_cooldown_nero = false;
bool NoDtCooldown::no_dt_cooldown_dante = false;
uintptr_t NoDtCooldown::jmp_ret = NULL;

naked void detour() {
    _asm {
            cmp byte ptr [esi+0x1494], 1 // controller id nero
            je CheckNero
            cmp byte ptr [NoDtCooldown::no_dt_cooldown_dante], 1
            je RetCode
            jmp Code

        CheckNero:
            cmp byte ptr [NoDtCooldown::no_dt_cooldown_nero], 1
            je RetCode
        Code:
            movss [esi+0x00001E6C],xmm0
        RetCode:
            jmp dword ptr [NoDtCooldown::jmp_ret]
    }
}

void NoDtCooldown::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("No DT Cooldown"), &no_dt_cooldown_nero);
        ImGui::SameLine();
        help_marker(_("Disable the cooldown on leaving Devil Trigger upon entering"));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("No DT Cooldown"), &no_dt_cooldown_dante);
        ImGui::SameLine();
        help_marker(_("Disable the cooldown on leaving Devil Trigger upon entering"));
    }
}

std::optional<std::string> NoDtCooldown::on_initialize() {
    if (!install_hook_offset(0x404B84, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init NoDtCooldown\n");
		return "Failed to init NoDtCooldown";
	}

    return Mod::on_initialize();
}

void NoDtCooldown::on_config_load(const utility::Config& cfg) {
    no_dt_cooldown_nero = cfg.get<bool>("no_dt_cooldown_nero").value_or(false);
    no_dt_cooldown_dante = cfg.get<bool>("no_dt_cooldown_dante").value_or(false);
}

void NoDtCooldown::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_dt_cooldown_nero", no_dt_cooldown_nero);
    cfg.set<bool>("no_dt_cooldown_dante", no_dt_cooldown_dante);
}

#endif
