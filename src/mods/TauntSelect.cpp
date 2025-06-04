#include "TauntSelect.hpp"

#if 1
bool TauntSelect::mod_enabled = false;
bool TauntSelect::mod_enabled2 = false;
uintptr_t TauntSelect::jmp_ret1 = NULL;
uintptr_t TauntSelect::jmp_ret2 = NULL;
uintptr_t TauntSelect::jmp_ret3 = NULL;
static int danteSelectedTauntID = 0;
static int neroSelectedTauntID = 0;

const char* danteTauntNames[] = {
  __("0x00 Idle"),
  __("0x01 Walk forward"),
  __("0x02 Run forward"),
  __("0x03 Sprint forward"),
  __("0x04 Jump"),
  __("0x05 Jump forward"),
  __("0x06 Return to idle"),
  __("0x07 Roll left"),
  __("0x08 Roll right"),
  __("0x09 Jump back"),
  __("0x0A NULL"),
  __("0x0B NULL"),
  __("0x0C NULL"),
  __("0x0D Land from stingerjump"),
  __("0x0E Stingerjump"),
  __("0x0F T pose (recoverable)"),
  __("0x10 Trickster dash forward"),
  __("0x11 Land from jump"),
  __("0x12 Rise midair"),
  __("0x13 Holy water"),
  __("0x14 Walljump"),
  __("0x15 Walljump"),
  __("0x16 Walljump"),
  __("0x17 Walljump"),
  __("0x18 Enemy step"),
  __("0x19 Walljump"),
  __("0x1A Walljump"),
  __("0x1B Walljump"),
  __("0x1C Walljump"),
  __("0x1D Jump back"),
  __("0x1E NULL"),
  __("0x1F NULL"),
  __("0x20 NULL"),
  __("0x21 NULL"),
  __("0x22 Stingerjump"),
  __("0x23 Land from stingerjump"),
  __("0x24 Flip from jump pad"),
  __("0x25 NULL"),
  __("0x26 NULL"),
  __("0x27 NULL"),
  __("0x28 NULL"),
  __("0x29 NULL"),
  __("0x2A NULL"),
  __("0x2B NULL"),
  __("0x2C NULL"),
  __("0x2D NULL"),
  __("0x2E NULL"),
  __("0x2F NULL"),
  __("0x30 NULL"),
  __("0x31 NULL"),
  __("0x32 DT"),
  __("0x33 NULL"),
  __("0x34 NULL"),
  __("0x35 NULL"),
  __("0x36 NULL"),
  __("0x37 NULL"),
  __("0x38 NULL"),
  __("0x39 NULL"),
  __("0x3A Leave DT"),
  __("0x3B NULL"),
  __("0x3C NULL"),
  __("0x3D NULL"),
  __("0x3E NULL"),
  __("0x3F NULL"),
  __("0x40 NULL"),
  __("0x41 NULL"),
  __("0x42 NULL"),
  __("0x43 NULL"),
  __("0x44 NULL"),
  __("0x45 NULL"),
  __("0x46 NULL"),
  __("0x47 NULL"),
  __("0x48 NULL"),
  __("0x49 NULL"),
  __("0x4A NULL"),
  __("0x4B NULL"),
  __("0x4C NULL"),
  __("0x4D NULL"),
  __("0x4E NULL"),
  __("0x4F NULL"),
  __("0x50 NULL"),
  __("0x51 NULL"),
  __("0x52 NULL"),
  __("0x53 NULL"),
  __("0x54 NULL"),
  __("0x55 NULL"),
  __("0x56 NULL"),
  __("0x57 NULL"),
  __("0x58 NULL"),
  __("0x59 NULL"),
  __("0x5A NULL"),
  __("0x5B NULL"),
  __("0x5C NULL"),
  __("0x5D NULL"),
  __("0x5E NULL"),
  __("0x5F NULL"),
  __("0x60 NULL"),
  __("0x61 NULL"),
  __("0x62 NULL"),
  __("0x63 NULL"),
  __("0x64 Take damage"),
  __("0x65 NULL"),
  __("0x66 NULL"),
  __("0x67 NULL"),
  __("0x68 Get knocked back into death"),
  __("0x69 NULL"),
  __("0x6A NULL"),
  __("0x6B Gold orb flip on to feet"),
  __("0x6C NULL"),
  __("0x6D NULL"),
  __("0x6E NULL"),
  __("0x6F Die"),
  __("0x70 NULL"),
  __("0x71 NULL"),
  __("0x72 NULL"),
  __("0x73 NULL"),
  __("0x74 Get knocked back"),
  __("0x75 NULL"),
  __("0x76 Get up again"),
  __("0x77 NULL"),
  __("0x78 Gold orb"),
  __("0x79 Take stun"),
  __("0x7A Get air juggled"),
  __("0x7B Shield face from wind"),
  __("0x7C Shield face from wind big"),
  __("0x7D Break out of something"),
  __("0x7E Break out of something big"),
  __("0x7F Fall after being hit"),
  __("0x80 NULL"),
  __("0x81 NULL"),
  __("0x82 Possessed thrown around"),
  __("0x83 Held in the air"),
  __("0x84 Falling from being held"),
  __("0x85 Fall to floor touching floor"),
  __("0x86 Get up from floor"),
  __("0x87 Fly back along floor on feet"),
  __("0x88 Shield face from wind"),
  __("0x89 NULL"),
  __("0x8A NULL"),
  __("0x8B NULL"),
  __("0x8C NULL"),
  __("0x8D NULL"),
  __("0x8E NULL"),
  __("0x8F NULL"),
  __("0x90 NULL"),
  __("0x91 NULL"),
  __("0x92 NULL"),
  __("0x93 NULL"),
  __("0x94 NULL"),
  __("0x95 NULL"),
  __("0x96 One handed cmon"),
  __("0x97 2 handed cmon"),
  __("0x98 Shrug"),
  __("0x99 Step back alright baby"),
  __("0x9A Cut bow"),
  __("0x9B Clap for doggo lean forward"),
  __("0x9C Dust off hands"),
  __("0x9D Turn around hands up"),
  __("0x9E Laugh"),
  __("0x9F Hands on hips"),
  __("0xA0 Stretch hands above head"),
  __("0xA1 Idly look around"),
  __("0xA2 Trickster"),
  __("0xA3 Swordmaster"),
  __("0xA4 Gunslinger"),
  __("0xA5 Royal Guard style change"),
  __("0xA6 Lean forward showdown"),
  __("0xA7 Cut gilg taunt"),
  __("0xA8 Cut luci taunt"),
  __("0xA9 Cut ei taunt"),
  __("0xAA Cut shotty taunt"),
  __("0xAB Shoot sky"),
  __("0xAC Cmon, broken root motion"),
  __("0xAD Lean forward cmon"),
  __("0xAE Turn around hands up"),
  __("0xAF Darkslayer"),
  __("0xB0 Dante boss death"),
  __("0xB1 Dante boss death"),
  __("0xB2 NULL"),
  __("0xB3 NULL"),
  __("0xB4 NULL"),
  __("0xB5 NULL"),
  __("0xB6 NULL"),
  __("0xB7 NULL"),
  __("0xB8 NULL"),
  __("0xB9 NULL"),
  __("0xBA NULL"),
  __("0xBB NULL"),
  __("0xBC NULL"),
  __("0xBD NULL"),
  __("0xBE NULL"),
  __("0xBF END?"),
  __("0xC0"),
  __("0xC1"),
  __("0xC2"),
  __("0xC3"),
  __("0xC4"),
  __("0xC5"),
  __("0xC6"),
  __("0xC7"),
  __("0xC8"),
  __("0xC9"),
  __("0xCA"),
  __("0xCB"),
  __("0xCC"),
  __("0xCD"),
  __("0xCE"),
  __("0xCF"),
  __("0xD0"),
  __("0xD1"),
  __("0xD2"),
  __("0xD3"),
  __("0xD4"),
  __("0xD5"),
  __("0xD6"),
  __("0xD7"),
  __("0xD8"),
  __("0xD9"),
  __("0xDA"),
  __("0xDB"),
  __("0xDC"),
  __("0xDD"),
  __("0xDE"),
  __("0xDF"),
  __("0xE0"),
  __("0xE1"),
  __("0xE2"),
  __("0xE3"),
  __("0xE4"),
  __("0xE5"),
  __("0xE6"),
  __("0xE7"),
  __("0xE8"),
  __("0xE9"),
  __("0xEA"),
  __("0xEB"),
  __("0xEC"),
  __("0xED"),
  __("0xEE"),
  __("0xEF"),
  __("0xF0"),
  __("0xF1"),
  __("0xF2"),
  __("0xF3"),
  __("0xF4"),
  __("0xF5"),
  __("0xF6"),
  __("0xF7"),
  __("0xF8"),
  __("0xF9"),
  __("0xFA"),
  __("0xFB"),
  __("0xFC"),
  __("0xFD"),
  __("0xFE"),
  __("0xFF")
};

