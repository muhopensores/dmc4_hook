#include "LiveMoveTableSwaps.hpp"
#include "TrickDown.hpp"
#include "MoveTable.hpp"

bool LiveMoveTableSwaps::instant_honeycomb_enabled = false;
uintptr_t LiveMoveTableSwaps::live_move_table_swaps_jmp_ret = NULL;

naked void live_move_table_swaps_detour(void) { // player in ecx
	_asm {
			cmp byte ptr [TrickDown::mod_enabled], 1
			je playercheck
			cmp byte ptr [LiveMoveTableSwaps::instant_honeycomb_enabled], 1
			je playercheck
			jmp originalcode // if nothing is using the timer, skip it

		playercheck:
			push eax
			mov eax, [static_mediator_ptr]
			mov eax, [eax]
			mov eax, [eax+0x24]
			cmp eax, ecx
			pop eax
			jne originalcode // only inc timer if player
			cmp byte ptr [ecx+0x1494], 0 // dante controller id
			je dantecode
			jmp originalcode

		dantecode:
        // trick down check
			cmp byte ptr [TrickDown::mod_enabled], 1
			je tricktimercompare
			jmp honeycombcheck

		tricktimercompare:
			cmp byte ptr [ecx+0x27f8], 3 // If trick down is enabled, replace trickster dash and sky star to trick when timer is 3
			jne dontreplacetrick
		replacetrick:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetrick
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 96 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x5D // trickster dash
			mov dword ptr [eax+0x4+0x38], 0x5D // sky star
			pop ebx
			pop eax
			jmp honeycombcheck

		dontreplacetrick:
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 96 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x5B // trickster dash
			mov dword ptr [eax+0x4+0x38], 0x5C // sky star
			pop ebx
			pop eax
		honeycombcheck:
			cmp byte ptr [LiveMoveTableSwaps::instant_honeycomb_enabled], 1
			je honeycombtimercompare   // If instant honeycomb is enabled, replace twosome time with honeycomb fire when backforward is 3
			jmp dontreplacetwosome

		honeycombtimercompare:
			cmp byte ptr [ecx+0x27f8], 3
			jne dontreplacetwosome
		replacetwosome:
            cmp byte ptr [ecx+0x16D0], 0 // lockon
			je dontreplacetwosome
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 69 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x45 // Replace Twosome Time ID with Honeycomb Fire ID
			pop ebx
			pop eax
			jmp originalcode

		dontreplacetwosome:
			push eax
			push ebx
			mov eax, 0x38 // struct size
			mov ebx, 69 // entry
			add ebx, [MoveTable::extra_dante_moves] // + extra entries
			imul ebx, eax
			mov eax, [ecx+0x1DCC] // kAtckDefTblPtr
			add eax, ebx // Twosome time start
			mov dword ptr [eax+0x4], 0x44 // Restore Honeycomb Fire ID
			pop ebx
			pop eax

		originalcode:
			test bl, 01
			mov dword ptr [ecx+0x00000EA8], 00000000
			jmp dword ptr [LiveMoveTableSwaps::live_move_table_swaps_jmp_ret]
	}
}

std::optional<std::string> LiveMoveTableSwaps::on_initialize() {
    if (!install_hook_offset(0x003AD767, live_move_table_swaps_hook, &live_move_table_swaps_detour, &live_move_table_swaps_jmp_ret, 13)) {
		spdlog::error("Failed to init LiveMoveTableSwaps mod\n");
		return "Failed to init LiveMoveTableSwaps mod";
	}
	return Mod::on_initialize();
}

void LiveMoveTableSwaps::on_gui_frame(int display) {
    if (display == DISPLAY_DANTE_A) {
        ImGui::Checkbox(_("Backforward Honeycomb"), &instant_honeycomb_enabled);
        ImGui::SameLine();
        help_marker(_("Honeycomb Mapped To backforward + Gunslinger"));
    }
}

void LiveMoveTableSwaps::on_config_load(const utility::Config& cfg) {
    instant_honeycomb_enabled = cfg.get<bool>("instant_honeycomb").value_or(false);
}

void LiveMoveTableSwaps::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("instant_honeycomb", instant_honeycomb_enabled);
}
