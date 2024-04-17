#include "DisableKeyboard.hpp"

#if 1
bool DisableKeyboard::mod_enabled{false};

std::optional<std::string> DisableKeyboard::on_initialize() {
    using v_key = std::vector<uint32_t>;
    m_hotkeys.emplace_back(std::make_unique<utility::Hotkey>(v_key{VK_SCROLL}, "Disable Keyboard Inputs", "disable_keyboard_inputs_hotkey"));

    return Mod::on_initialize();
}

void DisableKeyboard::toggle(bool enable) {
    if (enable) {
        install_patch_offset(0x4E0947, patch, "\xE9\xB2\x01\x00\x00\x90", 6); // E9 B2 01 00 00 90 jmp 008E0AFE
    } else {
        patch.reset(); // 0F 8C B1 01 00 00 jl 008E0AFE
    }
}
void DisableKeyboard::on_gui_frame() {
    if (ImGui::Checkbox("Disable Keyboard Input", &mod_enabled)) {
        toggle(mod_enabled);
    }
    ImGui::SameLine();
    help_marker("Disables your keyboard, useful when using background input");
}

void DisableKeyboard::on_update_input(utility::Input& input) {
    if (m_hotkeys[0]->check(input)) {
        DisableKeyboard::mod_enabled = !DisableKeyboard::mod_enabled;
        toggle(mod_enabled);
    }
}

void DisableKeyboard::on_config_load(const utility::Config& cfg) {
    mod_enabled = cfg.get<bool>("disable_keyboard").value_or(false);
    toggle(mod_enabled);
}

void DisableKeyboard::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("disable_keyboard", mod_enabled);
}

#endif
