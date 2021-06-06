#include "MessageDisplay.hpp"

void message::show() {
	ImGui::TextWrapped("%s (%f)",m_message.c_str(), m_time);
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
