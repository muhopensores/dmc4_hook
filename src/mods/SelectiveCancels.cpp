
#include "SelectiveCancels.hpp"
#include "MoveIDs.hpp"
#include "MoveIDsNero.hpp"

#if 1
bool       SelectiveCancels::selective_cancels_enable = false;
uint32_t   SelectiveCancels::cancels = 0;
uintptr_t  SelectiveCancels::selective_cancels_continue = 0x0080332F;

naked void selective_cancels_proc() { // player in eax + edi
	_asm {
		cmp byte ptr [SelectiveCancels::selective_cancels_enable], 0
		je originalcode

		cmp byte ptr [esi+0x13C],0xFFFFFFFF
		jne originalcode
		cmp byte ptr [esi+0x144],0xFFFFFFFF
		jne originalcode

		cmp [MoveIds::move_id],0x411						// Grounded Ecstasy	// Checks move id like usual every tick
		je cancellableecstasy								// If correct moveid, check against Gui:
		cmp [MoveIds::move_id],0x412						// Aerial Ecstasy
		je cancellableecstasy
		cmp [MoveIds::move_id],0x732						// Argument
		je cancellableargument
		cmp [MoveIds::move_id],0x30E						// Kick 13
		je cancellablekickthirteen
		cmp [MoveIds::move_id],0x30F						// DT Kick 13
		je cancellablekickthirteen
		cmp [MoveIds::move_id],0x900						// Slash Dimension
		je cancellableslashdimension
		cmp [MoveIds::move_id],0x232						// Prop
		je cancellableprop
		cmp [MoveIds::move_id],0x333						// Shock
		je cancellableshock
		cmp [MoveIds::move_id],0x735						// Omen
		je cancellableomen
		cmp [MoveIds::move_id],0x635						// Gunstinger
		je cancellablegunstinger
		cmp [MoveIds::move_id],0x706						// Epidemic
		je cancellableepidemic
		cmp [MoveIds::move_id],0x410						// DT Pin Up part 2
		je cancellableDTPinUp
		cmp [MoveIdsNero::move_id_nero],0x33B				// Showdown
		je cancellableShowdown
		cmp [MoveIdsNero::move_id_nero],0x32				// DT Ground
		je cancellableDTGround
		cmp [MoveIds::move_id],0x310                        // Draw
		je cancellableDraw
		//cmp [MoveIds::move_id],0x332						// Beast Uppercut
		//je cancellableBeastUppercut
		jmp originalcode

			cancellableecstasy:
			test [SelectiveCancels::cancels], ECSTASY	// If Gui is ticked,
			jg cancellable								// make the move cancellable
			jmp originalcode							// if not, don't make it cancellable

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

			//cancellableBeastUppercut:
			//test [SelectiveCancels::cancels], BEAST_UPPERCUT
			//je originalcode
			//cmp [eax+0x348], 0x42040000 // (float)34 // address only writes when moves start so can't compare frames here rip
			//ja cancellable
			//jmp originalcode

			cancellable:
			mov dword ptr [esi+0x8C],0x02				// only movs to [esi+8C] after filtering out anything that doesn't have [esi+13C],FFFFFFFF and [esi+144],FFFFFFFF
														// a change of cmps would allow for different types of cancels such as cancelling an animation with walking or another attack
			originalcode:								// buffers are also used around this area - the je a few bytes down used to be an inconvenience
			mov edi,0x00000008							// originalcode has nothing to do with our newmem, just a convenient jmp point so always run
			jmp dword ptr [SelectiveCancels::selective_cancels_continue]
	}
}

std::optional<std::string> SelectiveCancels::on_initialize() {
	if (!install_hook_offset(0x40332A, detour, &selective_cancels_proc, 0, 6)) {
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

	ImGui::Spacing();

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
}

void SelectiveCancels::on_config_load(const utility::Config& cfg) {
	selective_cancels_enable = cfg.get<bool>("selective_cancels").value_or(false);
	cancels = cfg.get<uint32_t>("cancels").value_or(0);
}

#endif
