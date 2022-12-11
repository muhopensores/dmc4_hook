// include your mod header file
#include "Twitch.hpp"
#include "../Mods.hpp"
#include <thread>
#include <random>

static bool twitch_login_on_boot = false;

static TwitchClient* g_twc;
//static std::mutex g_mut{};

struct Voting {
    virtual ~Voting() {};
    virtual void tick(fmilliseconds& dt) {};
    virtual void pause(bool p) {};
    virtual TwitchClient::TWITCH_VOTING_STATE get_type() { return TwitchClient::TWITCH_VOTING_STATE::STATE_NONE; }
};

struct VotingState : public Voting {
    VotingState() { g_twc->m_voting_timer->start(); }
    void tick(fmilliseconds& dt) override {
        if (paused) { return; }
        g_twc->m_voting_timer->tick(dt);
    };
    void pause(bool p) override { paused = p; }
    virtual TwitchClient::TWITCH_VOTING_STATE get_type() { return TwitchClient::TWITCH_VOTING_STATE::STATE_VOTING; }
    bool paused{ false };
};

struct IdleState : public Voting {
    IdleState() { g_twc->m_idle_timer->start(); }
    void tick(fmilliseconds& dt) override {
        if (paused) { return; }
        g_twc->m_idle_timer->tick(dt);
    }
    void pause(bool p) override { paused = p; }
    virtual TwitchClient::TWITCH_VOTING_STATE get_type() { return TwitchClient::TWITCH_VOTING_STATE::STATE_IDLE; }
    bool paused{ false };
};

void VoteManager::on_chat_message(const std::string& sender, const std::string& msg) {
    if (msg.length() != 1) { return; }
    size_t sender_hash = std::hash<std::string>{}(sender);

    if (std::any_of(m_voters.begin(), m_voters.end(), [sender_hash](size_t i) { return i == sender_hash; })) {
        return;
    }

    m_voters.emplace_back(sender_hash);

    char fchar = msg.front();
    size_t index = 0;

    const std::array<std::pair<char, char>, 6> vote_tokens{
        std::make_pair('1','0'),
        std::make_pair('2','0'),
        std::make_pair('3','0'),
        std::make_pair('e', 'e'),
        std::make_pair('f', 'e'),
        std::make_pair('g', 'e') 
    };

    for (size_t i = 0; i <= vote_tokens.size(); i++) {
        if (vote_tokens[i].first == fchar) {
            index = (vote_tokens[i].first - vote_tokens[i].second);
        }
    }
    m_vote_entries[index].m_votes++;

    m_vote_distribution_display = m_vote_entries;
    std::sort(m_vote_distribution_display.begin(), m_vote_distribution_display.end(), [](VoteEntry a, VoteEntry b) { return a.m_votes > b.m_votes; });
}

static void twitch_voting_start() {
    //std::lock_guard <std::mutex> lock(g_mut);
    auto vmgr = g_twc->m_vote_manager;
    auto& gamemodes = MutatorRegistry::inst().m_mutators;
    std::vector<Mutator*> out;
    size_t n_elem = 3;
    std::sample(gamemodes.begin(), gamemodes.end(), std::back_inserter(out), n_elem, std::mt19937{ std::random_device{}() });
    // it was too late i remembered there was std::partial_sort ;_;

    for (size_t i = 0; i < out.size(); ++i) {
        spdlog::info("index: {}, name: {}", i, gamemodes[i]->m_name);

        vmgr->m_vote_entries.push_back(VoteEntry(out[i]));
    }

    for (size_t i = 0; i < vmgr->m_vote_entries.size(); i++) {
        if (vmgr->m_anti_anti_spam) {
            constexpr static std::array<char, 3> nums{ '0', '1', '2' };
            vmgr->m_vote_entries[i].m_token = nums[i];
        }
        else {
            constexpr static std::array<char, 3> chars{ 'e', 'f', 'g' };
            vmgr->m_vote_entries[i].m_token = chars[i];
        }
    }

    if (!g_twc->m_relay_voting_messages) {
        goto skip_messages;
    }

    g_twc->twitch->send_chat_message("VOTE FOR NEXT MOD");

    for (size_t i = 0; i < vmgr->m_vote_entries.size(); i++) {
        if (vmgr->m_anti_anti_spam) {
            g_twc->twitch->send_chat_message(fmt::format("{}: {}", i, vmgr->m_vote_entries[i].m_mod->m_name));
        }
        else {
            constexpr static std::array<char, 3> chars{'e', 'f', 'g' };
            g_twc->twitch->send_chat_message(fmt::format("{}: {}", chars[i], vmgr->m_vote_entries[i].m_mod->m_name));
        }
    }
skip_messages:
    vmgr->m_vote_distribution_display = vmgr->m_vote_entries;
    g_twc->twitch_vote_state = new IdleState();
    vmgr->m_anti_anti_spam = !(vmgr->m_anti_anti_spam);
}

