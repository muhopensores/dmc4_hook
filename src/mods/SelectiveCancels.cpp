#include "SelectiveCancels.hpp"
#include "FireworksTweaks.hpp"

#if 1
bool      SelectiveCancels::mod_enabled_nero = false;
bool      SelectiveCancels::mod_enabled_dante = false;

uintptr_t SelectiveCancels::jmp_ret1 = 0x0080332F;
uintptr_t SelectiveCancels::jmp_ret2 = NULL;
uintptr_t SelectiveCancels::grief_jmp_ret1 = NULL;
uintptr_t SelectiveCancels::grief_jmp_ret2 = NULL;
static constexpr uintptr_t grief_detour2_jmp = 0x836FCF;
uint32_t  SelectiveCancels::cancels = 0;

constexpr uintptr_t static_mediator_ptr  = 0x00E558B8;
static bool good_grief = false;
static bool fixGuardInertia = false;
static float xmm0backup = 0.0f;

static float shockBuffer = 30.0f;
static float shockCancel = 50.0f;

static float kickThirteenBuffer = 20.0f;
static float kickThirteenCancel = 46.8f;

static float fireworksBuffer = 40.0f;
static float fireworksCancel = 64.0f;

naked void detour1() { // player in eax + edi
	_asm {
		// see if any cheats are even active
			push ecx
			mov cl, [SelectiveCancels::mod_enabled_nero]
			or cl, [SelectiveCancels::mod_enabled_dante]
			or cl, [FireworksTweaks::mod_enabled]
			test cl, cl
			pop ecx
			je originalcode
		// validate this is player 1
			push ecx
			mov ecx, [static_mediator_ptr]
			mov ecx, [ecx]
			test ecx, ecx
			mov ecx, [ecx+0x24]
			cmp eax, ecx
			pop ecx
			jne originalcode

			// after this, always use popcode
			movss [xmm0backup], xmm0
			movss xmm0, [eax+0x348]

			cmp byte ptr [eax+0x1494], 1 // controller id nero
			je CheckNero
		// Dante
			cmp dword ptr [eax+0x2998], 0x632 // Fireworks
			je Fireworks
			cmp byte ptr [SelectiveCancels::mod_enabled_dante], 0
			je popcode
			cmp dword ptr [eax+0x2998], 0x007 // Roll left
			je Roll
			cmp dword ptr [eax+0x2998], 0x008 // Roll right
			je Roll
			cmp dword ptr [eax+0x2998], 0x411 // Grounded Ecstasy
			je Ecstasy
			cmp dword ptr [eax+0x2998], 0x412 // Aerial Ecstasy
			je Ecstasy
			cmp dword ptr [eax+0x2998], 0x732 // Argument
			je Argument
			cmp dword ptr [eax+0x2998], 0x30E // Kick 13
			je KickThirteen
			cmp dword ptr [eax+0x2998], 0x30F // DT Kick 13
			je KickThirteen
			cmp dword ptr [eax+0x2998], 0x900 // Slash Dimension
			je SlashDimension
			cmp dword ptr [eax+0x2998], 0x232 // Prop
			je Prop
			cmp dword ptr [eax+0x2998], 0x333 // Shock
			je Shock
			cmp dword ptr [eax+0x2998], 0x735 // Omen
			je Omen
			cmp dword ptr [eax+0x2998], 0x635 // Gunstinger
			je Gunstinger
			cmp dword ptr [eax+0x2998], 0x706 // Epidemic
			je Epidemic
			cmp dword ptr [eax+0x2998], 0x410 // DT Pin Up part 2
			je DTPinUp
			cmp dword ptr [eax+0x2998], 0x310 // Draw
			je Draw
			cmp dword ptr [eax+0x2998], 0x20F // Stinger max level
			je Stinger
			cmp dword ptr [eax+0x2998], 0x335 // Real Impact
			je RealImpact
			cmp dword ptr [eax+0x2998], 0x31E // Flush
			je Flush
			cmp dword ptr [eax+0x2998], 0x31F // Flush forwards
			je Flush
			cmp dword ptr [eax+0x2998], 0x320 // Flush backwards
			je Flush
			jmp popcode

		CheckNero:
			cmp byte ptr [SelectiveCancels::mod_enabled_nero], 0
			je popcode
			cmp dword ptr [eax+0x2998], 0x007 // Roll left
			je Roll
			cmp dword ptr [eax+0x2998], 0x008 // Roll right
			je Roll
			cmp dword ptr [eax+0x2998], 0x33B // Nero Showdown
			je Showdown
			cmp dword ptr [eax+0x2998], 0x032 // Nero DT Ground
			je DTGround
			jmp popcode

	// Dante
		Ecstasy:
			test [SelectiveCancels::cancels], ECSTASY
			jne UsualCancel
			jmp popcode

		Argument:
			test [SelectiveCancels::cancels], ARGUMENT
			jne UsualCancel
			jmp popcode

		KickThirteen:
			test [SelectiveCancels::cancels], KICK13
			je popcode
			comiss xmm0, [kickThirteenBuffer]
			jb popcode
			comiss xmm0, [kickThirteenCancel]
			jb UsualBuffer
			jmp UsualCancel

		SlashDimension:
			test [SelectiveCancels::cancels], SLASH_DIMENSION
			jne UsualCancel
			jmp popcode

		Prop:
			test [SelectiveCancels::cancels], PROP
			jne UsualCancel
			jmp popcode

		Shock:
			test [SelectiveCancels::cancels], SHOCK
			je popcode
			comiss xmm0, [shockBuffer]
			jb popcode
			comiss xmm0, [shockCancel]
			jb UsualBuffer
			jmp UsualCancel

		Omen:
			test [SelectiveCancels::cancels], OMEN
			jne UsualCancel
			jmp popcode

		GunStinger:
			test [SelectiveCancels::cancels], GUNSTINGER
			jne UsualCancel
			jmp popcode

		Fireworks:
			cmp byte ptr [FireworksTweaks::mod_enabled], 0
			je popcode
			comiss xmm0, [fireworksBuffer]
			jb popcode
			comiss xmm0, [fireworksCancel]
			jb FireworksBufferChecks
		// Fireworks Cancel
			mov dword ptr [eax+0x3148], 2 // style cancel
			mov dword ptr [eax+0x31CC], 2 // gun cancel
			jmp popcode
		FireworksBufferChecks:
			mov dword ptr [eax+0x3148], 1 // style buffer
			mov dword ptr [eax+0x31CC], 1 // gun buffer
			jmp popcode

		Epidemic:
			test [SelectiveCancels::cancels], EPIDEMIC
			jne UsualCancel
			jmp popcode

		DTPinUp:
			test [SelectiveCancels::cancels], DT_PIN_UP_P2
			jne UsualCancel
			jmp popcode

		Draw:
			test [SelectiveCancels::cancels], DRAW
			jne UsualCancel
			jmp popcode

		Stinger:
			test [SelectiveCancels::cancels], STINGER
			jne UsualCancel
			jmp popcode

		RealImpact:
			test [SelectiveCancels::cancels], REAL_IMPACT
			jne UsualCancel
			jmp popcode

		Flush:
			test [SelectiveCancels::cancels], FLUSH
			jne MostCancel
			jmp popcode

	// Nero
		Showdown:
			test [SelectiveCancels::cancels], SHOWDOWN
			jne UsualCancel
			jmp popcode

		DTGround:
			test [SelectiveCancels::cancels], DTGROUND
			jne UsualCancel
			jmp popcode

	// Shared
		Roll:
			test [SelectiveCancels::cancels], ROLL
			jne UsualCancel
			jmp popcode
////////////////////////////////////////////////////////////////////////////////////////////////////
		UsualBuffer:
			mov dword ptr [eax+0x3174], 1 // jump / trickster dashes etc
			jmp popcode
		// MeleeCancel:
			mov dword ptr [eax+0x30C4], 2 // 2 is cancellable, 1 sets a buffer
			jmp popcode
		UsualCancel: // also trickster dash cancellable etc
			mov dword ptr [eax+0x3174], 2
			jmp popcode
		// StyleCancel:
			mov dword ptr [eax+0x3148], 2
			jmp popcode
		// GunCancel:
			mov dword ptr [eax+0x31CC], 2
			jmp popcode
		// WalkCancel:
			mov dword ptr [eax+0x31F8], 2
			jmp popcode
		MostCancel:
			mov dword ptr [eax+0x30C4], 2 // melee
			mov dword ptr [eax+0x3148], 2 // style
			mov dword ptr [eax+0x3174], 2 // jump / trickster dashes etc
			jmp popcode
		
		popcode:
			movss xmm0, [xmm0backup]
		originalcode:
			mov edi, 0x00000008
			jmp dword ptr [SelectiveCancels::jmp_ret1]
			// 0x30F0 // 
	}
}

