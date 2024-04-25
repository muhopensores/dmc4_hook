// include your mod header file
#include "Twitch.hpp"
#include "../Mods.hpp"
#include <thread>
#include <random>
#include <optional>
#include "GuiFunctions.hpp"

static bool twitch_login_on_boot = false;

static TwitchClient* g_twc;

using VoteQueueEntry = std::pair<const std::string, const std::string>;

struct Voting {
    virtual ~Voting() {};
    virtual void tick(fmilliseconds& dt) {};
    virtual void pause(bool p) {};
    virtual void on_message(const std::string& sender, const std::string& message) {};
    virtual void on_imgui(bool draw_debug_info) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 139, 0, 255));
        ImGui::TextWrapped("STATE_NONE. Enter gameplay or pause/unpause the game to start.");
        ImGui::PopStyleColor();
    };
    static void imgui_draw_winner(VoteManager* vote_manager) {
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(104, 239, 11, 255));
        if (!vote_manager->m_previous_winner) {
            ImGui::PopStyleColor();
            return;
        }
        auto winner_raw_ptr = vote_manager->m_previous_winner.get();

        ImGui::TextWrapped("Winner: %s", winner_raw_ptr->m_mod->m_name.c_str());
        //ImGui::Text("%s", winner_raw_ptr->m_mod->m_description.c_str());
        ImGui::Text("with %d votes", winner_raw_ptr->m_votes);
        float time_left = mutator_is_mod_active(winner_raw_ptr->m_mod);
        if (time_left >= 0.0f) {
            ImGui::TextWrapped("currently active. time left: %.2f", time_left);
        }
        ImGui::PopStyleColor();
    }
    virtual TwitchClient::TWITCH_VOTING_STATE get_type() { return TwitchClient::TWITCH_VOTING_STATE::STATE_NONE; }
};

struct VotingState : public Voting {
    VotingState() : 
        m_timer(g_twc->m_vote_manager->m_voting_timer.get()) { 
        assert((g_twc != nullptr) && "g_twc is NULL!");
        m_vote_manager = g_twc->m_vote_manager;
        m_timer->start(); 
    }
    ~VotingState() {
        m_timer->stop();
    };
    void tick(fmilliseconds& dt) override {
        assert((m_timer != nullptr) && "vote timer is NULL!");
        m_timer->tick(dt);
    };
    void pause(bool p) override {
        assert(g_twc != nullptr && "g_twc is NULL!");
        assert(m_vote_manager && "VoteManager is NULL!");

        if (p == m_paused) {
            for (auto& entry : m_queue) {
                m_vote_manager->on_chat_message(entry.first, entry.second);
            }
            m_queue.clear();
        }
        m_paused = !p; 
        m_timer->m_active = p;
    }
    TwitchClient::TWITCH_VOTING_STATE get_type() override { return TwitchClient::TWITCH_VOTING_STATE::STATE_VOTING; }
    void on_message(const std::string& sender, const std::string& message) override {
        if (m_paused) {
            m_queue.emplace_back(sender, message);
        }
        else {
            assert(g_twc != nullptr && "g_twc is NULL!");
            m_vote_manager->on_chat_message(sender, message);
        }
    };
    void on_imgui(bool draw_debug_info) override {
        if (draw_debug_info) {
            ImGui::Text("TWITCH_VOTE_TIMER: %f", m_timer->m_time.count());
        }
        ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(104, 239, 239, 255));

        auto vote_distribution = m_vote_manager->m_vote_distribution_display;
        for (auto& entry : vote_distribution) {
            ImGui::Text("[%c] - %s:\t%d", entry.m_token, entry.m_mod->m_name.c_str(), entry.m_votes);
        }

        ImGui::Text("Vote in chat.");
        imgui_draw_winner(m_vote_manager);
        ImGui::PopStyleColor();

    };
    VoteManager* m_vote_manager;
    utility::Timer* m_timer{ nullptr };
    bool m_paused{ false };
    std::vector<VoteQueueEntry> m_queue;
};

