#pragma once

#if 1
#include "../mod.hpp"
#include "../twitch/twitch.hpp"
// Define ModName class that is a Mod
class TwitchClient : public Mod {
public:
    TwitchClient() = default;

    Mod::ModType get_mod_type() override { return SLOW; }

    std::string get_mod_name() override { return "TwitchClient"; };
    std::optional<std::string> on_initialize() override;

    // callbacks
    /*void onFrame() override;*/
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame() override;

    void make_instance();
    void disconnect();

    bool vote_checkbox        = false;
    bool mirror_chat_checkbox = true; // always forget to set this when testing shit
    bool libirc_loaded        = false;
    int voting_result         = 0;

    Twitch* twitch;
    std::thread twitch_thread;
    TwitchConnectionStatus twitch_status = TWITCH_DISCONNECTED;

private:
    char twitch_login[128]{};
    char twitch_chat_oauth_password[128]{};
};
#endif
