#pragma once

#if 1
#include "../mod.hpp"
#include "../twitch/twitch.hpp"
// Define ModName class that is a Mod
class TwitchClient : public Mod {
public:
	TwitchClient() = default;

	std::string getModName() override { return "TwitchClient"; };
	std::optional<std::string> onInitialize() override;

	// callbacks
	void onFrame() override;
	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;

	void makeInstance();
	void disconnect();

	bool vote_checkbox = false;
	bool mirror_chat_checkbox = false;
	bool use_ssl = false;
	int  voting_result = 0;

	Twitch* twitch;
	std::thread twitch_thread;
	TwitchConnectionStatus twitchStatus = TWITCH_DISCONNECTED;

private:
	char twitch_login[128]{};
	char twitch_chat_oauth_password[128]{};
};
#endif