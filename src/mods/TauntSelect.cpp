#include "TauntSelect.hpp"

#if 1
bool TauntSelect::mod_enabled{ false };
bool TauntSelect::mod_enabled2{ false };
uintptr_t TauntSelect::jmp_ret1{ NULL };
uintptr_t TauntSelect::jmp_ret2{ NULL };
uintptr_t TauntSelect::jmp_ret3{ NULL };
static int danteSelectedTauntID = 0;
static int neroSelectedTauntID = 0;

const char* danteTauntNames[] = {
  "0x00 Idle",
  "0x01 Walk forward",
  "0x02 Run forward",
  "0x03 Sprint forward",
  "0x04 Jump",
  "0x05 Jump forward",
  "0x06 Return to idle",
  "0x07 Rol left",
  "0x08 Roll right",
  "0x09 Jump back",
  "0x0A NULL",
  "0x0B NULL",
  "0x0C NULL",
  "0x0D Land from stingerjump",
  "0x0E Stingerjump",
  "0x0F T pose (recoverable)",
  "0x10 Trickster dash forward",
  "0x11 Land from jump",
  "0x12 Rise midair",
  "0x13 Holy water",
  "0x14 Walljump",
  "0x15 Walljump",
  "0x16 Walljump",
  "0x17 Walljump",
  "0x18 Enemy step",
  "0x19 Walljump",
  "0x1A Walljump",
  "0x1B Walljump",
  "0x1C Walljump",
  "0x1D Jump back",
  "0x1E NULL",
  "0x1F NULL",
  "0x20 NULL",
  "0x21 NULL",
  "0x22 Stingerjump",
  "0x23 Land from stingerjump",
  "0x24 Flip from jump pad",
  "0x25 NULL",
  "0x26 NULL",
  "0x27 NULL",
  "0x28 NULL",
  "0x29 NULL",
  "0x2A NULL",
  "0x2B NULL",
  "0x2C NULL",
  "0x2D NULL",
  "0x2E NULL",
  "0x2F NULL",
  "0x30 NULL",
  "0x31 NULL",
  "0x32 DT",
  "0x33 NULL",
  "0x34 NULL",
  "0x35 NULL",
  "0x36 NULL",
  "0x37 NULL",
  "0x38 NULL",
  "0x39 NULL",
  "0x3A Leave DT",
  "0x3B NULL",
  "0x3C NULL",
  "0x3D NULL",
  "0x3E NULL",
  "0x3F NULL",
  "0x40 NULL",
  "0x41 NULL",
  "0x42 NULL",
  "0x43 NULL",
  "0x44 NULL",
  "0x45 NULL",
  "0x46 NULL",
  "0x47 NULL",
  "0x48 NULL",
  "0x49 NULL",
  "0x4A NULL",
  "0x4B NULL",
  "0x4C NULL",
  "0x4D NULL",
  "0x4E NULL",
  "0x4F NULL",
  "0x50 NULL",
  "0x51 NULL",
  "0x52 NULL",
  "0x53 NULL",
  "0x54 NULL",
  "0x55 NULL",
  "0x56 NULL",
  "0x57 NULL",
  "0x58 NULL",
  "0x59 NULL",
  "0x5A NULL",
  "0x5B NULL",
  "0x5C NULL",
  "0x5D NULL",
  "0x5E NULL",
  "0x5F NULL",
  "0x60 NULL",
  "0x61 NULL",
  "0x62 NULL",
  "0x63 NULL",
  "0x64 Take damage",
  "0x65 NULL",
  "0x66 NULL",
  "0x67 NULL",
  "0x68 Get knocked back into death",
  "0x69 NULL",
  "0x6A NULL",
  "0x6B Gold orb flip on to feet",
  "0x6C NULL",
  "0x6D NULL",
  "0x6E NULL",
  "0x6F Die",
  "0x70 NULL",
  "0x71 NULL",
  "0x72 NULL",
  "0x73 NULL",
  "0x74 Get knocked back",
  "0x75 NULL",
  "0x76 Get up again",
  "0x77 NULL",
  "0x78 Gold orb",
  "0x79 Take stun",
  "0x7A Get air juggled",
  "0x7B Shield face from wind",
  "0x7C Shield face from wind big",
  "0x7D Break out of something",
  "0x7E Break out of something big",
  "0x7F Fall after being hit",
  "0x80 NULL",
  "0x81 NULL",
  "0x82 Possessed thrown around",
  "0x83 Held in the air",
  "0x84 Falling from being held",
  "0x85 Fall to floor touching floor",
  "0x86 Get up from floor",
  "0x87 Fly back along floor on feet",
  "0x88 Shield face from wind",
  "0x89 NULL",
  "0x8A NULL",
  "0x8B NULL",
  "0x8C NULL",
  "0x8D NULL",
  "0x8E NULL",
  "0x8F NULL",
  "0x90 NULL",
  "0x91 NULL",
  "0x92 Lean Forward",
  "0x93 NULL",
  "0x94 NULL",
  "0x95 NULL",
  "0x96 One handed cmon",
  "0x97 2 handed cmon",
  "0x98 Shrug",
  "0x99 Step back alright baby",
  "0x9A Cut bow",
  "0x9B Clap for doggo lean forward",
  "0x9C Dust off hands",
  "0x9D Turn around hands up",
  "0x9E Laugh",
  "0x9F Hands on hips",
  "0xA0 Stretch hands above head",
  "0xA1 Idly look around",
  "0xA2 Trickster",
  "0xA3 Swordmaster",
  "0xA4 Gunslinger",
  "0xA5 Royal Guard style change",
  "0xA6 Lean forward showdown",
  "0xA7 Cut gilg taunt",
  "0xA8 Cut luci taunt",
  "0xA9 Cut ei taunt",
  "0xAA Cut shotty taunt",
  "0xAB Shoot sky",
  "0xAC Cmon, broken root motion",
  "0xAD Lean forward cmon",
  "0xAE Turn around hands up",
  "0xAF Darkslayer",
  "0xB0 Dante boss death",
  "0xB1 Dante boss death",
  "0xB2 NULL",
  "0xB3 NULL",
  "0xB4 NULL",
  "0xB5 NULL",
  "0xB6 NULL",
  "0xB7 NULL",
  "0xB8 NULL",
  "0xB9 NULL",
  "0xBA NULL",
  "0xBB NULL",
  "0xBC NULL",
  "0xBD NULL",
  "0xBE NULL",
  "0xBF END?",
  "0xC0",
  "0xC1",
  "0xC2",
  "0xC3",
  "0xC4",
  "0xC5",
  "0xC6",
  "0xC7",
  "0xC8",
  "0xC9",
  "0xCA",
  "0xCB",
  "0xCC",
  "0xCD",
  "0xCE",
  "0xCF",
  "0xD0",
  "0xD1",
  "0xD2",
  "0xD3",
  "0xD4",
  "0xD5",
  "0xD6",
  "0xD7",
  "0xD8",
  "0xD9",
  "0xDA",
  "0xDB",
  "0xDC",
  "0xDD",
  "0xDE",
  "0xDF",
  "0xE0",
  "0xE1",
  "0xE2",
  "0xE3",
  "0xE4",
  "0xE5",
  "0xE6",
  "0xE7",
  "0xE8",
  "0xE9",
  "0xEA",
  "0xEB",
  "0xEC",
  "0xED",
  "0xEE",
  "0xEF",
  "0xF0",
  "0xF1",
  "0xF2",
  "0xF3",
  "0xF4",
  "0xF5",
  "0xF6",
  "0xF7",
  "0xF8",
  "0xF9",
  "0xFA",
  "0xFB",
  "0xFC",
  "0xFD",
  "0xFE",
  "0xFF"
};

