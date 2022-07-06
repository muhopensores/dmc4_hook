
#include "SelectiveCancels.hpp"
#include "MoveIDs.hpp"
#include "MoveIDsNero.hpp"

#if 1
bool       SelectiveCancels::selective_cancels_enable = false;
uint32_t   SelectiveCancels::cancels = 0;
uintptr_t  SelectiveCancels::selective_cancels_continue = 0x0080332F;

naked void selective_cancels_proc() {
	_asm {
		cmp byte ptr [SelectiveCancels::selective_cancels_enable], 0
		je originalcode

		cmp byte ptr [esi+0x13C],0xFFFFFFFF
		jne originalcode
		cmp byte ptr [esi+0x144],0xFFFFFFFF
		jne originalcode

		cmp [MoveIds::move_id],0x411						// Grounded Ecstasy	// Checks move id like usual every tick
		je cancellableecstasy							// If correct moveid, check against Gui:
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
		cmp [MoveIdsNero::move_id_nero],0x33B // Showdown
		je cancellableShowdown
		jmp originalcode

			cancellableecstasy:
			test [SelectiveCancels::cancels], ECSTASY	// If Gui is ticked,
			jg cancellable						    // make the move cancellable
			jmp originalcode						// if not, don't make it cancellable

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

			cancellable:
		mov dword ptr [esi+0x8C],0x02					// only movs to [esi+8C] after filtering out anything that doesn't have [esi+13C],FFFFFFFF and [esi+144],FFFFFFFF
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

	if (ImGui::CollapsingHeader("Selective Cancel Toggles")){
			ImGui::Spacing();
			ImGui::Text("Swords");
			ImGui::Spacing();

			draw_checkbox_simple("Ecstasy", ECSTASY);
			ImGui::SameLine(205);
			draw_checkbox_simple("Kick 13", KICK13);

			draw_checkbox_simple("Prop", PROP);
			ImGui::SameLine(205);
			draw_checkbox_simple("Shock", SHOCK);

			draw_checkbox_simple("Slash Dimension", SLASH_DIMENSION);
			ImGui::SameLine(205);
			draw_checkbox_simple("DT Pin Up Part 2", DT_PIN_UP_P2);

			draw_checkbox_simple("Showdown", SHOWDOWN);

			ImGui::Spacing();
			ImGui::Text("Guns");
			ImGui::Spacing();

			draw_checkbox_simple("Omen", OMEN);
			ImGui::SameLine(205);
			draw_checkbox_simple("Epidemic", EPIDEMIC);

			draw_checkbox_simple("Argument", ARGUMENT);
			ImGui::SameLine(205);
			draw_checkbox_simple("Gun Stinger", GUNSTINGER);
	}
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();
}

void SelectiveCancels::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("selective_cancels", selective_cancels_enable);
	cfg.set<uint32_t>("cancels", cancels);
}

void SelectiveCancels::on_config_load(const utility::Config& cfg) {
	selective_cancels_enable = cfg.get<bool>("selective_cancels").value_or(false);
	cancels = cfg.get<uint32_t>("cancels").value_or(0);
}

#else
// using variable external to the module. In this case moveID.
// only definition to signal the linker.
extern "C" int moveID;

// mod namespace
namespace modSelCancels {

	// using powers of two here to avoid making alot of bool variables
	enum CANCEL_MOVES {       // bits table
		ECSTASY         = 1,  // 0000 0001
		ARGUMENT        = 2,  // 0000 0010
		KICK13          = 4,  // 0000 0100
		SLASH_DIMENSION = 8,  // 0000 1000
		PROP            = 16, // 0001 0000
		SHOCK           = 32, // 0010 0000
		OMEN            = 64  // 0100 0000
	};
	
	// we will use bitwise operations to get/set various bits
	// on a single cancels variable
	extern "C" { 
		bool selectiveCancelsEnable = false;
		uint32_t cancels = 0;
	};

	static uintptr_t  _selectiveCancelsContinue = 0x0080332F;
	static hl::Hooker detour;
	
