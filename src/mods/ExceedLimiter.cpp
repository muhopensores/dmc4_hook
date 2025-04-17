#include "ExceedLimiter.hpp"

#if 1
bool      ExceedLimiter::mod_enabled   = false;
uintptr_t ExceedLimiter::jmp_ret1      = NULL;

naked void detour1(void) { // 
    _asm {
        cmp byte ptr [ExceedLimiter::mod_enabled], 1
        jne originalcode

        cmp dword ptr [edi+0x1444], 0 // exceed pressed
        je originalcode
        mov edx, 1
        cmp dword ptr [edi+0x1564], 38
        je exStreak
        mov word ptr [edi+0x176C], 1
        jmp originalcode
        exStreak:
        mov word ptr [edi+0x176C], 0

        originalcode:
        mov eax, [ecx+0x04]
        cmp eax, 3
        jmp dword ptr [ExceedLimiter::jmp_ret1]
    }
}

void ExceedLimiter::on_gui_frame(int display) {
    ImGui::Checkbox(_("Exceed Limiter"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Attacks only use one bar of exceed if holding the exceed button"));
}

// void ExceedLimiter::on_frame(fmilliseconds& dt) {}

std::optional<std::string> ExceedLimiter::on_initialize() {
    if (!install_hook_offset(0x3EA490, hook1, &detour1, &jmp_ret1, 6)) {
		spdlog::error("Failed to init ExceedLimiter mod\n");
		return "Failed to init ExceedLimiter mod";
	}
    return Mod::on_initialize();
}


void ExceedLimiter::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("exceed_limiter", mod_enabled);
}

void ExceedLimiter::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("exceed_limiter").value_or(false);
}
#endif
