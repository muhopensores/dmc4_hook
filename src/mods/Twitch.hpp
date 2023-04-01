#pragma once

#if 1
#include "../mod.hpp"
#include "../twitch/twitch.hpp"

struct VoteEntry {
    Mutator* m_mod;
    size_t m_votes;
    char m_token;
    VoteEntry(Mutator* mod) : m_mod(mod), m_votes(0), m_token(0) {};
};

using VoteTimer = std::pair<float, std::function<void()>>;
using IdleTimer = std::pair<float, std::function<void()>>;

struct VoteManager {

    explicit VoteManager(VoteTimer&& voting_timer, IdleTimer&&idle_timer) {
            m_voting_timer = std::make_unique<utility::Timer>((float)voting_timer.first, voting_timer.second);
            m_idle_timer = std::make_unique<utility::Timer>((float)idle_timer.first, idle_timer.second);
    };

    std::unique_ptr<utility::Timer> m_voting_timer;
    std::unique_ptr<utility::Timer> m_idle_timer;

    std::unique_ptr<VoteEntry> m_previous_winner;

    std::vector<VoteEntry> m_vote_entries;
    std::vector<VoteEntry> m_vote_distribution_display;
    std::vector<size_t> m_voters;

    size_t m_prev_mod_idx{ 0 };
    bool m_anti_anti_spam{ false };
    void on_chat_message(const std::string& sender, const std::string& msg);

    inline void stop_timers() {
        m_voting_timer->stop();
        m_idle_timer->stop();
    };

    
};

struct Voting;

// Define ModName class that is a Mod
class TwitchClient : public Mod {
public:

    enum TWITCH_VOTING_STATE {
        STATE_NONE,
        STATE_VOTING,
        STATE_IDLE,
        STATE_MAX,
    };

    enum TWITCH_MODE {
        CHAOS,
        VOTING,
        LOCAL,
        MAX
    };

    TwitchClient() = default;

    Mod::ModType get_mod_type() override { return SLOW; }

    std::string get_mod_name() override { return "TwitchClient"; };
    std::optional<std::string> on_initialize() override;

    // callbacks
    void on_frame(fmilliseconds& dt) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;
    void custom_imgui_window();

    void make_instance(bool standalone = false);
    void disconnect();

    bool   vote_checkbox        = false;
    bool   mirror_chat_checkbox = true; // always forget to set this when testing shit
    bool   libirc_loaded        = false;
    int    voting_result        = 0;

    Twitch* twitch{ nullptr };
    std::mutex m_twitch_thread_lock;
    std::thread twitch_thread;
    TwitchConnectionStatus twitch_status = TWITCH_DISCONNECTED;
    
    VoteManager* m_vote_manager{ nullptr };
    //TWITCH_VOTING_STATE twitch_vote_state = STATE_NONE;
    Voting* twitch_vote_state;

    int32_t m_vote_time{30};
    int32_t m_idle_time{15};

    bool m_twitch_vote_debug{ false };
    bool m_disable_overlay{ false };
    bool m_relay_voting_messages{ true };
private:
    char twitch_login[128]{};
    char twitch_chat_oauth_password[128]{};

    ImFont* custom_window_font{};

    HMODULE m_libirc{ NULL };

    void stop_voting();
    void start_voting();
};

#endif
