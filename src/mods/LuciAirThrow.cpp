#include "LuciAirThrow.hpp"
#include "MoveTable.hpp"

bool LuciAirThrow::mod_enabled = false;
uintptr_t  LuciAirThrow::jmp_ret1 = NULL;
    float SplashThrowAnimSpeed = 0.8f;
    float SplashThrowFrame = 33.0f;
uintptr_t  LuciAirThrow::jmp_ret2 = NULL;
    float SplashThrowWpFrame = 28.0f;
uintptr_t  LuciAirThrow::jmp_ret3 = NULL;
uintptr_t  LuciAirThrow::jmp_ret4 = NULL;

naked void detour1(void) { // Character frame skip
    _asm {
            cmp byte ptr [LuciAirThrow::mod_enabled],1
            jne originalcode
            cmp byte ptr [ebp+0x1564], 0x6D
            jne originalcode
            fld [SplashThrowAnimSpeed]
            fstp [esp+8]
            fld [SplashThrowFrame]
            jmp skipcode
        originalcode:
            fldz
        skipcode:
            mov ecx, ebp
            fstp dword ptr [esp+04]
            jmp [LuciAirThrow::jmp_ret1]
    }
}

naked void detour2(void) { // Wp frame skip
    _asm {
            cmp byte ptr [LuciAirThrow::mod_enabled],1
            jne originalcode
            cmp byte ptr [ebp+0x1564], 0x6D
            jne originalcode
            fld [SplashThrowWpFrame]
            fstp [ebx+0x348]
        originalcode:
            mov eax, [ebp+0x28A4]
            jmp [LuciAirThrow::jmp_ret2]
    }
}

naked void detour3(void) { // Modify luci unused actor flags
    _asm {
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            jne originalcode
            cmp byte ptr [ebp+0x1564], 0x6D
            jne originalcode
            mov [eax+0xEAC], 0x1B // unused actor flags
        originalcode:
            add dword ptr [ebp+0x1504], 01
            jmp [LuciAirThrow::jmp_ret3]
    }
}

naked void detour4(void) { // Wp frame skip
    _asm {
            cmp byte ptr [LuciAirThrow::mod_enabled], 1
            jne originalcode
            cmp byte ptr [ebp+0x1564], 0x6D
            jne originalcode
            mov [eax+0xEAC], 0x1B
        originalcode:
            mov edx, [ebp+00]
            fld dword ptr [esp+0x14]
            jmp [LuciAirThrow::jmp_ret4]
    }
}

std::optional<std::string> LuciAirThrow::on_initialize() {
    if (!install_hook_offset(0x3D6F2B, hook1, &detour1, &jmp_ret1, 8)) {
		spdlog::error("Failed to init LuciAirThrow mod\n");
		return "Failed to init LuciAirThrow mod";
	}
    if (!install_hook_offset(0x3D7061, hook2, &detour2, &jmp_ret2, 6)) {
		spdlog::error("Failed to init LuciAirThrow mod2\n");
		return "Failed to init LuciAirThrow mod2";
	}
    if (!install_hook_offset(0x3D70A8, hook3, &detour3, &jmp_ret3, 7)) {
		spdlog::error("Failed to init LuciAirThrow mod3\n");
		return "Failed to init LuciAirThrow mod3";
	}
    if (!install_hook_offset(0x3D7158, hook4, &detour4, &jmp_ret4, 7)) {
		spdlog::error("Failed to init LuciAirThrow mod4\n");
		return "Failed to init LuciAirThrow mod4";
	}
    return Mod::on_initialize();
}

void toggle(bool enabled) {
    kAtckDefTbl* AirThrowEntry = (kAtckDefTbl*)HookDanteKADTbl + MoveTable::extra_dante_moves - MoveTable::AirThrow;
    if (enabled) {
        AirThrowEntry->command.atckCommandNo = 0;
    }
    else {
        AirThrowEntry->command.atckCommandNo = 1;
    }
}

void LuciAirThrow::on_gui_frame(int display) {
    if (ImGui::Checkbox(_("Lucifer Air Throw"), &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker(_("Add new move mimicking V5rgil's air taunt. Mapped to forward + melee"));
}

void LuciAirThrow::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("luci_air_throw").value_or(false);
    if (mod_enabled) toggle(mod_enabled);
};

void LuciAirThrow::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("luci_air_throw", mod_enabled);
};