	naked void selectiveCancels_proc(void)
	{
		_asm {
			cmp byte ptr [selectiveCancelsEnable], 0
			je originalcode

			cmp byte ptr [esi+0x13C],0xFFFFFFFF
			jne originalcode
			cmp byte ptr [esi+0x144],0xFFFFFFFF
			jne originalcode

			cmp [moveID],0x411				// Grounded Ecstasy	// Checks move id like usual every tick
			je cancellableecstasy				// If correct moveid, check against Gui:
			cmp [moveID],0x412						// Aerial Ecstasy
			je cancellableecstasy
			cmp [moveID],0x732						// Argument
			je cancellableargument
			cmp [moveID],0x30E						// Kick 13
			je cancellablekickthirteen
			cmp [moveID],0x30F						// DT Kick 13
			je cancellablekickthirteen
			cmp [moveID],0x900						// Slash Dimension
			je cancellableslashdimension
			cmp [moveID],0x232						// Prop
			je cancellableprop
			cmp [moveID],0x333						// Shock
			je cancellableshock
			cmp [moveID],0x735						// Omen
			je cancellableomen
			jmp originalcode

			cancellableecstasy:
			test [cancels], ECSTASY	// If Gui is ticked,
				jg cancellable						// make the move cancellable
				jmp originalcode						// if not, don't make it cancellable

				cancellableargument:
			test [cancels], ARGUMENT
				jg cancellable
				jmp originalcode

				cancellablekickthirteen:
			test [cancels], KICK13
				jg cancellable
				jmp originalcode

				cancellableslashdimension:
			test [cancels], SLASH_DIMENSION
				jg cancellable
				jmp originalcode

				cancellableprop:
			test [cancels], PROP
				jg cancellable
				jmp originalcode

				cancellableshock:
			test [cancels], SHOCK
				jg cancellable
				jmp originalcode

				cancellableomen:
			test [cancels], OMEN
				jg cancellable
				jmp originalcode

				cancellable:
			mov dword ptr [esi+0x8C],0x02	// only movs to [esi+8C] after filtering out anything that doesn't have [esi+13C],FFFFFFFF and [esi+144],FFFFFFFF
											// a change of cmps would allow for different types of cancels such as cancelling an animation with walking or another attack
				originalcode:								// buffers are also used around this area - the je a few bytes down used to be an inconvenience
				mov edi,0x00000008				// originalcode has nothing to do with our newmem, just a convenient jmp point so always run
				jmp dword ptr [_selectiveCancelsContinue]
		}
	}

	bool init(uintptr_t base) {
		uintptr_t selectiveCancelsAddress = base + 0x40332A;
		auto hook = detour.hookJMP(selectiveCancelsAddress, 6, &selectiveCancels_proc);
		if (!hook) {
			return false;
		}
		// if everything went well register our ini callbacks.
		// those will be called at ini load/save.
		/*loadCallbackRegister(loadConfig);
		saveCallbackRegister(saveConfig);*/

		return true;
	};
	void toggle(bool value) {
		selectiveCancelsEnable = value;
		modMoveIDs::toggle(value);
	};

