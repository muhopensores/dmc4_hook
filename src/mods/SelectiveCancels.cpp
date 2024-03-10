
#include "SelectiveCancels.hpp"
#include "MoveIDs.hpp"
#include "MoveIDsNero.hpp"

#if 1
bool      SelectiveCancels::selective_cancels_enable = false;
uint32_t  SelectiveCancels::cancels = 0;
uintptr_t SelectiveCancels::selective_cancels_continue = 0x0080332F;
uintptr_t SelectiveCancels::jmp_ret2 = NULL;
bool fixGuardInertia = false;

naked void detour1() { // player in eax + edi
	_asm {
		cmp byte ptr [SelectiveCancels::selective_cancels_enable], 0
		je originalcode

		cmp dword ptr [MoveIds::move_id], 0x411 // Grounded Ecstasy
		je cancellableecstasy
		cmp dword ptr [MoveIds::move_id], 0x412 // Aerial Ecstasy
		je cancellableecstasy
		cmp dword ptr [MoveIds::move_id], 0x732 // Argument
		je cancellableargument
		cmp dword ptr [MoveIds::move_id], 0x30E // Kick 13
		je cancellablekickthirteen
		cmp dword ptr [MoveIds::move_id], 0x30F // DT Kick 13
		je cancellablekickthirteen
		cmp dword ptr [MoveIds::move_id], 0x900 // Slash Dimension
		je cancellableslashdimension
		cmp dword ptr [MoveIds::move_id], 0x232 // Prop
		je cancellableprop
		cmp dword ptr [MoveIds::move_id], 0x333 // Shock
		je cancellableshock
		cmp dword ptr [MoveIds::move_id], 0x735 // Omen
		je cancellableomen
		cmp dword ptr [MoveIds::move_id], 0x635 // Gunstinger
		je cancellablegunstinger
		cmp dword ptr [MoveIds::move_id], 0x706 // Epidemic
		je cancellableepidemic
		cmp dword ptr [MoveIds::move_id], 0x410 // DT Pin Up part 2
		je cancellableDTPinUp
		cmp dword ptr [MoveIdsNero::move_id_nero], 0x33B // Showdown
		je cancellableShowdown
		cmp dword ptr [MoveIdsNero::move_id_nero], 0x32 // DT Ground
		je cancellableDTGround
		cmp dword ptr [MoveIds::move_id], 0x310 // Draw
		je cancellableDraw
		cmp dword ptr [MoveIds::move_id], 0x7 // Roll left
		je cancellableRoll
		cmp dword ptr [MoveIds::move_id], 0x8 // Roll right
		je CancellableRoll
		cmp dword ptr [MoveIdsNero::move_id_nero], 0x7 // Roll left
		je CancellableRoll
		cmp dword ptr [MoveIdsNero::move_id_nero], 0x8 // Roll right
		je CancellableRoll
		jmp originalcode

		cancellableecstasy:
		test [SelectiveCancels::cancels], ECSTASY
		jg cancellable
		jmp originalcode

		cancellableargument:
		test [SelectiveCancels::cancels], ARGUMENT
		jg cancellable
		jmp originalcode

		cancellablekickthirteen:
		test [SelectiveCancels::cancels], KICK13
		jg cancellable
		jmp originalcode

		cancellableslashdimension:
		test [SelectiveCancels::cancels], SLASH_DIMENSION
		jg cancellable
		jmp originalcode

		cancellableprop:
		test [SelectiveCancels::cancels], PROP
		jg cancellable
		jmp originalcode

		cancellableshock:
		test [SelectiveCancels::cancels], SHOCK
		jg cancellable
		jmp originalcode

		cancellableomen:
		test [SelectiveCancels::cancels], OMEN
		jg cancellable
		jmp originalcode

		cancellablegunstinger:
		test [SelectiveCancels::cancels], GUNSTINGER
		jg cancellable
		jmp originalcode

		cancellableepidemic:
		test [SelectiveCancels::cancels], EPIDEMIC
		jg cancellable
		jmp originalcode

		cancellableDTPinUp:
		test [SelectiveCancels::cancels], DT_PIN_UP_P2
		jg cancellable
		jmp originalcode

		cancellableShowdown:
		test [SelectiveCancels::cancels], SHOWDOWN
		jg cancellable
		jmp originalcode

		cancellableDTGround:
		test [SelectiveCancels::cancels], DTGROUND
		jg cancellable
		jmp originalcode

		cancellableDraw:
		test [SelectiveCancels::cancels], DRAW
		jg cancellable
		jmp originalcode

		cancellableRoll:
		// cmp dword ptr [eax+0x348], 0x41200000 // 10.0f // timer example
		// jb originalcode
		test [SelectiveCancels::cancels], ROLL
		jg cancellable
		jmp originalcode

		cancellable:
		mov dword ptr [eax+0x3174], 0x02 // [+0x3174] is jumps + trickster + guard // 2 is cancellable, 1 sets a buffer
			
		originalcode:
		mov edi,0x00000008
		jmp dword ptr [SelectiveCancels::selective_cancels_continue]
	}
}

