// include your mod header file
#include "Twitch.hpp"
#include "../Mods.hpp"

static bool twitch_login_on_boot = false;

#if 1
// initialization
// return Mod::onInitialize(); on success or error string on failure.
std::optional<std::string> TwitchClient::on_initialize() {
	if (!dyn_link_lib_irc_client()) {
		//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll not found, skipping.");
		spdlog::info("[TwitchClient] libircclient.dll not found, skipping.");
	}
	libirc_loaded = true;
	//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll loaded.");
	spdlog::info("[TwitchClient] libircclient.dll loaded.");
	return Mod::on_initialize();
}

void TwitchClient::make_instance() {

	if ((twitch_status == TWITCH_CONNECTING) || (twitch_status == TWITCH_CONNECTED)) {
		return;
	}

	if (!twitch) {
		twitch = new Twitch();

		twitch->on_connected = [this] {
			//spdlog::info("[TwitchClient]: Connected to Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Connected to Twitch chat");
			twitch_status = TWITCH_CONNECTED;
		};

		twitch->on_disconnected = [this] {
			//spdlog::info("[TwitchClient]: Disconnected from Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Disconnected from Twitch chat");
			twitch_status = TWITCH_DISCONNECTED;
		};
		
		twitch->on_error = [this](int error_code, const std::string& error) {
			//spdlog::info("[TwitchClient]: Twitch Chat error %d: %s\n", errorCode, error.c_str());

			DISPLAY_MESSAGE(error.c_str());
		};

		twitch->on_message = [this](const std::string& sender, const std::string& message) {
			//spdlog::info("[TwitchClient]: Got message! %s: %s \n", sender.c_str(), message.c_str());
			if (mirror_chat_checkbox) {
				DISPLAY_MESSAGE(std::string{ sender + ": " + message });
			}
			std::size_t found = message.find("\\");
			if (found != std::string::npos) {
                auto& mods = g_framework->get_mods();
				mods->on_chat_command(message);
			}
		};
	}

	auto login = std::string{ twitch_login };
	auto password = std::string{ twitch_chat_oauth_password };
	if ( !login.empty() && !password.empty() ) {
		spdlog::info("[TwitchClient] Connecting to Twitch chat...\n");
		twitch_thread = twitch->connect( login, password);
		twitch_thread.detach();
		twitch_status = TWITCH_CONNECTING;
	}
}

void TwitchClient::disconnect() {
	if (twitch) {
		twitch->disconnect();
		delete twitch;
		twitch = nullptr;
	}
}

// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void TwitchClient::on_gui_frame() { 
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
		switch (twitch_status) {
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
			make_instance();
		} ImGui::SameLine();
		if (ImGui::Button("Disconnect From Twitch")) {
			disconnect();
		}
        ImGui::Checkbox("Log In On Game Boot Automatically", &twitch_login_on_boot);
		ImGui::SameLine();
        help_marker("This sometimes doesn't work, just come back here and hit disconnect > connect to reconnect");
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

void TwitchClient::on_config_save(utility::Config& cfg)
{
    cfg.set("twitch_login", twitch_login);
    cfg.set("twitch_oauth", twitch_chat_oauth_password);
    cfg.set<bool>("twitch_login_on_boot", twitch_login_on_boot);
}

// onConfigLoad
// load data into variables from config structure.
void TwitchClient::on_config_load(const utility::Config& cfg) {
    auto cfg_login = cfg.get("twitch_login").value_or("");
    auto cfg_oauth = cfg.get("twitch_oauth").value_or("");
    strcpy_s(twitch_login, sizeof(twitch_login), cfg_login.c_str());
    strcpy_s(twitch_chat_oauth_password, sizeof(twitch_chat_oauth_password), cfg_oauth.c_str());
    twitch_login_on_boot = cfg.get<bool>("twitch_login_on_boot").value_or(false);
    if (twitch_login_on_boot) {
        make_instance(); // sometimes gets stuck on connecting, says "IRC session terminated" 
    }
}

#endif
