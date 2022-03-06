#include "MessageDisplay.hpp"

bool shouldDisplay = true;

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

void MessageDisplay::add_message(std::string msg)
{
	m_messages.push_front(message{ msg });
}

void MessageDisplay::show_messages() 
{
	for (message& m : m_messages) 
	{
		m.show();
	}
}

void MessageDisplay::update_messages()
{
	for (message& m : m_messages) 
	{
		if (!m.update(0.1f)) {
			m_messages.pop_back();
		}
	}
}

std::optional<std::string> MessageDisplayMod::onInitialize()
{
    return Mod::onInitialize();
}

void MessageDisplayMod::onGUIframe()
{
    ImGui::Checkbox("Display Messages", &shouldDisplay);
    ImGui::SameLine();
    HelpMarker("Show/Hide popups at the top left such as \"Infinite Health On\"");
}

void MessageDisplayMod::onConfigLoad(const utils::Config& cfg)
{
    shouldDisplay = cfg.get<bool>("display_messages").value_or(true);
};

void MessageDisplayMod::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("display_messages", shouldDisplay);
};