const char* neroTauntNames[] = {
  "0x00 Idle",
  "0x01 Walk forward",
  "0x02 Run forward",
  "0x03 Sprint forward",
  "0x04 Jump",
  "0x05 Jump forward",
  "0x06 Return to idle",
  "0x07 Roll left",
  "0x08 Roll right",
  "0x09 Jump back",
  "0x0A NULL",
  "0x0B NULL",
  "0x0C Roll forward",
  "0x0D Land from stingerjump",
  "0x0E Stingerjump",
  "0x0F NULL",
  "0x10 Rise, prob midair anim",
  "0x11 Tele right",
  "0x12 Rise, prob midair anim",
  "0x13 Holy water",
  "0x14 Walljump",
  "0x15 Walljump back",
  "0x16 Walljump right",
  "0x17 Walljump left",
  "0x18 Airhike",
  "0x19 Walljump forward",
  "0x1A Walljump forward",
  "0x1B Walljump forward",
  "0x1C Walljump forward",
  "0x1D Jump back",
  "0x1E NULL",
  "0x1F Table Hopper left",
  "0x20 Table Hopper right",
  "0x21 Table Hopper forward",
  "0x22 Stinger jump fall",
  "0x23 Land from long jump",
  "0x24 Flip from jump pad",
  "0x25 NULL",
  "0x26 NULL",
  "0x27 NULL",
  "0x28 NULL",
  "0x29 NULL",
  "0x2A NULL",
  "0x2B NULL",
  "0x2C NULL",
  "0x2D NULL",
  "0x2E NULL",
  "0x2F NULL",
  "0x30 NULL",
  "0x31 NULL",
  "0x32 DT",
  "0x33 NULL",
  "0x34 NULL",
  "0x35 NULL",
  "0x36 NULL",
  "0x37 NULL",
  "0x38 NULL",
  "0x39 NULL",
  "0x3A NULL",
  "0x3B NULL",
  "0x3C Air DT",
  "0x3D NULL",
  "0x3E NULL",
  "0x3F NULL",
  "0x40 NULL",
  "0x41 NULL",
  "0x42 NULL",
  "0x43 NULL",
  "0x44 NULL",
  "0x45 NULL",
  "0x46 NULL",
  "0x47 NULL",
  "0x48 NULL",
  "0x49 NULL",
  "0x4A NULL",
  "0x4B NULL",
  "0x4C NULL",
  "0x4D NULL",
  "0x4E NULL",
  "0x4F NULL",
  "0x50 NULL",
  "0x51 NULL",
  "0x52 NULL",
  "0x53 NULL",
  "0x54 NULL",
  "0x55 NULL",
  "0x56 NULL",
  "0x57 NULL",
  "0x58 NULL",
  "0x59 NULL",
  "0x5A NULL",
  "0x5B NULL",
  "0x5C NULL",
  "0x5D NULL",
  "0x5E NULL",
  "0x5F NULL",
  "0x60 NULL",
  "0x61 NULL",
  "0x62 NULL",
  "0x63 NULL",
  "0x64 Take a hit",
  "0x65 Small stagger forward",
  "0x66 Stagger back",
  "0x67 Stagger forward",
  "0x68 Thrown back, die",
  "0x69 Thrown back with a twirl",
  "0x6A Flip back on to feet",
  "0x6B Gold orb",
  "0x6C NULL",
  "0x6D NULL",
  "0x6E NULL",
  "0x6F Fall forward and die",
  "0x70 NULL",
  "0x71 Held up",
  "0x72 Land from a jump",
  "0x73 NULL",
  "0x74 Thrown back on to feet",
  "0x75 Thrown back with a twirl",
  "0x76 Walter dodge",
  "0x77 NULL",
  "0x78 Gold orb",
  "0x79 Stagger back",
  "0x7A Get juggled",
  "0x7B Shield face from wind",
  "0x7C Shield face from wind",
  "0x7D Pull apart something, glass sound, hitbox",
  "0x7E Glass sound",
  "0x7F Fall from height",
  "0x80 NULL",
  "0x81 NULL",
  "0x82 Being thrown around",
  "0x83 Being held up",
  "0x84 Being pulled towards",
  "0x85 Falling down",
  "0x86 Getting up",
  "0x87 Sliding back on feet",
  "0x88 Shield face from wind",
  "0x89 NULL",
  "0x8A NULL",
  "0x8B NULL",
  "0x8C NULL",
  "0x8D NULL",
  "0x8E NULL",
  "0x8F NULL",
  "0x90 NULL",
  "0x91 NULL",
  "0x92 NULL",
  "0x93 NULL",
  "0x94 NULL",
  "0x95 NULL",
  "0x96 Slit throat",
  "0x97 What you say",
  "0x98 Come on",
  "0x99 Byebye",
  "0x9A Sword rev",
  "0x9B Slaps",
  "0x9C Shall we dance",
  "0x9D Guitar",
  "0x9E Claps",
  "0x9F Look at arm",
  "0xA0 NULL",
  "0xA1 T pose (recoverable)",
  "0xA2 Streak end",
  "0xA3 Streak mid",
  "0xA4 Streak start",
  "0xA5 Hey hey hey, come on babes",
  "0xA6 Shhh",
  "0xA7 NULL",
  "0xA8 NULL",
  "0xA9 NULL",
  "0xAA NULL",
  "0xAB NULL",
  "0xAC NULL",
  "0xAD NULL",
  "0xAE NULL",
  "0xAF NULL",
  "0xB0 NULL",
  "0xB1 NULL",
  "0xB2 NULL",
  "0xB3 NULL",
  "0xB4 END?",
  "0xB5",
  "0xB6",
  "0xB7",
  "0xB8",
  "0xB9",
  "0xBA",
  "0xBB",
  "0xBC",
  "0xBD",
  "0xBE",
  "0xBF",
  "0xC0",
  "0xC1",
  "0xC2",
  "0xC3",
  "0xC4",
  "0xC5",
  "0xC6",
  "0xC7",
  "0xC8",
  "0xC9",
  "0xCA",
  "0xCB",
  "0xCC",
  "0xCD",
  "0xCE",
  "0xCF",
  "0xD0",
  "0xD1",
  "0xD2",
  "0xD3",
  "0xD4",
  "0xD5",
  "0xD6",
  "0xD7",
  "0xD8",
  "0xD9",
  "0xDA",
  "0xDB",
  "0xDC",
  "0xDD",
  "0xDE",
  "0xDF",
  "0xE0",
  "0xE1",
  "0xE2",
  "0xE3",
  "0xE4",
  "0xE5",
  "0xE6",
  "0xE7",
  "0xE8",
  "0xE9",
  "0xEA",
  "0xEB",
  "0xEC",
  "0xED",
  "0xEE",
  "0xEF",
  "0xF0",
  "0xF1",
  "0xF2",
  "0xF3",
  "0xF4",
  "0xF5",
  "0xF6",
  "0xF7",
  "0xF8",
  "0xF9",
  "0xFA",
  "0xFB",
  "0xFC",
  "0xFD",
  "0xFE",
  "0xFF"
};