naked void detour2() { // player in eax + edi
	_asm {
			cmp byte ptr [SelectiveCancels::selective_cancels_enable], 0
			je originalcode
			cmp byte ptr [fixGuardInertia], 0
			je originalcode

			mov dword ptr [ecx+0xec0], 0 // x
			mov dword ptr [ecx+0xec8], 0 // z

		originalcode:
			push 0x00000132
			jmp dword ptr [SelectiveCancels::jmp_ret2]
    }
}

std::optional<std::string> SelectiveCancels::on_initialize() {
	if (!install_hook_offset(0x40332A, hook1, &detour1, 0, 6)) {
		spdlog::error("Failed to init SelectiveCancels\n");
		return "Failed to init SelectiveCancels";
	}
	if (!install_hook_offset(0x3CBA06, hook2, &detour2, &jmp_ret2, 5)) {
		spdlog::error("Failed to init SelectiveCancels\n");
		return "Failed to init SelectiveCancels";
	}
	return Mod::on_initialize();
}

inline void SelectiveCancels::draw_checkbox_simple(const char* name, CancelMoves move) {
	// bitwise [AND cancels, move] to extract the bit that matters to us.
	bool check = (cancels & move) > 0;
	if (ImGui::Checkbox(name, &check)) {
		if (check) {
			// checking checkboxes in GUI applies bitwise [OR cancels, move] to set the bit.
			cancels |= move;
		}
		else {
			// unchecking uses bitwise [XOR cancels, move] to unset the bit, prob there is
			// a better way using some assembly 101 tips and tricks
			// but i want to sleep i dont want to shuffle bits in windows calculator
			cancels ^= move;
		}
	}
}

void SelectiveCancels::on_gui_frame() {
	ImGui::Text("Selective Cancels");
	ImGui::Spacing();
	ImGui::Checkbox("Enable", &selective_cancels_enable);
	ImGui::SameLine();
	help_marker("Allows cancelling out of selected moves with evasive actions");
	ImGui::SameLine(sameLineWidth);
	ImGui::Checkbox("Disable Guardslide", &fixGuardInertia);
	ImGui::SameLine();
	help_marker("Guarding a move with momentum will halt your movement");

	ImGui::Spacing();
	
	ImGui::Separator();
    ImGui::Text("Shared");
    ImGui::Spacing();
	
	draw_checkbox_simple("Roll", ROLL);

	ImGui::Separator();
    ImGui::Text("Nero");
    ImGui::Spacing();

	draw_checkbox_simple("Grounded DT Activation", DTGROUND);
    ImGui::SameLine(sameLineWidth);
    draw_checkbox_simple("Showdown", SHOWDOWN);

	ImGui::Spacing();
	ImGui::Separator();

	ImGui::Spacing();
    ImGui::Text("Dante");

    ImGui::Spacing();
	ImGui::Text("Swords");
	ImGui::Spacing();

	draw_checkbox_simple("Ecstasy", ECSTASY);
	ImGui::SameLine(sameLineWidth);
	draw_checkbox_simple("Kick 13", KICK13);

	draw_checkbox_simple("Prop", PROP);
	ImGui::SameLine(sameLineWidth);
	draw_checkbox_simple("Shock", SHOCK);

	draw_checkbox_simple("Slash Dimension", SLASH_DIMENSION);
	ImGui::SameLine(sameLineWidth);
	draw_checkbox_simple("DT Pin Up Part 2", DT_PIN_UP_P2);

	//draw_checkbox_simple("Beast Uppercut", BEAST_UPPERCUT);

	draw_checkbox_simple("Draw", DRAW);
	
	ImGui::Spacing();
	ImGui::Text("Guns");
	ImGui::Spacing();

	draw_checkbox_simple("Omen", OMEN);
	ImGui::SameLine(sameLineWidth);
	draw_checkbox_simple("Epidemic", EPIDEMIC);

	draw_checkbox_simple("Argument", ARGUMENT);
	ImGui::SameLine(sameLineWidth);
	draw_checkbox_simple("Gun Stinger", GUNSTINGER);
}

void SelectiveCancels::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("selective_cancels", selective_cancels_enable);
	cfg.set<uint32_t>("cancels", cancels);
	cfg.set<bool>("fix_guard_inertia", fixGuardInertia);
}

void SelectiveCancels::on_config_load(const utility::Config& cfg) {
	selective_cancels_enable = cfg.get<bool>("selective_cancels").value_or(false);
	cancels = cfg.get<uint32_t>("cancels").value_or(0);
	fixGuardInertia = cfg.get<bool>("fix_guard_inertia").value_or(false);
}

#endif
