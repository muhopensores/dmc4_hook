#include "SelectiveCancels.hpp"

#if 1
bool      SelectiveCancels::mod_enabled = false;
uintptr_t SelectiveCancels::jmp_ret1 = 0x0080332F;
uintptr_t SelectiveCancels::jmp_ret2 = NULL;
uintptr_t SelectiveCancels::grief_jmp_ret1 = NULL;
uintptr_t SelectiveCancels::grief_jmp_ret2 = NULL;
constexpr uintptr_t grief_detour2_jmp = 0x836FCF;
bool good_grief = false;

constexpr uintptr_t static_mediator_ptr  = 0x00E558B8;
uint32_t  SelectiveCancels::cancels = 0;
bool fixGuardInertia = false;

naked void detour1() { // player in eax + edi
	_asm {
			cmp byte ptr [SelectiveCancels::mod_enabled], 0
			je originalcode

		// validate this is player 1
			push ecx
			mov ecx, [static_mediator_ptr]
			mov ecx, [ecx]
			mov ecx, [ecx+0x24]
			cmp eax, ecx
			pop ecx
			jne originalcode

		// Dante
			cmp dword ptr [eax+0x1494], 0 // Dante
			jne CheckNero
			cmp dword ptr [eax+0x2998], 0x411 // Grounded Ecstasy
			je CancellableEcstasy
			cmp dword ptr [eax+0x2998], 0x412 // Aerial Ecstasy
			je CancellableEcstasy
			cmp dword ptr [eax+0x2998], 0x732 // Argument
			je CancellableArgument
			cmp dword ptr [eax+0x2998], 0x30E // Kick 13
			je CancellableKickThirteen
			cmp dword ptr [eax+0x2998], 0x30F // DT Kick 13
			je CancellableKickThirteen
			cmp dword ptr [eax+0x2998], 0x900 // Slash Dimension
			je CancellableSlashDimension
			cmp dword ptr [eax+0x2998], 0x232 // Prop
			je CancellableProp
			cmp dword ptr [eax+0x2998], 0x333 // Shock
			je CancellableShock
			cmp dword ptr [eax+0x2998], 0x735 // Omen
			je CancellableOmen
			cmp dword ptr [eax+0x2998], 0x635 // Gunstinger
			je CancellableGunstinger
			cmp dword ptr [eax+0x2998], 0x706 // Epidemic
			je CancellableEpidemic
			cmp dword ptr [eax+0x2998], 0x410 // DT Pin Up part 2
			je CancellableDTPinUp
			cmp dword ptr [eax+0x2998], 0x310 // Draw
			je CancellableDraw
			cmp dword ptr [eax+0x2998], 0x007 // Roll left
			je CancellableRoll
			cmp dword ptr [eax+0x2998], 0x008 // Roll right
			je CancellableRoll
			cmp dword ptr [eax+0x2998], 0x20F // Stinger max level
			je CancellableStinger
			cmp dword ptr [eax+0x2998], 0x335 // Real Impact
			je CancellableRealImpact
			cmp dword ptr [eax+0x2998], 0x31E // Flush
			je CancellableFlush
			cmp dword ptr [eax+0x2998], 0x31F // Flush forwards
			je CancellableFlush
			cmp dword ptr [eax+0x2998], 0x320 // Flush backwards
			je CancellableFlush
			jmp originalcode

		CheckNero:
			cmp dword ptr [eax+0x2998], 0x33B // Nero Showdown
			je CancellableShowdown
			cmp dword ptr [eax+0x2998], 0x032 // Nero DT Ground
			je CancellableDTGround
			cmp dword ptr [eax+0x2998], 0x007 // Roll left
			je CancellableRoll
			cmp dword ptr [eax+0x2998], 0x008 // Roll right
			je CancellableRoll
			jmp originalcode

	// Dante
		CancellableEcstasy:
			test [SelectiveCancels::cancels], ECSTASY
			jne JumpCancellable
			jmp originalcode

		CancellableArgument:
			test [SelectiveCancels::cancels], ARGUMENT
			jne JumpCancellable
			jmp originalcode

		CancellableKickThirteen:
			cmp dword ptr [eax+0x348], 0x41c80000 // 25.0f
			jb originalcode
			test [SelectiveCancels::cancels], KICK13
			jne MeleeCancellable
			jmp originalcode

		CancellableSlashDimension:
			test [SelectiveCancels::cancels], SLASH_DIMENSION
			jne JumpCancellable
			jmp originalcode

		CancellableProp:
			test [SelectiveCancels::cancels], PROP
			jne JumpCancellable
			jmp originalcode

		CancellableShock:
			cmp dword ptr [eax+0x348], 0x42200000 // 40.0f
			jb originalcode
			test [SelectiveCancels::cancels], SHOCK
			jne JumpCancellable
			jmp originalcode

		CancellableOmen:
			test [SelectiveCancels::cancels], OMEN
			jne JumpCancellable
			jmp originalcode

		CancellableGunStinger:
			test [SelectiveCancels::cancels], GUNSTINGER
			jne JumpCancellable
			jmp originalcode

		CancellableEpidemic:
			test [SelectiveCancels::cancels], EPIDEMIC
			jne JumpCancellable
			jmp originalcode

		CancellableDTPinUp:
			test [SelectiveCancels::cancels], DT_PIN_UP_P2
			jne JumpCancellable
			jmp originalcode

		CancellableDraw:
			test [SelectiveCancels::cancels], DRAW
			jne JumpCancellable
			jmp originalcode

		CancellableStinger:
			test [SelectiveCancels::cancels], STINGER
			jne JumpCancellable
			jmp originalcode

		CancellableRealImpact:
			test [SelectiveCancels::cancels], REAL_IMPACT
			jne JumpCancellable
			jmp originalcode

		CancellableFlush:
			test [SelectiveCancels::cancels], FLUSH
			jne AllCancellable
			jmp originalcode

	// Nero
		CancellableShowdown:
			test [SelectiveCancels::cancels], SHOWDOWN
			jne JumpCancellable
			jmp originalcode

		CancellableDTGround:
			test [SelectiveCancels::cancels], DTGROUND
			jne JumpCancellable
			jmp originalcode

	// Shared
		CancellableRoll:
			test [SelectiveCancels::cancels], ROLL
			jne JumpCancellable
			jmp originalcode

	// End
		MeleeCancellable:
			mov dword ptr [eax+0x30C4], 0x02 // 2 is cancellable, 1 sets a buffer
			jmp originalcode
		JumpCancellable: // also trickster dash cancellable etc
			mov dword ptr [eax+0x3174], 0x02
			jmp originalcode
		// StyleCancellable:
			mov dword ptr [eax+0x3148], 0x02
			jmp originalcode
		// GunCancellable:
			mov dword ptr [eax+0x31CC], 0x02
			jmp originalcode
		// Walkancellable:
			mov dword ptr [eax+0x31F8], 0x02
			jmp originalcode
		AllCancellable:
			mov dword ptr [eax+0x30C4], 0x02 // melee
			mov dword ptr [eax+0x3148], 0x02 // style
			mov dword ptr [eax+0x3174], 0x02 // jump / trickster dashes etc
			jmp originalcode
		originalcode:
			mov edi, 0x00000008
			jmp dword ptr [SelectiveCancels::jmp_ret1]
			// 0x30F0 // 
	}
}

