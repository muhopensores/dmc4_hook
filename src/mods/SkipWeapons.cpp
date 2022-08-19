
#include "SkipWeapons.hpp"

#if 1
static bool hooked{ false };

bool SkipWeapons::skip_pandora{ false };
bool SkipWeapons::skip_lucifer{ false };
uintptr_t SkipWeapons::skip_dante_gun_continue{ NULL };
uintptr_t SkipWeapons::skip_dante_sword_continue{ NULL };

naked void skip_dante_gun_proc(void) {
	_asm {
		cmp byte ptr [SkipWeapons::skip_pandora], 1
		jne originalcode
		cmp eax, 8
		je pandora
	originalcode:
		mov [edi+30h], eax
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_gun_continue]

	pandora:
		mov [edi+30h], 9
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_gun_continue]
	}
}

naked void skip_dante_sword_proc(void) {
    _asm {
		cmp byte ptr [SkipWeapons::skip_lucifer], 1
		jne originalcode
		cmp eax, 6
		je rebellion
	originalcode:
		mov [edi+2Ch], eax
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_sword_continue]

	rebellion:
		mov [edi+2Ch], 4
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_sword_continue]
    }
}

std::optional<std::string> SkipWeapons::on_initialize() {
	// bad fix for crashing on securom exe
	const unsigned char bytes[5] = { 0x89, 0x47, 0x30, 0xB0, 0x01 };
	if (memcmp((void*)0x7B122F, bytes, sizeof(bytes))) { // TODO(): might be an error dont remember
		hooked = false;
		return Mod::on_initialize();
	}
	hooked = true;
	if (!install_hook_offset(0x3B122F, hook, &skip_dante_gun_proc, &SkipWeapons::skip_dante_gun_continue, 5)) { // breaks 28mb exe
		spdlog::error("Failed to init SkipPandora mod\n");
		return "Failed to init SkipPandora mod";
	}
	if (!install_hook_offset(0x3B126F, hook2, &skip_dante_sword_proc, &SkipWeapons::skip_dante_sword_continue, 5)) { // breaks 28mb exe
		spdlog::error("Failed to init SkipLucifer mod\n");
		return "Failed to init SkipLucifer mod";
	}

	return Mod::on_initialize();
}

void SkipWeapons::on_gui_frame() {
	if (!hooked) { return; }
	ImGui::Checkbox("Skip Pandora", &skip_pandora);
    ImGui::SameLine(205);
	ImGui::Checkbox("Skip Lucifer", &skip_lucifer);
}

void SkipWeapons::on_config_load(const utility::Config& cfg) {
    skip_pandora = cfg.get<bool>("skip_pandora").value_or(false);
	skip_lucifer = cfg.get<bool>("skip_lucifer").value_or(false);
}

void SkipWeapons::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("skip_pandora", skip_pandora);
    cfg.set<bool>("skip_lucifer", skip_lucifer);
}

#endif
