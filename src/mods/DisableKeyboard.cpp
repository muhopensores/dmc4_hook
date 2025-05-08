#include "DisableKeyboard.hpp"
#include "Console.hpp"
#if 1
bool DisableKeyboard::always_block_inputs = false;
uintptr_t DisableKeyboard::jmp_ret = NULL;
uintptr_t jmp_jl = 0x008E0AFE;
bool DisableKeyboard::auto_block_inputs = false;
static bool is_hook_open = false;

naked void detour(void) {
	_asm {
            cmp byte ptr [ImGuiConsole::is_open], 1
            je jmp_jl_code 

			cmp byte ptr [DisableKeyboard::always_block_inputs], 1
			je jmp_jl_code

            cmp byte ptr [DisableKeyboard::auto_block_inputs], 1
            je CheckHookOpenState

        originalcode:
            test eax, eax
			jl jmp_jl_code
        // retcode:
			jmp dword ptr [DisableKeyboard::jmp_ret]

        jmp_jl_code:
            jmp dword ptr [jmp_jl]

        CheckHookOpenState:
            cmp byte ptr [is_hook_open], 1
            je jmp_jl_code
            jmp originalcode
	}
}

std::optional<std::string> DisableKeyboard::on_initialize() {
    if (!install_hook_offset(0x4E0945, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init DisableKeyboard mod\n");
		return "Failed to init DisableKeyboard mod";
	}

    using v_key = std::vector<uint32_t>;
    utility::create_keyboard_hotkey(DisableKeyboard::m_hotkeys, {VK_SCROLL}, "Disable Keyboard Inputs", "disable_keyboard_inputs_hotkey");

    return Mod::on_initialize();
}

void DisableKeyboard::on_gui_frame(int display) {
    if (display == 1) {
        ImGui::Checkbox(_("Always Disable Keyboard Input"), &always_block_inputs);
        ImGui::SameLine();
        help_marker(_("Disable keyboard inputs throughout DMC4\nUseful when using \"Background Input\""));
    }
    else if (display == 2) {
        ImGui::Checkbox(_("Trainer Disables Keyboard Input"), &auto_block_inputs);
        ImGui::SameLine();
        help_marker(_("Disable keyboard inputs when opening the trainer"));
    }
}

void DisableKeyboard::on_game_pause(bool toggle) {
    is_hook_open = toggle;
}

void DisableKeyboard::on_update_input(utility::Input& input) {
    if (DisableKeyboard::m_hotkeys[0]->check(input)) {
        DisableKeyboard::always_block_inputs = !DisableKeyboard::always_block_inputs;
    }
}

void DisableKeyboard::on_config_load(const utility::Config& cfg) {
    always_block_inputs = cfg.get<bool>("disable_keyboard_always").value_or(false);
    auto_block_inputs = cfg.get<bool>("disable_keyboard_auto").value_or(false);
}

void DisableKeyboard::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_keyboard_always", always_block_inputs);
    cfg.set<bool>("disable_keyboard_auto", auto_block_inputs);
}

#endif
