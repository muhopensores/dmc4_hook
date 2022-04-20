// include your mod header file
#include "modTwitch.hpp"
#include "../utils/MessageDisplay.hpp"
#include "../main.h"
#include "../Mods.hpp"
#include "hacklib/Logging.h" // TODO: log's not working for some reason idk

static hlMain* g_main{};
bool twitchLoginOnBoot = false;

#if 1
// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> TwitchClient::onInitialize() {
	g_main = GetMain();
	if (!dynLinkLibIRCclient()) {
		//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll not found, skipping.");
		HL_LOG_RAW("[TwitchClient] libircclient.dll not found, skipping.");
	}
	libirc_loaded = true;
	//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll loaded.");
	HL_LOG_RAW("[TwitchClient] libircclient.dll loaded.");
	return Mod::onInitialize();
}

void TwitchClient::makeInstance() {

	if ((twitchStatus == TWITCH_CONNECTING) || (twitchStatus == TWITCH_CONNECTED)) {
		return;
	}

	if (!twitch) {
		twitch = new Twitch();

		twitch->OnConnected = [this] {
			//HL_LOG_RAW("[TwitchClient]: Connected to Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Connected to Twitch chat");
			twitchStatus = TWITCH_CONNECTED;
		};

		twitch->OnDisconnected = [this] {
			//HL_LOG_RAW("[TwitchClient]: Disconnected from Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Disconnected from Twitch chat");
			twitchStatus = TWITCH_DISCONNECTED;
		};
		
		twitch->OnError = [this](int errorCode, const std::string& error) {
			//HL_LOG_RAW("[TwitchClient]: Twitch Chat error %d: %s\n", errorCode, error.c_str());

			DISPLAY_MESSAGE(error.c_str());
		};

		twitch->OnMessage = [this](const std::string& sender, const std::string& message) {
			//HL_LOG_RAW("[TwitchClient]: Got message! %s: %s \n", sender.c_str(), message.c_str());
			if (mirror_chat_checkbox) {
				DISPLAY_MESSAGE(std::string{ sender + ": " + message });
			}
			std::size_t found = message.find("\\");
			if (found != std::string::npos) {
				auto& mods = g_main->getMods();
				mods->onChatCommand(message);
			}
		};
	}

	auto login = std::string{ twitch_login };
	auto password = std::string{ twitch_chat_oauth_password };
	if ( !login.empty() && !password.empty() ) {
		HL_LOG_RAW("[TwitchClient] Connecting to Twitch chat...\n");
		twitch_thread = twitch->Connect( login, password);
		twitch_thread.detach();
		twitchStatus = TWITCH_CONNECTING;
	}
}

void TwitchClient::disconnect() {
	if (twitch) {
		twitch->Disconnect();
		delete twitch;
		twitch = nullptr;
	}
}

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void TwitchClient::onGUIframe() { 
	if (!libirc_loaded) {
		ImGui::Text("libircclient.dll not found, twitch support disabled");
		return;
	}

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
		if ( ImGui::Button( "Get OAuth Password..." ) ) {
			ShellExecute( 0, 0, "https://twitchapps.com/tmi/", 0, 0, SW_SHOW );
		}
		ImGui::InputText( "Twitch Username", twitch_login, 128);
		ImGui::InputText( "Twitch OAuth", twitch_chat_oauth_password, 128, ImGuiInputTextFlags_Password );
		if(ImGui::Button("Connect To Twitch")) {
			makeInstance();
		} ImGui::SameLine();
		if (ImGui::Button("Disconnect From Twitch")) {
			disconnect();
		}
        ImGui::Checkbox("Log In On Game Boot Automatically", &twitchLoginOnBoot);
		ImGui::SameLine();
        HelpMarker("This sometimes doesn't work, just come back here and hit disconnect > connect to reconnect");
		// ImGui::SameLine();
		// FIXME not implemented lmao use config
		/*if ( ImGui::Button( "Save login info" ) ) {
		}*/
		/*
		ImGui::Text( "\n" );
		// FIXME not implemented
		if ( ImGui::Checkbox( "Viewers Can Vote On Random Gameplay Mods", &vote_checkbox ) ) {

		}
		// FIXME not implemented
		if ( vote_checkbox ) {
			if ( ImGui::RadioButton( "Voting Affects Which Mod WILL BE Activated Next", &voting_result, 0 ) ) {
			}
			if ( ImGui::RadioButton( "Voting Affects Which Mod IS MORE LIKELY To Be Activated next", &voting_result, 1 ) ) {
			}
		}*/
		// FIXME you guessed it
		if ( ImGui::Checkbox( "Relay Twitch Chat To Devil May Cry 4", &mirror_chat_checkbox ) ) {
		}
	}
}

void TwitchClient::onConfigSave(utils::Config& cfg)
{
    cfg.set("twitch_login", twitch_login);
    cfg.set("twitch_oauth", twitch_chat_oauth_password);
    cfg.set<bool>("twitch_login_on_boot", twitchLoginOnBoot);
}

// onConfigLoad
// load data into variables from config structure.
void TwitchClient::onConfigLoad(const utils::Config& cfg) {
    auto cfg_login = cfg.get("twitch_login").value_or("");
    auto cfg_oauth = cfg.get("twitch_oauth").value_or("");
    strcpy(twitch_login, cfg_login.c_str());
    strcpy(twitch_chat_oauth_password, cfg_oauth.c_str());
    twitchLoginOnBoot = cfg.get<bool>("twitch_login_on_boot").value_or(false);
    if (twitchLoginOnBoot) {
        makeInstance(); // sometimes gets stuck on connecting, says "IRC session terminated" 
    }
}

#endif