const char* neroTauntNames[] = {
  __("0x00 Idle"),
  __("0x01 Walk forward"),
  __("0x02 Run forward"),
  __("0x03 Sprint forward"),
  __("0x04 Jump"),
  __("0x05 Jump forward"),
  __("0x06 Return to idle"),
  __("0x07 Roll left"),
  __("0x08 Roll right"),
  __("0x09 Jump back"),
  __("0x0A NULL"),
  __("0x0B NULL"),
  __("0x0C Roll forward"),
  __("0x0D Land from stingerjump"),
  __("0x0E Stingerjump"),
  __("0x0F 180 twirl"),
  __("0x10 Rise, prob midair anim"),
  __("0x11 Tele right"),
  __("0x12 Rise, prob midair anim"),
  __("0x13 Holy water"),
  __("0x14 Walljump"),
  __("0x15 Walljump back"),
  __("0x16 Walljump right"),
  __("0x17 Walljump left"),
  __("0x18 Airhike"),
  __("0x19 Walljump forward"),
  __("0x1A Walljump forward"),
  __("0x1B Walljump forward"),
  __("0x1C Walljump forward"),
  __("0x1D Jump back"),
  __("0x1E NULL"),
  __("0x1F Table Hopper left"),
  __("0x20 Table Hopper right"),
  __("0x21 Table Hopper forward"),
  __("0x22 Stinger jump fall"),
  __("0x23 Land from long jump"),
  __("0x24 Flip from jump pad"),
  __("0x25 NULL"),
  __("0x26 NULL"),
  __("0x27 NULL"),
  __("0x28 NULL"),
  __("0x29 NULL"),
  __("0x2A NULL"),
  __("0x2B NULL"),
  __("0x2C NULL"),
  __("0x2D NULL"),
  __("0x2E NULL"),
  __("0x2F NULL"),
  __("0x30 NULL"),
  __("0x31 NULL"),
  __("0x32 DT"),
  __("0x33 NULL"),
  __("0x34 NULL"),
  __("0x35 NULL"),
  __("0x36 NULL"),
  __("0x37 NULL"),
  __("0x38 NULL"),
  __("0x39 NULL"),
  __("0x3A NULL"),
  __("0x3B NULL"),
  __("0x3C Air DT"),
  __("0x3D NULL"),
  __("0x3E NULL"),
  __("0x3F NULL"),
  __("0x40 NULL"),
  __("0x41 NULL"),
  __("0x42 NULL"),
  __("0x43 NULL"),
  __("0x44 NULL"),
  __("0x45 NULL"),
  __("0x46 NULL"),
  __("0x47 NULL"),
  __("0x48 NULL"),
  __("0x49 NULL"),
  __("0x4A NULL"),
  __("0x4B NULL"),
  __("0x4C NULL"),
  __("0x4D NULL"),
  __("0x4E NULL"),
  __("0x4F NULL"),
  __("0x50 NULL"),
  __("0x51 NULL"),
  __("0x52 NULL"),
  __("0x53 NULL"),
  __("0x54 NULL"),
  __("0x55 NULL"),
  __("0x56 NULL"),
  __("0x57 NULL"),
  __("0x58 NULL"),
  __("0x59 NULL"),
  __("0x5A NULL"),
  __("0x5B NULL"),
  __("0x5C NULL"),
  __("0x5D NULL"),
  __("0x5E NULL"),
  __("0x5F NULL"),
  __("0x60 NULL"),
  __("0x61 NULL"),
  __("0x62 NULL"),
  __("0x63 NULL"),
  __("0x64 Take a hit"),
  __("0x65 Small stagger forward"),
  __("0x66 Stagger back"),
  __("0x67 Stagger forward"),
  __("0x68 Thrown back, die"),
  __("0x69 Thrown back with a twirl"),
  __("0x6A Flip back on to feet"),
  __("0x6B Gold orb"),
  __("0x6C NULL"),
  __("0x6D NULL"),
  __("0x6E NULL"),
  __("0x6F Fall forward and die"),
  __("0x70 NULL"),
  __("0x71 Held up"),
  __("0x72 Land from a jump"),
  __("0x73 NULL"),
  __("0x74 Thrown back on to feet"),
  __("0x75 Thrown back with a twirl"),
  __("0x76 Get up again"),
  __("0x77 NULL"),
  __("0x78 Gold orb"),
  __("0x79 Stagger back"),
  __("0x7A Get juggled"),
  __("0x7B Shield face from wind"),
  __("0x7C Shield face from wind"),
  __("0x7D Breaking out of ice, hitbox"),
  __("0x7E Breaking out of ice"),
  __("0x7F Fall from height"),
  __("0x80 NULL"),
  __("0x81 NULL"),
  __("0x82 Being thrown around"),
  __("0x83 Being held up"),
  __("0x84 Being pulled towards"),
  __("0x85 Falling down"),
  __("0x86 Getting up"),
  __("0x87 Sliding back on feet"),
  __("0x88 Shield face from wind"),
  __("0x89 NULL"),
  __("0x8A NULL"),
  __("0x8B NULL"),
  __("0x8C NULL"),
  __("0x8D NULL"),
  __("0x8E NULL"),
  __("0x8F NULL"),
  __("0x90 NULL"),
  __("0x91 NULL"),
  __("0x92 NULL"),
  __("0x93 NULL"),
  __("0x94 NULL"),
  __("0x95 NULL"),
  __("0x96 Slit throat"),
  __("0x97 What you say"),
  __("0x98 Come on"),
  __("0x99 Byebye"),
  __("0x9A Sword rev"),
  __("0x9B Slaps"),
  __("0x9C Shall we dance"),
  __("0x9D Guitar"),
  __("0x9E Claps"),
  __("0x9F Look at arm"),
  __("0xA0 NULL"),
  __("0xA1 T pose (recoverable)"),
  __("0xA2 Streak end"),
  __("0xA3 Streak mid"),
  __("0xA4 Streak start"),
  __("0xA5 Hey hey hey, come on babes"),
  __("0xA6 Shhh"),
  __("0xA7 NULL"),
  __("0xA8 NULL"),
  __("0xA9 NULL"),
  __("0xAA NULL"),
  __("0xAB NULL"),
  __("0xAC NULL"),
  __("0xAD NULL"),
  __("0xAE NULL"),
  __("0xAF NULL"),
  __("0xB0 NULL"),
  __("0xB1 NULL"),
  __("0xB2 NULL"),
  __("0xB3 NULL"),
  __("0xB4 END?"),
  __("0xB5"),
  __("0xB6"),
  __("0xB7"),
  __("0xB8"),
  __("0xB9"),
  __("0xBA"),
  __("0xBB"),
  __("0xBC"),
  __("0xBD"),
  __("0xBE"),
  __("0xBF"),
  __("0xC0"),
  __("0xC1"),
  __("0xC2"),
  __("0xC3"),
  __("0xC4"),
  __("0xC5"),
  __("0xC6"),
  __("0xC7"),
  __("0xC8"),
  __("0xC9"),
  __("0xCA"),
  __("0xCB"),
  __("0xCC"),
  __("0xCD"),
  __("0xCE"),
  __("0xCF"),
  __("0xD0"),
  __("0xD1"),
  __("0xD2"),
  __("0xD3"),
  __("0xD4"),
  __("0xD5"),
  __("0xD6"),
  __("0xD7"),
  __("0xD8"),
  __("0xD9"),
  __("0xDA"),
  __("0xDB"),
  __("0xDC"),
  __("0xDD"),
  __("0xDE"),
  __("0xDF"),
  __("0xE0"),
  __("0xE1"),
  __("0xE2"),
  __("0xE3"),
  __("0xE4"),
  __("0xE5"),
  __("0xE6"),
  __("0xE7"),
  __("0xE8"),
  __("0xE9"),
  __("0xEA"),
  __("0xEB"),
  __("0xEC"),
  __("0xED"),
  __("0xEE"),
  __("0xEF"),
  __("0xF0"),
  __("0xF1"),
  __("0xF2"),
  __("0xF3"),
  __("0xF4"),
  __("0xF5"),
  __("0xF6"),
  __("0xF7"),
  __("0xF8"),
  __("0xF9"),
  __("0xFA"),
  __("0xFB"),
  __("0xFC"),
  __("0xFD"),
  __("0xFE"),
  __("0xFF")
};