	inline static void drawCheckboxSimple(const char* name, CANCEL_MOVES move) {
		// bitwise [AND cancels, move] to extract the bit that matters to us.
		bool check = (cancels & move) > 0;
		if (ImGui::Checkbox(name, &check))
		{
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
	};

	void onGUIframe() {
		if (ImGui::CollapsingHeader("Selective Cancels"))
		{
			if (ImGui::Checkbox("Enable", &selectiveCancelsEnable)) {
				toggle(selectiveCancelsEnable);
			}

			ImGui::Separator();
			ImGui::Text("Common");

			//drawCheckbox("Ecstasy", &checkEcstasy, ECSTASY);
			drawCheckboxSimple("Ecstasy", ECSTASY);

			ImGui::SameLine(198);

			//drawCheckbox("Argument", &checkArgument, ARGUMENT);
			drawCheckboxSimple("Argument", ARGUMENT);
			//drawCheckbox("Kick 13", &checkKick13, KICK13);
			drawCheckboxSimple("Kick 13", KICK13);

			ImGui::SameLine(198);

			//drawCheckbox("Slash Dimension", &checkSD, SLASH_DIMENSION);
			drawCheckboxSimple("Slash Dimension", SLASH_DIMENSION);

			ImGui::Separator();
			ImGui::Text("Swords");

			//drawCheckbox("Prop", &checkProp, PROP);
			drawCheckboxSimple("Prop", PROP);

			ImGui::SameLine(198);

			//drawCheckbox("Shock", &checkShock, SHOCK);
			drawCheckboxSimple("Shock", SHOCK);

			ImGui::Separator();
			ImGui::Text("Guns");

			//drawCheckbox("Omen", &checkOmen, OMEN);
			drawCheckboxSimple("Omen", OMEN);
		}
	};
	void onConfigSave(utility::Config& cfg) {
		cfg.set<bool>("selective_cancels", selectiveCancelsEnable);
		//config.player.options["selective_cancels"] = selectiveCancelsEnable;
		cfg.set<uint32_t>("cancels", cancels);
		/*
		config.player.options["ecstasy_cancel"]    = (cancels & ECSTASY) > 0;
		config.player.options["argument_cancel"]   = (cancels & ARGUMENT) > 0;
		config.player.options["kick13_cancel"]     = (cancels & KICK13) > 0;
		config.player.options["sd_cancel"]         = (cancels & SLASH_DIMENSION) > 0;
		config.player.options["prop_cancel"]       = (cancels & PROP) > 0;
		config.player.options["shock_cancel"]      = (cancels & SHOCK) > 0;
		config.player.options["omen_cancel"]       = (cancels & OMEN) > 0;
		*/
	};

	void onConfigLoad(const utility::Config& cfg) {
		selectiveCancelsEnable = cfg.get<bool>("selective_cancels").value_or(false);
		cancels = cfg.get<uint32_t>("cancels").value_or(0);
		toggle(selectiveCancelsEnable);
		/*
		//toggle(config.player.options["selective_cancels"]);
		if (config.player.options["ecstasy_cancel"])  { cancels |= ECSTASY; }
		if (config.player.options["argument_cancel"]) { cancels |= ARGUMENT; }
		if (config.player.options["kick13_cancel"])   { cancels |= KICK13; }
		if (config.player.options["sd_cancel"])       { cancels |= SLASH_DIMENSION;  }
		if (config.player.options["prop_cancel"])     { cancels |= PROP;  }
		if (config.player.options["shock_cancel"])    { cancels |= SHOCK;  }
		if (config.player.options["omen_cancel"])     { cancels |= OMEN;  }
		*/
	};

	void saveConfig(CONFIG& config) {
		config.player.options["selective_cancels"] = selectiveCancelsEnable;
		config.player.options["ecstasy_cancel"]    = (cancels & ECSTASY) > 0;
		config.player.options["argument_cancel"]   = (cancels & ARGUMENT) > 0;
		config.player.options["kick13_cancel"]     = (cancels & KICK13) > 0;
		config.player.options["sd_cancel"]         = (cancels & SLASH_DIMENSION) > 0;
		config.player.options["prop_cancel"]       = (cancels & PROP) > 0;
		config.player.options["shock_cancel"]      = (cancels & SHOCK) > 0;
		config.player.options["omen_cancel"]       = (cancels & OMEN) > 0;
	};

	void loadConfig(CONFIG& config) {
		toggle(config.player.options["selective_cancels"]);
		if (config.player.options["ecstasy_cancel"])  { cancels |= ECSTASY; }
		if (config.player.options["argument_cancel"]) { cancels |= ARGUMENT; }
		if (config.player.options["kick13_cancel"])   { cancels |= KICK13; }
		if (config.player.options["sd_cancel"])       { cancels |= SLASH_DIMENSION;  }
		if (config.player.options["prop_cancel"])     { cancels |= PROP;  }
		if (config.player.options["shock_cancel"])    { cancels |= SHOCK;  }
		if (config.player.options["omen_cancel"])     { cancels |= OMEN;  }
	};

};
#endif
