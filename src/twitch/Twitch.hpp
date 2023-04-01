#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <list>

size_t dyn_link_lib_irc_client();

typedef struct irc_session_s irc_session_t;
struct TwitchContext;

enum TwitchConnectionStatus {
	TWITCH_DISCONNECTED,
	TWITCH_CONNECTING,
	TWITCH_CONNECTED
};

class Twitch {
public:
	Twitch() = default;
	~Twitch();

	std::thread connect( const std::string &user, const std::string &password);
	void disconnect();

	void send_chat_message( const std::string &message );

	std::string user;
	std::string channel;

	std::list<std::pair<std::string, std::string>> messages;

	TwitchConnectionStatus status = TWITCH_DISCONNECTED;

	std::function<void( const std::string &, const std::string & )> on_message = []( const std::string &, const std::string & ) {};
	std::function<void()> on_connected = []{};
	std::function<void( int, const char* )> on_error = []( int, const char* ){};
	std::function<void()> on_disconnected = []{};

private:
	irc_session_t *session = NULL;
};

struct TwitchContext {
	Twitch *twitch;
};