naked void detour1(void) { // Dante Taunt Style Switch
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

void TauntSelect::on_gui_frame(int display) {
	if (display == 1) {
		ImGui::Checkbox(_("Enable##TantSelectNero"), &mod_enabled2);
		ImGui::SameLine();
		help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
		if (mod_enabled2) {
            utility::ImGooListboxTranslated localized(neroTauntNames, IM_ARRAYSIZE(neroTauntNames));
			ImGui::Indent(lineIndent);
			if (ImGui::BeginCombo("##NeroTauntSelectComboBox", localized.data_[neroSelectedTauntID])) {
				for (int n = 0; n < localized.count; n++) {
					const bool is_selected = (neroSelectedTauntID == n);
					if (ImGui::Selectable(localized.data_[n], is_selected)) {
						neroSelectedTauntID = n;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::Unindent(lineIndent);
		}
	}
	else if (display == 2) {
		ImGui::Checkbox(_("Enable##TauntSelectDante"), &mod_enabled);
		ImGui::SameLine();
		help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
		if (mod_enabled) {
			utility::ImGooListboxTranslated localized(danteTauntNames, IM_ARRAYSIZE(danteTauntNames));
			ImGui::Indent(lineIndent);
			if (ImGui::BeginCombo("##DanteTauntSelectComboBox", localized.data_[danteSelectedTauntID])) {
				for (int n = 0; n < localized.count; n++) {
					const bool is_selected = (danteSelectedTauntID == n);
					if (ImGui::Selectable(localized.data_[n], is_selected)) {
						danteSelectedTauntID = n;
					}
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::Unindent(lineIndent);
		}
	}
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
