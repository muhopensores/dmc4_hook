// include your mod header file
#include "modTwitch.hpp"
#include "../utils/MessageDisplay.hpp"
#include "../main.h"
#include "../Mods.hpp"
#include "hacklib/Logging.h"

static hlMain* g_main{};

#if 1
// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> TwitchClient::onInitialize() {
	g_main = GetMain();
#if 0
	// install_hook_offset takes offset from exe base, hl::Hooker instance,
	// pointer to naked function, pointer to return address and next instruction offset
	if (!install_hook_offset(0xBADF00D, hook, &detour, &jmp_return, 6)) {
		return "Failed to init ModName mod";
	}
	// some patch example
	if (mod_name_enabled) {
		patch.apply(0x400000, "\x90\x90\x90\x90");
	}
#endif
	return Mod::onInitialize();
}

// onFrame()
// do something each frame example
void TwitchClient::onFrame() {
	
}
// onConfigSave
// save your data into cfg structure.
void TwitchClient::onConfigSave(utils::Config& cfg) {
	cfg.set("twitch_login", twitch_login);
	cfg.set("twitch_oauth", twitch_chat_oauth_password);
};
// onConfigLoad
// load data into variables from config structure.
void TwitchClient::onConfigLoad(const utils::Config& cfg) {
	auto cfg_login = cfg.get("twitch_login").value_or("");
	auto cfg_oauth = cfg.get("twitch_oauth").value_or("");

	strcpy(twitch_login, cfg_login.c_str());
	strcpy(twitch_chat_oauth_password, cfg_oauth.c_str());

};

void TwitchClient::makeInstance() {
	if (!twitch) 
	{
		twitch = new Twitch();

		twitch->OnConnected = [this] {
			HL_LOG_RAW("[TwitchClient]: Connected to Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Connected to Twitch chat");
			twitchStatus = TWITCH_CONNECTED;
		};

		twitch->OnDisconnected = [this] {
			HL_LOG_RAW("[TwitchClient]: Disconnected from Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Disconnected from Twitch chat");
			twitchStatus = TWITCH_DISCONNECTED;
		};
		
		twitch->OnError = [this](int errorCode, const std::string& error) {
			HL_LOG_RAW("[TwitchClient]: Twitch Chat error %d: %s\n", errorCode, error.c_str());

			DISPLAY_MESSAGE(error.c_str());
		};

		twitch->OnMessage = [this](const std::string& sender, const std::string& message) {
			HL_LOG_RAW("[TwitchClient]: Got message! %s: %s \n", sender.c_str(), message.c_str());
			DISPLAY_MESSAGE(std::string{ sender + ": " + message });
			auto& mods = g_main->getMods();
			mods->onChatCommand(message);
		};
	}

	auto login = std::string{ twitch_login };
	auto password = std::string{ twitch_chat_oauth_password };
	if ( !login.empty() && !password.empty() ) {
		HL_LOG_RAW("[TwitchClient] Connecting to Twitch chat...\n");

		twitch_thread = twitch->Connect( login, password );
		twitch_thread.detach();
		twitchStatus = TWITCH_CONNECTING;
	}
}

void TwitchClient::disconnect()
{
	twitch->Disconnect();
	delete twitch;
	twitch = nullptr;

}

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void TwitchClient::onGUIframe() 
{ 
	if ( ImGui::CollapsingHeader( "Twitch Integration" ) ) {
		ImGui::TextWrapped(
			"Filling both fields and enabling one of the options below will allow "
			"your Twitch viewers to affect your gameplay.\n"
			"Your credentials are stored in config file in plaintext\n"
			"Dont send your config with twitch data saved to other people\n"
		);
		switch (twitchStatus) {
		case(TWITCH_CONNECTING):
			ImGui::Text("Twitch status: TWITCH_CONNECTING");
			break;
		case(TWITCH_CONNECTED):
			ImGui::Text("Twitch status: TWITCH_CONNECTED");
			break;
		default:
			ImGui::Text("Twitch status: TWITCH_DISCONNECTED");
			break;
		}
		ImGui::Text( "\n" );
		if(ImGui::Button("Connect to Twitch")) {
			makeInstance();
		} ImGui::SameLine();
		if (ImGui::Button("Disconnect from Twitch")) {
			disconnect();
		}
		ImGui::Text( "\n" );
		ImGui::InputText( "Twitch login", twitch_login, 128);
		ImGui::InputText( "Twitch chat OAuth password", twitch_chat_oauth_password, 128, ImGuiInputTextFlags_Password );

		if ( ImGui::Button( "Get OAuth password..." ) ) {
			ShellExecute( 0, 0, "https://twitchapps.com/tmi/", 0, 0, SW_SHOW );
		}
		ImGui::SameLine();
		// FIXME not implemented lmao use config
		/*if ( ImGui::Button( "Save login info" ) ) {
		}*/

		ImGui::Text( "\n" );
		// FIXME not implemented
		if ( ImGui::Checkbox( "Viewers can vote on random gameplay mods", &vote_checkbox ) ) {

		}
		// FIXME not implemented
		if ( vote_checkbox ) {
			if ( ImGui::RadioButton( "Voting affects which mod WILL BE activated next", &voting_result, 0 ) ) {
			}
			if ( ImGui::RadioButton( "Voting affects which mod IS MORE LIKELY to be activated next", &voting_result, 1 ) ) {
			}
		}
		// FIXME you guessed it
		if ( ImGui::Checkbox( "Relay Twitch chat to Devil May Cry 4", &mirror_chat_checkbox ) ) {
		}
	}
};
#endif