static void twitch_voting_end() {
    //std::lock_guard <std::mutex> lock(g_mut);
    auto vmgr = g_twc->m_vote_manager;

    auto res = std::max_element(vmgr->m_vote_entries.begin(), vmgr->m_vote_entries.end(), [](VoteEntry a, VoteEntry b) { return a.m_votes < b.m_votes; });

    if (g_twc->m_relay_voting_messages) {
        g_twc->twitch->send_chat_message(fmt::format("VOTE ENDED : {}", (*res).m_mod->m_name));
    }

    if (g_twc->m_standalone) {
        DISPLAY_MESSAGE(fmt::format("ACTIVATING RANDOM MOD : {}", (*res).m_mod->m_name))
    }
    
    /*auto& res = vmgr->m_vote_entries[0];
    g_twc->twitch->send_chat_message(fmt::format("VOTE ENDED : {}", res.m_mod->m_name));*/
    MutatorRegistry::inst().activate_mod((*res).m_mod);
    strncpy_s(g_twc->m_winner, (*res).m_mod->m_name.c_str(), (*res).m_mod->m_name.length());

    vmgr->m_vote_entries.clear();
    vmgr->m_voters.clear();
    vmgr->m_vote_distribution_display.clear();

    //g_twc->twitch_vote_state = TwitchClient::STATE_VOTING;
    //g_twc->m_voting_timer->start();
    g_twc->twitch_vote_state = new VotingState();
}

void TwitchModeChaos::parse_message(const std::string & sender, const std::string & message) {
    //std::size_t found = message.find('\\');
    char fchar = message.front();
    if (fchar == '\\') {
        std::string s = std::string(std::begin(message) + 1, std::end(message));//message.substr(1, 64);
        spdlog::info("parse_msg: {}\n", s);
        MutatorRegistry::inst().activate_mod(s);
        //auto& mods = g_framework->get_mods();
        //mods->on_chat_command(message);
    }
}

void TwitchModeVoting::parse_message(const std::string & sender, const std::string & message) {
    if ((m_twc->twitch_vote_state->get_type() == TwitchClient::STATE_IDLE) && !m_twc->m_current_gameplay_state) {
        m_twc->m_vote_manager->on_chat_message(sender, message);
    }
}

std::optional<std::string> TwitchClient::on_initialize() {
    m_libirc = (HMODULE)dyn_link_lib_irc_client();

	if (!m_libirc) {
		//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll not found, skipping.");
		spdlog::info("[TwitchClient] libircclient.dll not found, skipping.");
	}
	libirc_loaded = true;
	//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll loaded.");
	spdlog::info("[TwitchClient] libircclient.dll loaded.");

	return Mod::on_initialize();
}