struct IdleState : public Voting {
    IdleState() :
        m_timer(g_twc->m_vote_manager->m_idle_timer.get()) {
        assert((g_twc != nullptr) && "g_twc is NULL!");
        m_vote_manager = g_twc->m_vote_manager;
        m_timer->start();
    }
    ~IdleState() {
        m_timer->stop();
    };
    void tick(fmilliseconds& dt) override {
        assert((m_timer != nullptr) && "idle timer is NULL!");
        m_timer->tick(dt);
    }
    void pause(bool p) override { m_timer->m_active = p; }
    TwitchClient::TWITCH_VOTING_STATE get_type() override { return TwitchClient::TWITCH_VOTING_STATE::STATE_IDLE; }
    void on_imgui(bool draw_debug_info) override {
        if (draw_debug_info) {
            ImGui::Text("TWITCH_IDLE_TIMER: %f", m_timer->m_time.count());
        }
        imgui_draw_winner(m_vote_manager);

    };

    utility::Timer* m_timer{ nullptr };
    VoteManager* m_vote_manager{ nullptr };
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
        std::make_pair('1','1'),
        std::make_pair('2','1'),
        std::make_pair('3','1'),
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
    assert(g_twc && "g_twc is NULL");
    auto vmgr = g_twc->m_vote_manager;
    assert(vmgr && "vmgr is NULL!");

    auto& gamemodes = MutatorRegistry::inst().m_mutators;
    auto& weights   = MutatorRegistry::inst().m_weights;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::discrete_distribution<> distr(weights.begin(), weights.end());

    std::vector<Mutator*> out;
#if MAX_CUM
    out = { gamemodes[0xd],gamemodes[0xd],gamemodes[0xd] };
#else
    size_t n_elem = 3;
    //std::sample(gamemodes.begin(), gamemodes.end(), std::back_inserter(out), n_elem, distr(gen));
    for (size_t i = 0; i < n_elem; i++)
    {
        size_t index = distr(gen);
        auto rnd_mod = gamemodes[index];
        static size_t break_out = 0; // safety in case user sets all probabilities to 0
        while (std::find(out.begin(), out.end(), rnd_mod) != out.end()) {
            if (break_out >= 50) { break; }
            // this is probably bad way to avoid dublicate entries with high probabilty
            // copying vectors of doubles and everything
            auto param = distr.param(); // grab param struct from discrete_distribution
            param._Pvec[index] = 0.0; // sets the probability of recurring index
            param._Init(); // renormalize
            distr.param(param); // set distribution to new parameters
            index = distr(gen); // generate again
            rnd_mod = gamemodes[index];
            break_out++;
        }
        out.push_back(rnd_mod);
    }
#endif

#ifndef NDEBUG

    for (const auto& mod : out) {
        spdlog::info("name: {}", mod->m_name);
    }
#endif

    for (size_t i = 0; i < out.size(); ++i) {
        //spdlog::info("index: {}, name: {}", i, out[i]->m_name);

        vmgr->m_vote_entries.push_back(VoteEntry(out[i]));
    }