naked void detour2() { // only called on ground guard
	_asm {
			cmp byte ptr [SelectiveCancels::mod_enabled], 0
			je originalcode
			cmp byte ptr [fixGuardInertia], 0
			je originalcode

			mov dword ptr [ecx+0xEC0], 0 // x
			mov dword ptr [ecx+0xEC8], 0 // z
		originalcode:
			push 0x00000132
			jmp dword ptr [SelectiveCancels::jmp_ret2]
    }
}

naked void grief_detour1() {
    _asm {
            cmp byte ptr [SelectiveCancels::mod_enabled], 0
            je originalcode
			cmp byte ptr [good_grief], 0
			je originalcode

            mov byte ptr [ebp+0x3174], 2 // movement abilities cancel
            mov byte ptr [ebp+0x30C4], 2 // melee cancel
            mov byte ptr [ebp+0x31CC], 2 // gun cancel
            mov byte ptr [ebp+0x3148], 2 // directional melee cancel
            mov byte ptr [ebp+0x30F0], 2
        originalcode:
            movss xmm0, [ebp+0x0000177C]
            jmp [SelectiveCancels::grief_jmp_ret1]
    }
}

naked void grief_detour2() { // janky as all hell, but it works
    _asm {
			cmp byte ptr [SelectiveCancels::mod_enabled], 0
			je originalcode
            cmp byte ptr [good_grief], 0
			je originalcode
            
            cmp dword ptr [eax+0x1564], 0x59
            je originalcode

            test word ptr [eax+0x140C], 0x2 // input held
            jne handler2

            cmp dword ptr [eax+0x1DB8],8 // weapon
            jne handler

            test word ptr [eax+0x140C], 0x200 // input held
            je handler

            cmp byte ptr [eax+0x14D98], 1 // style
            jne handler
            
            jmp [SelectiveCancels::grief_jmp_ret2]

        originalcode:
            cmp dword ptr [eax+0x1564], 0x59
            je handler 
            jmp [SelectiveCancels::grief_jmp_ret2]
        handler:
            jmp [grief_detour2_jmp]
        handler2:
            cmp dword ptr [eax+0x1DB8], 8 // weapon
            jne handler
            jmp originalcode
    }
}