naked void detour2() { // disable guardslide, only called on ground guard
	_asm {
			cmp byte ptr [SelectiveCancels::mod_enabled_dante], 0
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
            cmp byte ptr [SelectiveCancels::mod_enabled_dante], 0
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
			cmp byte ptr [SelectiveCancels::mod_enabled_dante], 0
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
	if (!install_hook_offset(0x3CBA06, hook2, &detour2, &jmp_ret2, 5)) { // disable guardslide, only called on ground guard
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

void SelectiveCancels::on_gui_frame(int display) {
	ImGui::PushID("SelectiveCancels");
	if (display == 1) {
		ImGui::Checkbox(_("Enable##SelectiveCancelsNero"), &mod_enabled_nero);
		ImGui::SameLine();
		help_marker(_("Allow cancelling out of selected moves with evasive actions"));
		if (mod_enabled_nero) {
			ImGui::Indent(lineIndent);
			draw_checkbox_simple(_("Roll"), ROLL);
			draw_checkbox_simple(_("Grounded DT Activation"), DTGROUND);
			draw_checkbox_simple(_("Showdown"), SHOWDOWN);
			ImGui::Unindent();
		}
	}
	else if (display == 2) {
		ImGui::Checkbox(_("Enable##SelectiveCancelsDante"), &mod_enabled_dante);
		ImGui::SameLine();
		help_marker(_("Allow cancelling out of selected moves with evasive actions"));
		if (mod_enabled_dante) {
			ImGui::Indent(lineIndent);
			ImGui::Checkbox(_("Disable Guardslide"), &fixGuardInertia);
			ImGui::SameLine();
			help_marker(_("Guarding a grounded move with momentum will halt your movement"));

			draw_checkbox_simple(_("Roll"), ROLL);

			draw_checkbox_simple(_("Flush"), FLUSH);

			ImGui::Spacing();
			ImGui::Text(_("Swords"));
			ImGui::Spacing();

			draw_checkbox_simple(_("Stinger"), STINGER);
			ImGui::SameLine(sameLineWidth + lineIndent);
			draw_checkbox_simple(_("Prop"), PROP);

			draw_checkbox_simple(_("Real Impact"), REAL_IMPACT);
			ImGui::SameLine(sameLineWidth + lineIndent);
			draw_checkbox_simple(_("Kick 13"), KICK13);
			ImGui::SameLine();
			help_marker(_("Cancellable from frame 38 (after the first 2 kicks)"));

			draw_checkbox_simple(_("Shock"), SHOCK);
			ImGui::SameLine();
			help_marker(_("Cancellable from frame 60"));
			ImGui::SameLine(sameLineWidth + lineIndent);
			draw_checkbox_simple(_("Ecstasy"), ECSTASY);

			draw_checkbox_simple(_("Slash Dimension"), SLASH_DIMENSION);
			ImGui::SameLine(sameLineWidth + lineIndent);
			draw_checkbox_simple(_("DT Pin Up Part 2"), DT_PIN_UP_P2);

			draw_checkbox_simple(_("Draw"), DRAW);

			ImGui::Spacing();
			ImGui::Text(_("Guns"));
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
			// ImGui::SameLine(sameLineWidth + lineIndent);
			ImGui::Unindent(lineIndent);
		}
	}
	ImGui::PopID();
}

void SelectiveCancels::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("selective_cancels_nero", mod_enabled_nero);
	cfg.set<bool>("selective_cancels_dante", mod_enabled_dante);
	cfg.set<uint32_t>("cancels", cancels);
	cfg.set<bool>("fix_guard_inertia", fixGuardInertia);
	cfg.set<bool>("good_grief", good_grief);
}

void SelectiveCancels::on_config_load(const utility::Config& cfg) {
	mod_enabled_nero = cfg.get<bool>("selective_cancels_nero").value_or(false);
	mod_enabled_dante = cfg.get<bool>("selective_cancels_dante").value_or(false);
	cancels = cfg.get<uint32_t>("cancels").value_or(0);
	fixGuardInertia = cfg.get<bool>("fix_guard_inertia").value_or(false);
	good_grief = cfg.get<bool>("good_grief").value_or(false);
	if (good_grief) griefToggle(good_grief);
}

#endif
