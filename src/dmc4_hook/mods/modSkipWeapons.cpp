#include "../mods.h"
#include "modSkipWeapons.hpp"

#if 1
static bool hooked{ false };

bool SkipWeapons::skipPandora{ false };
bool SkipWeapons::skipLucifer{ false };
uintptr_t SkipWeapons::_skipDanteGunContinue{ NULL };
uintptr_t SkipWeapons::_skipDanteSwordContinue{ NULL };

naked void skipDanteGun_proc(void) {
	_asm {
		cmp byte ptr [SkipWeapons::skipPandora], 1
		jne originalcode
		cmp eax, 8
		je pandora
	originalcode:
		mov [edi+30h], eax
		mov al, 01
		jmp dword ptr [SkipWeapons::_skipDanteGunContinue]

	pandora:
		mov [edi+30h], 9
		mov al, 01
		jmp dword ptr [SkipWeapons::_skipDanteGunContinue]
	}
}

naked void skipDanteSword_proc(void) {
    _asm {
		cmp byte ptr [SkipWeapons::skipLucifer], 1
		jne originalcode
		cmp eax, 6
		je rebellion
	originalcode:
		mov [edi+2Ch], eax
		mov al, 01
		jmp dword ptr [SkipWeapons::_skipDanteSwordContinue]

	rebellion:
		mov [edi+2Ch], 4
		mov al, 01
		jmp dword ptr [SkipWeapons::_skipDanteSwordContinue]
    }
}

std::optional<std::string> SkipWeapons::onInitialize() {
	// bad fix for crashing on securom exe
	const unsigned char bytes[5] = { 0x89, 0x47, 0x30, 0xB0, 0x01 };
	if (memcmp((void*)0x7B122F, bytes, sizeof(bytes))) {
		hooked = false;
		return Mod::onInitialize();
	}
	hooked = true;
	if (!install_hook_offset(0x3B122F, hook, &skipDanteGun_proc, &SkipWeapons::_skipDanteGunContinue, 5)) { // breaks 28mb exe
		HL_LOG_ERR("Failed to init SkipPandora mod\n");
		return "Failed to init SkipPandora mod";
	}
	if (!install_hook_offset(0x3B126F, hook, &skipDanteSword_proc, &SkipWeapons::_skipDanteSwordContinue, 5)) { // breaks 28mb exe
		HL_LOG_ERR("Failed to init SkipLucifer mod\n");
		return "Failed to init SkipLucifer mod";
	}

	return Mod::onInitialize();
}

void SkipWeapons::onGUIframe() {
	if (!hooked) { return; }
	ImGui::Checkbox("Skip Pandora", &skipPandora);
    ImGui::SameLine(205);
	ImGui::Checkbox("Skip Lucifer", &skipLucifer);
}

void SkipWeapons::onConfigLoad(const utils::Config& cfg) {
    skipPandora = cfg.get<bool>("skip_pandora").value_or(false);
	skipLucifer = cfg.get<bool>("skip_lucifer").value_or(false);
}

void SkipWeapons::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("skip_pandora", skipPandora);
    cfg.set<bool>("skip_lucifer", skipLucifer);
}

#endif
