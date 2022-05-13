#include "MessageDisplay.hpp"

static bool shouldDisplay = true;

static bool enableGamepadNavigation = false;
static bool enableKeyboardNavigation = false;

void message::show() {
    if (shouldDisplay){
		ImGui::TextWrapped("%s (%f)",m_message.c_str(), m_time);
    }
}

bool message::update(float dt) {
	m_time -= dt;
	if (m_time <= 0.0f) {
		return false;
	}
	return true;
}

void MessageDisplay::add_message(std::string msg) {
	m_messages.push_front(message{ msg });
}

void MessageDisplay::show_messages()  {
	for (message& m : m_messages) {
		m.show();
	}
}

void MessageDisplay::update_messages() {
	for (message& m : m_messages)  {
		if (!m.update(0.1f)) {
			m_messages.pop_back();
		}
	}
}

std::optional<std::string> MessageDisplayMod::onInitialize() {
    return Mod::onInitialize();
}

void MessageDisplayMod::onGUIframe() {
    ImGui::Checkbox("Display Messages", &shouldDisplay);
    ImGui::SameLine();
    HelpMarker("Show/Hide popups at the top left such as \"Infinite Health (All) On\"");

    ImGui::Checkbox("Keyboard Navigation", &enableKeyboardNavigation);
    ImGui::SameLine(205);
    ImGui::Checkbox("Gamepad Navigation", &enableGamepadNavigation);

    // I would do this not on tick but idk how to use getIO in onConfigLoad without crashing
    if (enableKeyboardNavigation) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }
    else {
        ImGui::GetIO().ConfigFlags &= ImGuiConfigFlags_NavEnableKeyboard;
    }
    if (enableGamepadNavigation) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }
    else {
        ImGui::GetIO().ConfigFlags &= ImGuiConfigFlags_NavEnableGamepad;
    }
}

void MessageDisplayMod::onConfigLoad(const utils::Config& cfg) {
    shouldDisplay = cfg.get<bool>("display_messages").value_or(true);
    enableKeyboardNavigation = cfg.get<bool>("keyboard_navigation").value_or(false);
    enableGamepadNavigation = cfg.get<bool>("gamepad_navigation").value_or(false);
}

void MessageDisplayMod::onConfigSave(utils::Config& cfg) {
    cfg.set<bool>("display_messages", shouldDisplay);
    cfg.set<bool>("keyboard_navigation", enableKeyboardNavigation);
    cfg.set<bool>("gamepad_navigation", enableGamepadNavigation);
}
