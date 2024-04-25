
#include "SkipWeapons.hpp"

#if 1
static bool hooked{ false };

bool SkipWeapons::skip_shotgun{ false };
bool SkipWeapons::skip_pandora{ false };
bool SkipWeapons::skip_gilgamesh{false};
bool SkipWeapons::skip_lucifer{ false };
uintptr_t SkipWeapons::skip_dante_gun_continue{ NULL };
uintptr_t SkipWeapons::skip_dante_sword_continue{ NULL };

naked void skip_dante_gun_proc(void) {
	_asm {
		cmp eax, 7 // shotgun?
		je checkshotgun
		cmp eax, 8 // pandora
		je checkpandora
		jmp originalcode

	checkshotgun:
		cmp byte ptr [SkipWeapons::skip_shotgun], 1
		je skipshotgun
		jmp originalcode

	skipshotgun:
		mov dword ptr [edi+0x30], 8
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_gun_continue]

	checkpandora:
		cmp byte ptr [SkipWeapons::skip_pandora], 1
		je skippandora
		jmp originalcode

	skippandora:
		mov dword ptr [edi+0x30], 9
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_gun_continue]

	originalcode:
		mov [edi+0x30], eax
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_gun_continue]
	}
}

naked void skip_dante_sword_proc(void) {
    _asm {
		cmp eax, 5
		je checkgilgamesh
		cmp eax, 6
		je checklucifer
		jmp originalcode

	checkgilgamesh:
		cmp byte ptr [SkipWeapons::skip_gilgamesh], 1
		je skipgilgamesh
		jmp originalcode

	skipgilgamesh:
		mov dword ptr [edi+0x2C], 6
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_sword_continue]

	checklucifer:
		cmp byte ptr [SkipWeapons::skip_lucifer], 1
		je skiplucifer
		jmp originalcode

	skiplucifer:
		mov dword ptr [edi+0x2C], 4
		mov al, 01
		jmp dword ptr [SkipWeapons::skip_dante_sword_continue]

	originalcode:
		mov [edi+0x2C], eax
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
    if (ImGui::Checkbox(_("Skip Shotgun"), &skip_shotgun))
		skip_pandora = false;
    ImGui::SameLine();
    help_marker(_("Skips over Shotgun when Gun switching"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Skip Gilgamesh"), &skip_gilgamesh))
        skip_lucifer = false;
    ImGui::SameLine();
    help_marker(_("Skips over Gilgamesh when Weapon switching"));
    if (ImGui::Checkbox("Skip Pandora", &skip_pandora))
        skip_shotgun = false;
    ImGui::SameLine();
    help_marker(_("Skips over Pandora when Gun switching"));
    ImGui::SameLine(sameLineWidth);
    if (ImGui::Checkbox(_("Skip Lucifer"), &skip_lucifer))
        skip_gilgamesh = false;
    ImGui::SameLine();
    help_marker(_("Skips over Lucifer when Gun switching"));
}

void SkipWeapons::on_config_load(const utility::Config& cfg) {
    skip_shotgun = cfg.get<bool>("skip_shotgun").value_or(false);
    skip_pandora = cfg.get<bool>("skip_pandora").value_or(false);
    skip_gilgamesh = cfg.get<bool>("skip_gilgamesh").value_or(false);
	skip_lucifer = cfg.get<bool>("skip_lucifer").value_or(false);
}

void SkipWeapons::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("skip_shotgun", skip_shotgun);
    cfg.set<bool>("skip_pandora", skip_pandora);
    cfg.set<bool>("skip_gilgamesh", skip_gilgamesh);
    cfg.set<bool>("skip_lucifer", skip_lucifer);
}

#endif
