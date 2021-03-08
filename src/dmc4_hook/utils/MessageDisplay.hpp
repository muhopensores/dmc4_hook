#include "../imgui_dmc4.h"
#include <deque>

#define SHOW_MESSAGES()  MessageDisplay::instance().show_messages();
#define UPDATE_MESSAGE()  MessageDisplay::instance().update_messages();
#define DISPLAY_MESSAGE(format) MessageDisplay::instance().add_message(format);


struct message {

	message(std::string msg) {
		m_message = msg;
	}

	std::string m_message;
	float m_time = 100.0f;


	void show();
	// would be too fast for people playing on uncapped framerate
	// change m_time or fuck with deltaTime
	bool update(float dt);
};

struct MessageDisplay {
	std::deque<message> m_messages;

	// WOOO singletons, bad practice my ass
	static MessageDisplay& instance()
	{
		static MessageDisplay *instance = new MessageDisplay();
		return *instance;
	}

	void add_message(std::string msg);
	void show_messages();
	void update_messages();
private:
	MessageDisplay() {};
};