void TwitchClient::make_instance(bool standalone) {

	if ((twitch_status == TWITCH_CONNECTING) || (twitch_status == TWITCH_CONNECTED)) {
		return;
	}
    if (standalone) {

        voting_result = TwitchClient::TWITCH_MODE::VOTING;
        
        m_vote_disabled = false;
        m_vote_manager = new VoteManager();
        m_voting_timer  = new utility::Timer(float(2.0f), twitch_voting_start);
        m_idle_timer    = new utility::Timer(float(m_vote_time), twitch_voting_end);

        m_relay_voting_messages = false;
        m_standalone = true;
        g_twc = this;
        return;
    }
	if (!twitch) {
        g_twc = this;
        delete m_twitch_mode;

        if (voting_result == TwitchClient::TWITCH_MODE::VOTING) { m_twitch_mode = new TwitchModeVoting(this); m_vote_disabled = false; }
        else { m_twitch_mode = new TwitchModeChaos(this); }

		twitch = new Twitch();

        twitch->on_connected = [this] {
            // spdlog::info("[TwitchClient]: Connected to Twitch chat\n");
            DISPLAY_MESSAGE("[TwitchClient]: Connected to Twitch chat");
            twitch_status = TWITCH_CONNECTED;
            if (voting_result == TwitchClient::TWITCH_MODE::VOTING) {
                m_voting_timer = new utility::Timer(float(m_idle_time), twitch_voting_start);
                m_idle_timer   = new utility::Timer(float(m_vote_time), twitch_voting_end);
                m_vote_manager = new VoteManager();
            }
        };

		twitch->on_disconnected = [this] {
			//spdlog::info("[TwitchClient]: Disconnected from Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Disconnected from Twitch chat");
			twitch_status = TWITCH_DISCONNECTED;
		};
		
		twitch->on_error = [this](int error_code, const std::string& error) {
			//spdlog::info("[TwitchClient]: Twitch Chat error %d: %s\n", errorCode, error.c_str());

			DISPLAY_MESSAGE(error);
		};

		twitch->on_message = [this](const std::string& sender, const std::string& message) {
			//spdlog::info("[TwitchClient]: Got message! %s: %s \n", sender.c_str(), message.c_str());
			if (mirror_chat_checkbox) {
				DISPLAY_MESSAGE(std::string{ sender + ": " + message });
			}
            if (m_twitch_mode) {
                m_twitch_mode->parse_message(sender, message);
            }
		};
	}

	auto login = std::string{ twitch_login };
	auto password = std::string{ twitch_chat_oauth_password };
	if ( !login.empty() && !password.empty() ) {
        const std::lock_guard<std::mutex> lock(m_twitch_thread_lock);
		spdlog::info("[TwitchClient] Connecting to Twitch chat...\n");
		twitch_thread = twitch->connect( login, password);
		twitch_thread.detach();
		twitch_status = TWITCH_CONNECTING;
	}
}

void TwitchClient::disconnect() {
    const std::lock_guard<std::mutex> lock(m_twitch_thread_lock);
	if (twitch) {
		twitch->disconnect();
        twitch_status = TWITCH_DISCONNECTED;
	}
    memset(m_winner, 0, sizeof(m_winner));
    delete twitch;
    twitch = nullptr;
    delete m_voting_timer;
    m_voting_timer = nullptr;
    delete m_idle_timer;
    m_idle_timer = nullptr;
    delete m_twitch_mode;
    m_twitch_mode = nullptr;
    delete m_vote_manager;
    m_vote_manager = nullptr;
    g_twc = nullptr;
    m_vote_disabled = true;
    m_standalone = false;
}

void TwitchClient::stop_voting() {
    memset(m_winner, 0, sizeof(m_winner));
    delete twitch_vote_state;
    twitch_vote_state = nullptr;
    if (m_vote_manager) {
        m_vote_manager->m_vote_entries.clear();
        m_vote_manager->m_voters.clear();
        m_vote_manager->m_vote_distribution_display.clear();
    }
    if (m_idle_timer) {
        m_idle_timer->m_time = fseconds{ 0.0f };
    }
    if (m_voting_timer) {
        m_voting_timer->m_time = fseconds{ 0.0f };
    }
}

void TwitchClient::start_voting() {
    if (!devil4_sdk::is_not_in_gameplay()) {
        twitch_vote_state = new VotingState();
    }
}


// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void TwitchClient::on_gui_frame() { 
	if (!libirc_loaded) {
		ImGui::Text("libircclient.dll not found, twitch support disabled");
		return;
	}

    if (ImGui::CollapsingHeader("Random timed gameplay mods")) {
        if (m_standalone) {
            ImGui::Text("Random gameplay mods active");
            ImGui::Text("TIMER: %f", m_idle_timer->m_time.count());
            if (m_current_gameplay_state) {
                ImGui::Text("Not in gameplay");
            }
        }
        ImGui::Text("Set those before starting:");
        if (ImGui::InputInt("Mod timer (seconds)", &m_vote_time, 1, 10)) {
            m_vote_time = std::clamp<int32_t>(m_vote_time, 5, INT_MAX);
        }

        if (ImGui::Button("Random mutator mode")) {
            voting_result = VOTING;
            make_instance(true);
        } ImGui::SameLine();
        help_marker("Activate random spicy gameplay mods on timer without connecting to twitch");
        if (ImGui::Button("Stop random mode")) {
            disconnect();
        }
        if (!m_vote_disabled) {
            if (ImGui::Button("Stop timers")) {
                stop_voting();
                m_vote_disabled = true;
            }
        }
        else {
            if (ImGui::Button("Start timers")) {
                m_vote_disabled = false;
                start_voting();
            }
        }

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

        if (voting_result == VOTING) {
            if (twitch_status == TWITCH_CONNECTED) {
                if (!m_vote_disabled) {
                    if (ImGui::Button("Stop voting")) {
                        stop_voting();
                        m_vote_disabled = true;
                    }
                }
                else {
                    if (ImGui::Button("Start voting")) {
                        m_vote_disabled = false;
                        start_voting();
                    }
                }
            }

            ImGui::Checkbox("Show debug info in vote window", &m_twitch_vote_debug);
            ImGui::Checkbox("Send voting messages in chat", &m_relay_voting_messages);
            ImGui::Checkbox("Disable voting overlay", &m_disable_overlay);

;           ImGui::Text("Set those before connecting:");
            if (ImGui::InputInt("Voting timer (seconds)", &m_vote_time, 1, 10)) {
                m_vote_time = std::clamp<int32_t>(m_vote_time, 5, INT_MAX);
            }
            if (ImGui::InputInt("Idle timer (seconds)", &m_idle_time, 1, 10)) {
                m_idle_time = std::clamp<int32_t>(m_idle_time, 5, INT_MAX);
            }
        }

        ImGui::Checkbox("Log In On Game Boot Automatically", &twitch_login_on_boot);
		ImGui::SameLine();
        help_marker("This sometimes doesn't work, just come back here and hit disconnect > connect to reconnect");


        ImGui::Text("Twitch Mode FAQ");
        ImGui::TextWrapped("Chaos - viewers can activate twitch mods directly through chat commands.");
        ImGui::TextWrapped("Vote mode - viewers can vote on what mods to activate next.");

        if (ImGui::RadioButton("Chaos mode", &voting_result, TwitchClient::TWITCH_MODE::CHAOS)) {
            stop_voting();
            disconnect();
        }
        if (ImGui::RadioButton("Vote mode", &voting_result, TwitchClient::TWITCH_MODE::VOTING)) {
            disconnect();
        }
        ImGui::Checkbox("Relay Twitch Chat To Devil May Cry 4", &mirror_chat_checkbox);
	}
}

void TwitchClient::custom_imgui_window() {
    if (!libirc_loaded || m_disable_overlay) { return; }

    bool should_display_messages{ false };
    if (!twitch_vote_state) {
        should_display_messages = m_twitch_vote_debug;
    }
    else {
        should_display_messages = (twitch_vote_state->get_type() == STATE_IDLE) || m_twitch_vote_debug;
    }

    if (!should_display_messages || !(m_vote_manager) || m_standalone) {
        return;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImVec2 window_size = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.35f);

    ImVec2 window_pos = ImVec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 128));
    ImGui::Begin("Vote results", &should_display_messages, window_flags);
    if (m_twitch_vote_debug) {
        if (twitch_vote_state) {
            ImGui::Text("TWITCH_VOTING_STATE: %d", twitch_vote_state->get_type());
        }
        ImGui::Text("TWITCH_VOTE_TIMER: %f", m_idle_timer->m_time.count());
        ImGui::Text("TWITCH_IDLE_TIMER: %f", m_voting_timer->m_time.count());
    }

    ImGui::PushFont(g_framework->get_custom_imgui_font());
    if (m_current_gameplay_state) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(239, 211, 104, 255));
        ImGui::Text("VOTING PAUSED");
    }
    else {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(104, 239, 239, 255));
    }

    if (m_idle_timer->m_active)
        ImGui::Text("Vote in chat");
    //ImGui::Text("Token is in []");
    for (auto& entry : m_vote_manager->m_vote_distribution_display) {
        ImGui::Text("[%c] - %s:\t%d", entry.m_token, entry.m_mod->m_name.c_str(), entry.m_votes);
    }

    if (m_winner[0]) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(104, 239, 11, 255));
        ImGui::Text("%s", m_winner);
        ImGui::PopStyleColor();
    }
    ImGui::PopStyleColor();
    ImGui::PopFont();
    ImGui::End();
    ImGui::PopStyleColor();
}

