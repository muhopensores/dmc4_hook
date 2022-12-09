#pragma once

#if 1
#include "../mod.hpp"
#include "../twitch/twitch.hpp"

struct VoteEntry {
    Mutator* m_mod;
    size_t m_votes;
    VoteEntry(Mutator* mod) : m_mod(mod), m_votes(0) {};
};

struct VoteManager {
    std::vector<Mutator*> m_previous_mods;

    std::vector<VoteEntry> m_vote_entries;
    std::vector<VoteEntry> m_vote_distribution_display;
    std::vector<size_t> m_voters;

    bool m_anti_anti_spam{ false };

    void on_chat_message(const std::string& sender, const std::string& msg);
};

struct Voting;

struct ITwitchMode {
    virtual void parse_message(const std::string& sender, const std::string& message) = 0;


    virtual ~ITwitchMode() {};
};

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

    void make_instance();
    void disconnect();

    bool vote_checkbox        = false;
    bool mirror_chat_checkbox = true; // always forget to set this when testing shit
    bool libirc_loaded        = false;
    int  voting_result         = 0;

    ITwitchMode* m_twitch_mode{ nullptr };

    Twitch* twitch{ nullptr };
    std::mutex m_twitch_thread_lock;
    std::thread twitch_thread;
    TwitchConnectionStatus twitch_status = TWITCH_DISCONNECTED;
    
    VoteManager* m_vote_manager{ nullptr };
    //TWITCH_VOTING_STATE twitch_vote_state = STATE_NONE;
    Voting* twitch_vote_state;

    int32_t m_vote_time{30};
    int32_t m_idle_time{15};

    utility::Timer* m_voting_timer{ nullptr };
    utility::Timer* m_idle_timer{ nullptr };

    bool m_vote_disabled{ false };
    bool m_twitch_vote_debug{ false };
    bool m_current_gameplay_state{ false };
    bool m_disable_overlay{ false };
    char m_winner[256]{ 0 };
private:
    char twitch_login[128]{};
    char twitch_chat_oauth_password[128]{};

    ImFont* custom_window_font{};

    HMODULE m_libirc{ NULL };

    void stop_voting();
    void start_voting();
};

struct TwitchModeChaos : public ITwitchMode {
    TwitchModeChaos(TwitchClient* twc) : m_twc(twc) {}
    TwitchClient* m_twc{ nullptr };
    void parse_message(const std::string& sender, const std::string& message) override;
};

struct TwitchModeVoting : public ITwitchMode {
    TwitchModeVoting(TwitchClient* twc) : m_twc(twc) {}
    TwitchClient* m_twc{ nullptr };
    void parse_message(const std::string& sender, const std::string& message) override;
};
#endif