void SelectiveCancels::griefToggle(bool enable) {
    if (enable) {
        install_patch_offset(0x436FAF, patch1, "\x90\x90",2);
        install_patch_offset(0x436FB8, patch2, "\x90\x90",2);
    }
    else {
        patch1.reset();
        patch2.reset();
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
	if (!install_hook_offset(0x3DF715, hookGrief, &grief_detour1, &grief_jmp_ret1, 8)) {
		spdlog::error("Failed to init GoodGrief mod\n");
		return "Failed to init GoodGrief mod";
	}
    if (!install_hook_offset(0x436FB1, hookGrief2, &grief_detour2, &grief_jmp_ret2, 7)) {
		spdlog::error("Failed to init GoodGrief mod\n");
		return "Failed to init GoodGrief mod2";
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
	ImGui::Text(_("Selective Cancels"));
	ImGui::Spacing();
	ImGui::Checkbox(_("Enable"), &mod_enabled);
	ImGui::SameLine();
	help_marker(_("Allows cancelling out of selected moves with evasive actions"));
	ImGui::SameLine(sameLineWidth);
	ImGui::Checkbox(_("Disable Guardslide"), &fixGuardInertia);
	ImGui::SameLine();
	help_marker(_("Guarding a grounded move with momentum will halt your movement"));

	if (mod_enabled) {
		ImGui::Indent(lineIndent);

		ImGui::Spacing();
		ImGui::Text(_("Shared Abilities"));
		ImGui::Spacing();

		draw_checkbox_simple(_("Roll"), ROLL);

		ImGui::Spacing();
		ImGui::Text(_("Nero Abilities"));
		ImGui::Spacing();

		draw_checkbox_simple(_("Grounded DT Activation"), DTGROUND);
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("Showdown"), SHOWDOWN);

		ImGui::Spacing();
		ImGui::Text(_("Dante Abilities"));
		ImGui::Spacing();

		draw_checkbox_simple(_("Flush"), FLUSH);

		ImGui::Spacing();
		ImGui::Text(_("Dante Swords"));
		ImGui::Spacing();

		draw_checkbox_simple(_("Stinger"), STINGER);
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("Prop"), PROP);

		draw_checkbox_simple(_("Real Impact"), REAL_IMPACT);
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("Kick 13"), KICK13);
		ImGui::SameLine();
		help_marker(_("Melee Cancellable from frame 25 (after the first 2 kicks)"));

		draw_checkbox_simple(_("Shock"), SHOCK);
		ImGui::SameLine();
		help_marker(_("Cancellable from frame 40"));
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("Ecstasy"), ECSTASY);

		draw_checkbox_simple(_("Slash Dimension"), SLASH_DIMENSION);
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("DT Pin Up Part 2"), DT_PIN_UP_P2);

		draw_checkbox_simple(_("Draw"), DRAW);

		ImGui::Spacing();
		ImGui::Text(_("Dante Guns"));
		ImGui::Spacing();

		draw_checkbox_simple(_("Omen"), OMEN);
		ImGui::SameLine();
		help_marker(_("Opening Pandora"));
		ImGui::SameLine(sameLineWidth + lineIndent);
		draw_checkbox_simple(_("Epidemic"), EPIDEMIC);
		ImGui::SameLine();
		help_marker(_("Pandora Ground shot 1"));

		draw_checkbox_simple(_("Argument"), ARGUMENT);
		ImGui::SameLine();
		help_marker(_("Gunship"));
		ImGui::SameLine(sameLineWidth + lineIndent);
		if (ImGui::Checkbox(_("Grief"), &good_grief)) {
			griefToggle(good_grief);
		}
		ImGui::SameLine();
		help_marker(_("Cancel out of Grief mid-throw animation without recalling Pandora"));
		
		draw_checkbox_simple(_("Gun Stinger"), GUNSTINGER);

		ImGui::Unindent(lineIndent);
	}
}

void SelectiveCancels::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("selective_cancels", mod_enabled);
	cfg.set<uint32_t>("cancels", cancels);
	cfg.set<bool>("fix_guard_inertia", fixGuardInertia);
	cfg.set<bool>("good_grief", good_grief);
}

void SelectiveCancels::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("selective_cancels").value_or(false);
	cancels = cfg.get<uint32_t>("cancels").value_or(0);
	fixGuardInertia = cfg.get<bool>("fix_guard_inertia").value_or(false);
	good_grief = cfg.get<bool>("good_grief").value_or(false);
	griefToggle(good_grief);
}

#endif