void TwitchClient::on_config_save(utility::Config& cfg) {
    cfg.set("twitch_login", twitch_login);
    cfg.set("twitch_oauth", twitch_chat_oauth_password);
    cfg.set<bool>("twitch_login_on_boot", twitch_login_on_boot);
    cfg.set<int>("twitch_voting_mode", voting_result); // chaos, voting
    cfg.set<bool>("twitch_vote_debug_display", m_twitch_vote_debug);
    cfg.set<bool>("twitch_vote_relay_messages", m_relay_voting_messages);
    cfg.set<bool>("twitch_vote_disable_overlay", m_disable_overlay);
    cfg.set<float>("twitch_vote_time", m_vote_time);
    cfg.set<float>("twitch_idle_time", m_idle_time);

}
static bool g_previos_gameplay_state = false;

void TwitchClient::on_frame(fmilliseconds & dt) {
    if (twitch_status != TWITCH_CONNECTED && !m_standalone) { return; }
    MutatorRegistry::inst().update(dt);
    if ((voting_result == TwitchClient::TWITCH_MODE::CHAOS) || 
        m_vote_disabled) {
        return;
    }
    m_current_gameplay_state = devil4_sdk::is_not_in_gameplay();
    if (!twitch_vote_state) {
        if (g_previos_gameplay_state != m_current_gameplay_state) {
            if (!m_current_gameplay_state) {
                twitch_vote_state = new VotingState();
           }
        }
        g_previos_gameplay_state = m_current_gameplay_state;
        return;
    }

    if (g_previos_gameplay_state != m_current_gameplay_state) {
        twitch_vote_state->pause(m_current_gameplay_state);
    }
    twitch_vote_state->tick(dt);
    g_previos_gameplay_state = m_current_gameplay_state;
}

// onConfigLoad
// load data into variables from config structure.
void TwitchClient::on_config_load(const utility::Config& cfg) {
    auto cfg_login = cfg.get("twitch_login").value_or("");
    auto cfg_oauth = cfg.get("twitch_oauth").value_or("");
    strcpy_s(twitch_login, sizeof(twitch_login), cfg_login.c_str());
    strcpy_s(twitch_chat_oauth_password, sizeof(twitch_chat_oauth_password), cfg_oauth.c_str());
    m_twitch_vote_debug = cfg.get<bool>("twitch_vote_debug_display").value_or(false);
    m_relay_voting_messages = cfg.get<bool>("twitch_vote_relay_messages").value_or(false);
    m_disable_overlay = cfg.get<bool>("twitch_vote_disable_overlay").value_or(false);
    voting_result = cfg.get<int>("twitch_voting_mode").value_or(1); // chaos, voting
    m_vote_time = cfg.get<float>("twitch_vote_time").value_or(30.0f);
    m_idle_time = cfg.get<float>("twitch_idle_time").value_or(15.0f);
    twitch_login_on_boot = cfg.get<bool>("twitch_login_on_boot").value_or(false);
    if (twitch_login_on_boot) {
        //make_instance(); // sometimes gets stuck on connecting, says "IRC session terminated" 
        std::thread hehe([&] { 
            std::this_thread::sleep_for(std::chrono::seconds(1));
            make_instance(); // lets add an insult to injury and throw it into std::thread
                             // still gets stuck on CONNECTING sometimes, now just takes longer to try
                             // shortened timer because you could get into gameplay in 4 secs
        });
        hehe.detach(); // idk might help or make things worse
    }
}