naked void detour1(void) { // Dante Taunt Switch
	_asm {
		cmp byte ptr [TauntSelect::mod_enabled], 0
		je originalcode
		mov eax,7 // taunt selector thinks we're always in SSS
	originalcode:
		cmp eax, 07
		push esi
		mov esi,ecx
		jmp dword ptr [TauntSelect::jmp_ret1]
	}
}

naked void detour2(void) { // Dante Taunt MoveID
    _asm {
		cmp byte ptr [TauntSelect::mod_enabled], 0
		je originalcode
		mov eax, [danteSelectedTauntID]
		jmp jmp_ret

	originalcode:
		add eax, 0x000000A6
	jmp_ret:
		jmp dword ptr [TauntSelect::jmp_ret2]
    }
}

naked void detour3(void) { // Nero Taunt MoveID
	_asm {
		cmp byte ptr [TauntSelect::mod_enabled2], 0
		je originalcode
		mov ecx, [neroSelectedTauntID]
		jmp cont
	originalcode:
		mov ecx, [esi] // No style selector needed - variable [esi]
	cont:
		sub esp, 0x0C
		jmp dword ptr [TauntSelect::jmp_ret3]
	}
}

std::optional<std::string> TauntSelect::on_initialize() {
	if (!install_hook_offset(0x3BD5F8, hook, &detour1, &TauntSelect::jmp_ret1, 6)) {
		spdlog::error("Failed to init TauntSelect mod\n");
		return "Failed to init TauntSelect mod";
	}
	if (!install_hook_offset(0x3BD676, hook2, &detour2, &TauntSelect::jmp_ret2, 5)) {
		spdlog::error("Failed to init TauntSelect mod2\n");
		return "Failed to init TauntSelect mod2";
	}

	if (!install_hook_offset(0x3E6FA5, hook3, &detour3, &TauntSelect::jmp_ret3, 5)) {
		spdlog::error("Failed to init TauntSelect mod3\n");
		return "Failed to init TauntSelect mod3";
	}

	return Mod::on_initialize();
}
void TauntSelect::on_gui_frame() {
	ImGui::Text(_("Taunt Select"));
	ImGui::Spacing();
	ImGui::Checkbox(_("Dante Taunt Select"), &mod_enabled);
	ImGui::SameLine();
	help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
	ImGui::Indent(lineIndent);
	if (ImGui::BeginCombo("##DanteTauntSelectComboBox", danteTauntNames[danteSelectedTauntID])) {
        for (int n = 0; n < IM_ARRAYSIZE(danteTauntNames); n++) {
            const bool is_selected = (danteSelectedTauntID == n);
			if (ImGui::Selectable(danteTauntNames[n], is_selected)) {
				danteSelectedTauntID = n;
			}
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
	}
	ImGui::Unindent(lineIndent);

	ImGui::Checkbox(_("Nero Taunt Select"), &mod_enabled2);
	ImGui::SameLine();
	help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
	ImGui::Indent(lineIndent);
	if (ImGui::BeginCombo("##NeroTauntSelectComboBox", neroTauntNames[neroSelectedTauntID])) {
        for (int n = 0; n < IM_ARRAYSIZE(neroTauntNames); n++) {
            const bool is_selected = (neroSelectedTauntID == n);
			if (ImGui::Selectable(neroTauntNames[n], is_selected)) {
				neroSelectedTauntID = n;
			}
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
	}
	ImGui::Unindent(lineIndent);
}

void TauntSelect::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("taunt_select_enabled").value_or(false);
	mod_enabled2 = cfg.get<bool>("taunt_select_enabled2").value_or(false);
	danteSelectedTauntID = cfg.get<int>("dante_selected_taunt").value_or(0xa6);
	neroSelectedTauntID = cfg.get<int>("nero_selected_taunt").value_or(0x9d);
}

void TauntSelect::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("taunt_select_enabled", mod_enabled);
	cfg.set<bool>("taunt_select_enabled2", mod_enabled2);
	cfg.set<int>("dante_selected_taunt", danteSelectedTauntID);
	cfg.set<int>("nero_selected_taunt", neroSelectedTauntID);
}

#endif