    for (size_t i = 0; i < vmgr->m_vote_entries.size(); i++) {
        if (vmgr->m_anti_anti_spam) {
            constexpr static std::array<char, 3> nums{ '1', '2', '3' };
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
        g_twc->twitch->send_chat_message(fmt::format("{}: {}", vmgr->m_vote_entries[i].m_token, vmgr->m_vote_entries[i].m_mod->m_name));
    }
skip_messages:
    vmgr->m_vote_distribution_display = vmgr->m_vote_entries;
    delete g_twc->twitch_vote_state;
    g_twc->twitch_vote_state = nullptr;
    g_twc->twitch_vote_state = new VotingState();
    vmgr->m_anti_anti_spam = !(vmgr->m_anti_anti_spam);
}

// assumes we start with idle state
static void twitch_voting_end() {
    assert(g_twc && "g_twc is NULL!");
    auto vmgr = g_twc->m_vote_manager;
    assert(vmgr && "vmgr is NULL!");
    auto res = std::max_element(vmgr->m_vote_entries.begin(), vmgr->m_vote_entries.end(), [](VoteEntry a, VoteEntry b) { return a.m_votes < b.m_votes; });
    if (g_twc->m_relay_voting_messages) {
        g_twc->twitch->send_chat_message(fmt::format("VOTE ENDED : {}", (*res).m_mod->m_name.c_str()));
    }

    if (g_twc->voting_result == TwitchClient::TWITCH_MODE::LOCAL) {
        DISPLAY_MESSAGE(fmt::format("ACTIVATING RANDOM MOD : {}", (*res).m_mod->m_name.c_str()))
    }
    
    MutatorRegistry::inst().activate_mod((*res).m_mod);
    vmgr->m_previous_winner = std::make_unique<VoteEntry>(*res);

    vmgr->m_vote_entries.clear();
    vmgr->m_voters.clear();
    vmgr->m_vote_distribution_display.clear();

    delete g_twc->twitch_vote_state;
    g_twc->twitch_vote_state = nullptr;
    g_twc->twitch_vote_state = new IdleState();
}

static void parse_message_noop(const std::string& sender, const std::string& message) {}

static void parse_message_chaos(const std::string & sender, const std::string & message) {
    //std::size_t found = message.find('\\');
    char fchar = message.front();
    if (fchar == '\\') {
        std::string s = std::string(std::begin(message) + 1, std::end(message));//message.substr(1, 64);
        //spdlog::info("parse_msg: {}\n", s);
        MutatorRegistry::inst().activate_mod(s);
        //auto& mods = g_framework->get_mods();
        //mods->on_chat_command(message);
    }
}

static void parse_message_voting(const std::string & sender, const std::string & message) {
    auto vote_state = g_twc->twitch_vote_state;
    if (!vote_state) { return; }
    vote_state->on_message(sender, message);
}

static void on_gameplay_status_toggle(TwitchClient* twitch_mgr, bool current_state) {
    if (twitch_mgr->twitch_vote_state) {
        if (twitch_mgr->twitch_vote_state->get_type() == TwitchClient::TWITCH_VOTING_STATE::STATE_NONE && current_state == true) {
            delete twitch_mgr->twitch_vote_state;
            // NOTE(): setting nullptr just in case since twitch runs on 
            // a diff thread here we arrive from dmc4s render thread only
            twitch_mgr->twitch_vote_state = nullptr;
            twitch_mgr->twitch_vote_state = new IdleState;
        }
        twitch_mgr->twitch_vote_state->pause(current_state);
    }
}

std::optional<std::string> TwitchClient::on_initialize() {
    m_libirc = (HMODULE)dyn_link_lib_irc_client();

	if (!m_libirc) {
		//DISPLAY_MESSAGE("[TwitchClient] libircclient.dll not found, skipping.");
		spdlog::info("[TwitchClient] libircclient.dll not found, skipping.");
	}
    else {
        libirc_loaded = true;
        //DISPLAY_MESSAGE("[TwitchClient] libircclient.dll loaded.");
        spdlog::info("[TwitchClient] libircclient.dll loaded.");
    }


	return Mod::on_initialize();
}

void TwitchClient::make_instance(bool standalone) {

	if ((twitch_status == TWITCH_CONNECTING) || (twitch_status == TWITCH_CONNECTED)) {
		return;
	}
    if (voting_result == TwitchClient::TWITCH_MODE::LOCAL) {
        
        m_vote_manager = new VoteManager(
            VoteTimer((float)m_vote_time, twitch_voting_end),
            IdleTimer((float)m_idle_time, twitch_voting_start)
        );

        m_relay_voting_messages = false;
        g_twc = this;
        twitch_vote_state = new Voting();
        twitch_status = TWITCH_CONNECTED;
        return;

    }
	if (!twitch) {
        g_twc = this;

		twitch = new Twitch();

        twitch->on_connected = [this] {
            const std::lock_guard<std::mutex> lock(m_twitch_thread_lock);
            // spdlog::info("[TwitchClient]: Connected to Twitch chat\n");
            DISPLAY_MESSAGE("[TwitchClient]: Connected to Twitch chat");
            twitch_status = TWITCH_CONNECTED;
            if (voting_result == TwitchClient::TWITCH_MODE::VOTING) {
                m_vote_manager = new VoteManager(
                    VoteTimer((float)m_vote_time, twitch_voting_end),
                    IdleTimer((float)m_idle_time, twitch_voting_start)
                );
                twitch_vote_state = new Voting();
            }
        };

		twitch->on_disconnected = [this] {
			//spdlog::info("[TwitchClient]: Disconnected from Twitch chat\n");
			DISPLAY_MESSAGE("[TwitchClient]: Disconnected from Twitch chat");
			twitch_status = TWITCH_DISCONNECTED;
		};
		
		twitch->on_error = [this](int error_code, const char* error) {
			//spdlog::info("[TwitchClient]: Twitch Chat error %d: %s\n", errorCode, error.c_str());
			DISPLAY_MESSAGE(error);
		};

		twitch->on_message = [this](const std::string& sender, const std::string& message) {
			//spdlog::info("[TwitchClient]: Got message! %s: %s \n", sender.c_str(), message.c_str());
            using ParseMessageFptr = void (*)(const std::string& sender, const std::string& message);
            static constexpr std::array<ParseMessageFptr, 3> parse_msg_mode = {
                &parse_message_chaos, &parse_message_voting, &parse_message_noop };
			if (mirror_chat_checkbox) {
				DISPLAY_MESSAGE(std::string{ sender + ": " + message });
			}
            parse_msg_mode[voting_result](sender, message);
		};
	}

	auto login = std::string{ twitch_login };
	auto password = std::string{ twitch_chat_oauth_password };
	if ( !login.empty() && !password.empty() ) {
        const std::lock_guard<std::mutex> lock(m_twitch_thread_lock);
		spdlog::info("[TwitchClient] Connecting to Twitch chat...\n");
		twitch_thread = twitch->connect(login, password);
		twitch_thread.detach();
		twitch_status = TWITCH_CONNECTING;
	}
}

void TwitchClient::disconnect() {
    const std::lock_guard<std::mutex> lock(m_twitch_thread_lock);
	if (twitch) {
		twitch->disconnect();
	}
    twitch_status = TWITCH_DISCONNECTED;
    delete twitch;
    twitch = nullptr;
    delete m_vote_manager;
    m_vote_manager = nullptr;
    delete twitch_vote_state;
    twitch_vote_state = nullptr;
    g_twc = nullptr;
}

void TwitchClient::stop_voting() {
    delete twitch_vote_state;
    twitch_vote_state = nullptr;
    if (m_vote_manager) {
        m_vote_manager->m_vote_entries.clear();
        m_vote_manager->m_voters.clear();
        m_vote_manager->m_vote_distribution_display.clear();

        m_vote_manager->stop_timers();
    }
}

void TwitchClient::start_voting() {
    delete twitch_vote_state;
    twitch_vote_state = nullptr;
    twitch_vote_state = new Voting();
}


// onGUIframe()
// draw your imgui widgets here, you are inside imgui context.
void TwitchClient::on_gui_frame() {
    if (ImGui::CollapsingHeader(_("Random timed gameplay mods"))) {
#if 0
        if (twitch_vote_state) {
            ImGui::Text("Random gameplay mods active");
            ImGui::Text("TIMER: %f", m_idle_timer->m_time.count());
            if (m_current_gameplay_state) {
                ImGui::Text("Not in gameplay");
            }
        }
#endif
        ImGui::Text("Set these before starting:");
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::InputInt(_("Mod timer (seconds)", &m_vote_time, 1, 10))) {
            m_vote_time = std::clamp<int32_t>(m_vote_time, 5, INT_MAX);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        help_marker(_("How long the random mod will last"));
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::InputInt(_("Cooldown timer (seconds)", &m_idle_time, 1, 10))) {
            m_idle_time = std::clamp<int32_t>(m_idle_time, 5, INT_MAX);
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        help_marker(_("How long before the next mod starts"));
        if (ImGui::Button(_("Random mutator mode"))) {
            voting_result = TwitchClient::TWITCH_MODE::LOCAL;
            make_instance(true);
        }
        ImGui::SameLine();
        help_marker(_("Activate random spicy gameplay mods on a timer without connecting to Twitch"));
        if (ImGui::Button(_("Stop random mode"))) {
            disconnect();
        }
    }

    if (ImGui::CollapsingHeader(_("Twitch Integration"))) {

        if (!libirc_loaded) {
            ImGui::Text(_("libircclient.dll not found, twitch support disabled"));
            ImGui::Text(_("Download libircclient-1.10-win32-dll.zip from the link below"));
            ImGui::Text(_("Extract libircclient.dll from zip file libircclient-1.10\\bin\\ to DMC4 root"));
            ImGui::Text(_("for example:"));
            ImGui::Text(_("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Devil May Cry 4\\libircclient.dll"));
            ImGui::Text(_("and restart the game to enable twitch support"));
            gui::ImGuiURL libircclient_url{ "https://sourceforge.net/projects/libircclient/files/libircclient/1.10/", "https://sourceforge.net/projects/libircclient/files/libircclient/1.10/" };
            libircclient_url.draw();
            return;
        }

        ImGui::TextWrapped(_("Filling both fields and enabling one of the options below will allow "
                           "your Twitch viewers to affect your gameplay.\n"
                           "Your credentials are stored in config file in plaintext\n"
                           "Dont send your config with twitch data saved to other people\n"));
        switch (twitch_status) {
        case (TWITCH_CONNECTING):
            ImGui::Text(_("Twitch status: TWITCH_CONNECTING"));
            break;
        case (TWITCH_CONNECTED):
            ImGui::Text(_("Twitch status: TWITCH_CONNECTED"));
            break;
        default:
            ImGui::Text(_("Twitch status: TWITCH_DISCONNECTED"));
            break;
        }
        if (ImGui::Button(_("Get OAuth Password..."))) {
            ShellExecute(0, 0, "https://twitchapps.com/tmi/", 0, 0, SW_SHOW);
        }
        ImGui::InputText(_("Twitch Username"), twitch_login, 128);
        ImGui::InputText(_("Twitch OAuth"), twitch_chat_oauth_password, 128, ImGuiInputTextFlags_Password);
        if (ImGui::Button(_("Connect To Twitch"))) {
            make_instance();
        }
        ImGui::SameLine();
        if (ImGui::Button(_("Disconnect From Twitch"))) {
            disconnect();
        }

        if (voting_result == VOTING) {
            if (twitch_status == TWITCH_CONNECTED) {
                if (ImGui::Button(_("Stop voting"))) {
                    stop_voting();
                }
                if (ImGui::Button(_("Start voting"))) {
                    start_voting();
                }
            }
        }

        ImGui::Checkbox(_("Show debug info in vote window"), &m_twitch_vote_debug);
        ImGui::Checkbox(_("Send voting messages in chat"), &m_relay_voting_messages);
        ImGui::Checkbox(_("Disable voting overlay"), &m_disable_overlay);
        ImGui::Text(_("Set these before connecting:"));
        ImGui::PushItemWidth(sameLineItemWidth);
        if (ImGui::InputInt("Voting timer (seconds)", &m_vote_time, 1, 10)) {
            m_vote_time = std::clamp<int32_t>(m_vote_time, 5, INT_MAX);
        }
        if (ImGui::InputInt(_("Idle timer (seconds)"), &m_idle_time, 1, 10)) {
            m_idle_time = std::clamp<int32_t>(m_idle_time, 5, INT_MAX);
        }
        ImGui::PopItemWidth();
        ImGui::Checkbox("Log In On Game Boot Automatically", &twitch_login_on_boot));
        ImGui::SameLine();
        help_marker(_("This sometimes doesn't work, just come back here and hit disconnect > connect to reconnect"));

        ImGui::Text(_("Twitch Mode FAQ"));
        ImGui::TextWrapped(_("Chaos - viewers can activate twitch mods directly through chat commands."));
        ImGui::TextWrapped(_("Vote mode - viewers can vote on what mods to activate next."));

        if (ImGui::RadioButton(_("Chaos mode"), &voting_result, TwitchClient::TWITCH_MODE::CHAOS)) {
            stop_voting();
            disconnect();
        }
        if (ImGui::RadioButton(_("Vote mode"), &voting_result, TwitchClient::TWITCH_MODE::VOTING)) {
            disconnect();
        }
        mutator_draw_mod_table(MutatorRegistry::inst());
        ImGui::Checkbox(_("Relay Twitch Chat To Devil May Cry 4"), &mirror_chat_checkbox);
    }
}

