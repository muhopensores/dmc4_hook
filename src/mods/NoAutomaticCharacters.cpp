
#include "NoAutomaticCharacters.hpp"

bool NoAutomaticCharacters::mod_enabled{ false };
uintptr_t NoAutomaticCharacters::jmp_ret{ NULL };
uintptr_t NoAutomaticCharacters::jmp_ret2{ NULL };

naked void detour(void) {
	_asm {
			cmp byte ptr [NoAutomaticCharacters::mod_enabled], 0
			je code

            cmp [ebp+0000008Ch], 0 // dante
            je dante
            cmp [ebp+0000008Ch], 2 // super dante
            je superdante
            cmp [ebp+0000008Ch], 4 // nero
            je nero
            cmp [ebp+0000008Ch], 6 // super nero
            je supernero
            jmp code

        dante:
            cmp eax, 7
            je mov6
            cmp eax, 1
            je mov2
            jmp code

        superdante:
            cmp eax, 1
            je mov0
            cmp eax, 3
            je mov4
            jmp code

        nero:
            cmp eax, 3
            je mov2
            cmp eax, 5
            je mov6
            jmp code

        supernero:
            cmp eax, 7
            je mov0
            cmp eax, 5
            je mov4
            jmp code

        mov0:
            mov eax, 0
            jmp code
        mov2:
            mov eax, 2
            jmp code
        mov4:
            mov eax, 4
            jmp code
        mov6:
            mov eax, 6
            jmp code

			code:
			mov [ebp+0000008Ch], eax
			jmp dword ptr [NoAutomaticCharacters::jmp_ret]
	}
}

naked void detour2(void) {
    _asm {
			cmp byte ptr [NoAutomaticCharacters::mod_enabled], 0
			je code

            cmp [ebp+0000008Ch], 0 // nero
            je nero
            cmp [ebp+0000008Ch], 2 // super nero
            je supernero
            jmp code

        nero:
            cmp eax,3
            je mov2
            cmp eax,1
            je mov2
            jmp code

        supernero:
            cmp eax,1
            je mov0
            cmp eax,3
            je mov0
            jmp code

        mov0:
            mov eax, 0
            jmp code
        mov2:
            mov eax, 2
            jmp code

			code:
			mov [ebp+0000008Ch], eax
			jmp dword ptr [NoAutomaticCharacters::jmp_ret2]
    }
}

std::optional<std::string> NoAutomaticCharacters::on_initialize() {
	if (!install_hook_offset(0x0378E75, hook, &detour, &jmp_ret, 6)){
		spdlog::error("Failed to init NoAutomaticCharacters mod\n");
		return "Failed to init NoAutomaticCharacters mod";
	}
    if (!install_hook_offset(0x0379082, hook2, &detour2, &jmp_ret2, 6)) {
        spdlog::error("Failed to init NoAutomaticCharacters2 mod\n");
        return "Failed to init NoAutomaticCharacters2 mod";
    }
	return Mod::on_initialize();
}

void NoAutomaticCharacters::on_gui_frame() {
    ImGui::Checkbox("No Auto Characters", &mod_enabled);
}

void NoAutomaticCharacters::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("no_automatic_characters").value_or(false);
}

void NoAutomaticCharacters::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_automatic_characters", mod_enabled);
}
