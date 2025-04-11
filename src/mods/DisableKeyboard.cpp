#include "DisableKeyboard.hpp"
#include "Console.hpp"
#if 1
bool DisableKeyboard::mod_enabled{false};
uintptr_t DisableKeyboard::jmp_ret{ NULL };
uintptr_t jmp_jl{ 0x008E0AFE };

naked void detour(void) {
	_asm {
            cmp byte ptr [ImGuiConsole::is_open], 1
            je jmp_jl_code 

			cmp byte ptr [DisableKeyboard::mod_enabled], 1
			je jmp_jl_code

        // originalcode:
            test eax,eax
			jl jmp_jl_code
			jmp dword ptr [DisableKeyboard::jmp_ret]

        jmp_jl_code:
            jmp dword ptr [jmp_jl]
	}
}

std::optional<std::string> DisableKeyboard::on_initialize() {
    if (!install_hook_offset(0x4E0947, hook, &detour, &jmp_ret, 8)) {
		spdlog::error("Failed to init DisableKeyboard mod\n");
		return "Failed to init DisableKeyboard mod";
	}

    using v_key = std::vector<uint32_t>;
    utility::create_keyboard_hotkey(m_hotkeys, {VK_SCROLL}, "Disable Keyboard Inputs", "disable_keyboard_inputs_hotkey");

    return Mod::on_initialize();
}

void DisableKeyboard::on_gui_frame() {
    ImGui::Checkbox(_("Disable Keyboard Input"), &mod_enabled);
    ImGui::SameLine();
    help_marker(_("Disable keyboard inputs throughout DMC4\nUseful when using \"Background Input\""));
}

void DisableKeyboard::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        DisableKeyboard::mod_enabled = !DisableKeyboard::mod_enabled;
    }
}

void DisableKeyboard::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_keyboard").value_or(false);
}

void DisableKeyboard::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_keyboard", mod_enabled);
}

#endif
