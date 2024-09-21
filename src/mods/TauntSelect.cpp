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
  "0x00",
  "0x01 Walk Forward",
  "0x02",
  "0x03",
  "0x04",
  "0x05",
  "0x06",
  "0x07",
  "0x08 Roll right",
  "0x09 Jump back",
  "0x0A T pose",
  "0x0B",
  "0x0C",
  "0x0D",
  "0x0E",
  "0x0F",
  "0x10 Trickster dash forward",
  "0x11",
  "0x12",
  "0x13",
  "0x14",
  "0x15",
  "0x16",
  "0x17",
  "0x18",
  "0x19",
  "0x1A",
  "0x1B",
  "0x1C",
  "0x1D",
  "0x1E",
  "0x1F",
  "0x20",
  "0x21",
  "0x22",
  "0x23",
  "0x24",
  "0x25",
  "0x26",
  "0x27",
  "0x28",
  "0x29",
  "0x2A",
  "0x2B",
  "0x2C",
  "0x2D",
  "0x2E",
  "0x2F",
  "0x30",
  "0x31",
  "0x32",
  "0x33",
  "0x34",
  "0x35",
  "0x36",
  "0x37",
  "0x38",
  "0x39",
  "0x3A",
  "0x3B",
  "0x3C",
  "0x3D",
  "0x3E",
  "0x3F",
  "0x40",
  "0x41",
  "0x42",
  "0x43",
  "0x44",
  "0x45",
  "0x46",
  "0x47",
  "0x48",
  "0x49",
  "0x4A",
  "0x4B",
  "0x4C",
  "0x4D",
  "0x4E",
  "0x4F",
  "0x50",
  "0x51",
  "0x52",
  "0x53",
  "0x54",
  "0x55",
  "0x56",
  "0x57",
  "0x58",
  "0x59",
  "0x5A",
  "0x5B",
  "0x5C",
  "0x5D",
  "0x5E",
  "0x5F",
  "0x60",
  "0x61",
  "0x62",
  "0x63",
  "0x64",
  "0x65",
  "0x66",
  "0x67",
  "0x68",
  "0x69",
  "0x6A",
  "0x6B",
  "0x6C",
  "0x6D",
  "0x6E",
  "0x6F",
  "0x70",
  "0x71",
  "0x72",
  "0x73",
  "0x74",
  "0x75",
  "0x76",
  "0x77",
  "0x78",
  "0x79",
  "0x7A",
  "0x7B",
  "0x7C",
  "0x7D",
  "0x7E",
  "0x7F",
  "0x80",
  "0x81",
  "0x82",
  "0x83",
  "0x84",
  "0x85",
  "0x86",
  "0x87",
  "0x88",
  "0x89",
  "0x8A",
  "0x8B",
  "0x8C",
  "0x8D",
  "0x8E",
  "0x8F",
  "0x90",
  "0x91",
  "0x92 Lean Forward",
  "0x93 Crash",
  "0x94 Crash",
  "0x95 Crash",
  "0x96 One handed cmon",
  "0x97 2 handed cmon",
  "0x98 Shrug",
  "0x99 Step back cmon",
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
  "0xAD Lean forward showdown",
  "0xAE Turn around hands up",
  "0xAF Darkslayer",
  "0xB0",
  "0xB1",
  "0xB2",
  "0xB3",
  "0xB4",
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

const char* neroTauntNames[] = {
  "0x00",
  "0x01",
  "0x02",
  "0x03",
  "0x04",
  "0x05",
  "0x06",
  "0x07",
  "0x08",
  "0x09",
  "0x0A",
  "0x0B",
  "0x0C",
  "0x0D",
  "0x0E",
  "0x0F",
  "0x10",
  "0x11",
  "0x12",
  "0x13",
  "0x14",
  "0x15",
  "0x16",
  "0x17",
  "0x18",
  "0x19",
  "0x1A",
  "0x1B",
  "0x1C",
  "0x1D",
  "0x1E",
  "0x1F",
  "0x20",
  "0x21",
  "0x22",
  "0x23",
  "0x24",
  "0x25",
  "0x26",
  "0x27",
  "0x28",
  "0x29",
  "0x2A",
  "0x2B",
  "0x2C",
  "0x2D",
  "0x2E",
  "0x2F",
  "0x30",
  "0x31",
  "0x32",
  "0x33",
  "0x34",
  "0x35",
  "0x36",
  "0x37",
  "0x38",
  "0x39",
  "0x3A",
  "0x3B",
  "0x3C",
  "0x3D",
  "0x3E",
  "0x3F",
  "0x40",
  "0x41",
  "0x42",
  "0x43",
  "0x44",
  "0x45",
  "0x46",
  "0x47",
  "0x48",
  "0x49",
  "0x4A",
  "0x4B",
  "0x4C",
  "0x4D",
  "0x4E",
  "0x4F",
  "0x50",
  "0x51",
  "0x52",
  "0x53",
  "0x54",
  "0x55",
  "0x56",
  "0x57",
  "0x58",
  "0x59",
  "0x5A",
  "0x5B",
  "0x5C",
  "0x5D",
  "0x5E",
  "0x5F",
  "0x60",
  "0x61",
  "0x62",
  "0x63",
  "0x64",
  "0x65",
  "0x66",
  "0x67",
  "0x68",
  "0x69",
  "0x6A",
  "0x6B",
  "0x6C",
  "0x6D",
  "0x6E",
  "0x6F",
  "0x70",
  "0x71",
  "0x72",
  "0x73",
  "0x74",
  "0x75",
  "0x76",
  "0x77",
  "0x78",
  "0x79",
  "0x7A",
  "0x7B",
  "0x7C",
  "0x7D",
  "0x7E",
  "0x7F",
  "0x80",
  "0x81",
  "0x82",
  "0x83",
  "0x84",
  "0x85",
  "0x86",
  "0x87",
  "0x88",
  "0x89",
  "0x8A",
  "0x8B",
  "0x8C",
  "0x8D",
  "0x8E",
  "0x8F",
  "0x90",
  "0x91",
  "0x92",
  "0x93",
  "0x94",
  "0x95 Crash",
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
  "0xA0 T pose",
  "0xA1",
  "0xA2",
  "0xA3",
  "0xA4",
  "0xA5",
  "0xA6",
  "0xA7",
  "0xA8",
  "0xA9",
  "0xAA",
  "0xAB",
  "0xAC",
  "0xAD",
  "0xAE",
  "0xAF",
  "0xB0",
  "0xB1",
  "0xB2",
  "0xB3",
  "0xB4",
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
	ImGui::Checkbox(_("Dante Taunt Select"), &mod_enabled);
	ImGui::SameLine();
	help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
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

	ImGui::Checkbox(_("Nero Taunt Select"), &mod_enabled2);
	ImGui::SameLine();
	help_marker(_("Unknown slots may crash. Test at your own risk.\nIf you get stuck, use character switch to free yourself"));
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
