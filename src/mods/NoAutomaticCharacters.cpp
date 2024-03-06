
#include "NoAutomaticCharacters.hpp"

bool NoAutomaticCharacters::mod_enabled{ false };
bool NoAutomaticCharacters::mod_enabled2{false};

uintptr_t NoAutomaticCharacters::jmp_ret{ NULL };
uintptr_t NoAutomaticCharacters::jmp_ret2{ NULL };

enum class CharPortraitNums : int {
    DANTE = 0,
    DANTE_AUTO = 1,
    DANTE_SUPER = 2,
    DANTE_SUPER_AUTO = 3,
    NERO = 4,
    NERO_AUTO = 5,
    NERO_SUPER = 6,
    NERO_SUPER_AUTO = 7,
};

naked void detour(void) {
	_asm {
            cmp byte ptr [NoAutomaticCharacters::mod_enabled2], 1
			je NoSupersOrAutos
			cmp byte ptr [NoAutomaticCharacters::mod_enabled], 1
			je NoAutos
            jmp code

        NoSupersOrAutos:
            cmp byte ptr [ebp+0000008Ch], 0 // dante
            je dante2
            cmp byte ptr [ebp+0000008Ch], 4 // nero
            je nero2
            jmp code

        NoAutos:
            cmp byte ptr [ebp+0000008Ch], 0 // dante
            je dante
            cmp byte ptr [ebp+0000008Ch], 2 // super dante
            je superdante
            cmp byte ptr [ebp+0000008Ch], 4 // nero
            je nero
            cmp byte ptr [ebp+0000008Ch], 6 // super nero
            je supernero
            jmp code

        dante:
            cmp eax, 7
            je mov6
            cmp eax, 1
            je mov2
            jmp code

        dante2:
            cmp eax, 7
            je mov4
            cmp eax, 1
            je mov4
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

        nero2:
            cmp eax, 3
            je mov0
            cmp eax, 5
            je mov0
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
            cmp byte ptr [NoAutomaticCharacters::mod_enabled2], 1
			je NoAutos
			cmp byte ptr [NoAutomaticCharacters::mod_enabled], 1
			je NoAutos
            jmp code

        NoAutos:
            cmp byte ptr [ebp+0000008Ch], 0 // nero
            je nero
            cmp byte ptr [ebp+0000008Ch], 2 // super nero
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
    if (ImGui::Checkbox("No Auto Characters", &mod_enabled)) {
        mod_enabled2 = false;
    }
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox("No Auto Or Super Characters", &mod_enabled2)) {
        mod_enabled = false;
    }
}

void NoAutomaticCharacters::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("no_automatic_characters").value_or(false);
    mod_enabled2 = cfg.get<bool>("no_automatic_or_super_characters").value_or(false);
}

void NoAutomaticCharacters::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("no_automatic_characters", mod_enabled);
    cfg.set<bool>("no_automatic_or_super_characters", mod_enabled2);
}
