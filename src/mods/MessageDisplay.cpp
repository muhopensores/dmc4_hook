#include "MessageDisplay.hpp"

static bool should_display_messages    = true;
static bool enable_gamepad_navigation  = false;
static bool enable_keyboard_navigation = false;

std::optional<std::string> MessageDisplayMod::on_initialize() {
    return Mod::on_initialize();
}

void MessageDisplayMod::custom_imgui_window() {
    
    if (!should_display_messages) { return; }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoMouseInputs;

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGui::Begin("Overlay", &should_display_messages, window_flags);
    // pls someone with some taste help me pick better colors for this shit
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 201, 0, 255));
    UPDATE_MESSAGE();
    ImGui::PopStyleColor();
    ImGui::End();
}

void MessageDisplayMod::on_gui_frame() {
    ImGui::Checkbox("Display Messages", &should_display_messages);
    ImGui::SameLine();
    help_marker("Show/Hide popups at the top left such as \"Infinite Health (All) On\"");

    ImGui::Checkbox("Keyboard Navigation", &enable_keyboard_navigation);
    ImGui::SameLine(205);
    ImGui::Checkbox("Gamepad Navigation", &enable_gamepad_navigation);

    // I would do this not on tick but idk how to use getIO in onConfigLoad without crashing
    if (enable_keyboard_navigation) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
    else {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    }
    if (enable_gamepad_navigation) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }
    else {
        ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }
}

void MessageDisplayMod::on_config_load(const utility::Config& cfg) {
    should_display_messages    = cfg.get<bool>("display_messages").value_or(true);
    enable_gamepad_navigation  = cfg.get<bool>("gamepad_navigation").value_or(false);
    enable_keyboard_navigation = cfg.get<bool>("keyboard_navigation").value_or(false);
}

void MessageDisplayMod::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("display_messages",    should_display_messages);
    cfg.set<bool>("gamepad_navigation",  enable_gamepad_navigation);
    cfg.set<bool>("keyboard_navigation", enable_keyboard_navigation);
}