void TwitchClient::custom_imgui_window() {
    if (!libirc_loaded || m_disable_overlay) { return; }
    bool early_out = (twitch_status != TWITCH_CONNECTED) || voting_result != TwitchClient::TWITCH_MODE::VOTING;
    if (early_out) { return; }

    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImVec2 window_size = ImVec2(io.DisplaySize.x * 0.2f, io.DisplaySize.y * 0.35f);

    ImVec2 window_pos = ImVec2(io.DisplaySize.x - window_size.x - 128.0f, 128.0f);

    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Once);
    ImGui::SetNextWindowSize(window_size, ImGuiCond_Once);

    ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(20, 20, 20, 100));

    ImGui::Begin("Vote results", nullptr, window_flags);
    //ImGui::PushFont(g_framework->get_custom_imgui_font());

    if (twitch_vote_state) {
        twitch_vote_state->on_imgui(m_twitch_vote_debug);
    }
    //ImGui::PopFont();
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
    cfg.set<int32_t>("twitch_vote_time", m_vote_time);
    cfg.set<int32_t>("twitch_idle_time", m_idle_time);

}
static bool g_previos_gameplay_state = false;

void TwitchClient::on_frame(fmilliseconds & dt) {
    bool early_out = (twitch_status != TWITCH_CONNECTED) || (voting_result == TwitchClient::CHAOS);
    if (early_out) { goto update_mutators; }

    if (devil4_sdk::was_gameplay_state_toggled()) {
        on_gameplay_status_toggle(this, !devil4_sdk::is_not_in_gameplay());
    }
    
    if (twitch_vote_state) {
        twitch_vote_state->tick(dt);
    }

update_mutators:
    MutatorRegistry::inst().update(dt);

#if 0
    if (twitch_status != TWITCH_CONNECTED && !m_standalone) { return; }
    MutatorRegistry::inst().update(dt);
    if ((voting_result == TwitchClient::TWITCH_MODE::CHAOS) || 
        m_vote_disabled) {
        return;
    }

    m_current_gameplay_state = devil4_sdk::is_not_in_gameplay();
    if (!twitch_vote_state->get_type()) {
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
#endif
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
    voting_result = cfg.get<int>("twitch_voting_mode").value_or(TwitchClient::TWITCH_MODE::CHAOS); // chaos, voting
    m_vote_time = cfg.get<int32_t>("twitch_vote_time").value_or(30);
    m_idle_time = cfg.get<int32_t>("twitch_idle_time").value_or(15);
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
