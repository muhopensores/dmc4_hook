#pragma once

#include <deque>
#include <string>

#include "imgui/imgui.h"
#include "fw-imgui/imgui_impl_dx9.h"
#include "fw-imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

struct Message {

	Message(std::string msg) : m_message(std::move(msg)) {
	}
    
    Message(Message&& src) noexcept : m_message(std::move(src.m_message)) {
    }

	std::string m_message;
	float m_time = 100.0f;

	//void show();
	// would be too fast for people playing on uncapped framerate
	// change m_time or fuck with deltaTime
	bool update(float dt);
};

struct MessageDisplay {
	std::deque<Message> m_messages;

	// WOOO singletons, bad practice my ass
	static MessageDisplay& instance()
	{
		static MessageDisplay *instance = new MessageDisplay();
		return *instance;
	}

	void add_message(std::string msg);
	//void show_messages();
	void update_messages();

private:
	MessageDisplay() {};
};

//#define SHOW_MESSAGES()  MessageDisplay::instance().show_messages();
#define UPDATE_MESSAGE()  MessageDisplay::instance().update_messages();
#define DISPLAY_MESSAGE(format) MessageDisplay::instance().add_message(format);
