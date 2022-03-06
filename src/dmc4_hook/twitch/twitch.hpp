#pragma once

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <list>
#include "libircclient.h"
#include "libirc_rfcnumeric.h"


int dynLinkLibIRCclient();


struct TwitchContext;

enum TwitchConnectionStatus {
	TWITCH_DISCONNECTED,
	TWITCH_CONNECTING,
	TWITCH_CONNECTED
};

class Twitch {
public:
	Twitch();
	~Twitch();

	std::thread Connect( const std::string &user, const std::string &password);
	void Disconnect();

	void SendChatMessage( const std::string &message );

	std::string user;
	std::string channel;

	std::list<std::pair<std::string, std::string>> messages;

	TwitchConnectionStatus status = TWITCH_DISCONNECTED;

	std::function<void( const std::string &, const std::string & )> OnMessage = []( const std::string &, const std::string & ) {};
	std::function<void()> OnConnected = []{};
	std::function<void( int, const std::string & )> OnError = []( int, const std::string & ){};
	std::function<void()> OnDisconnected = []{};

private:
	irc_session_t *session = NULL;
};

struct TwitchContext {
	Twitch *twitch;
};