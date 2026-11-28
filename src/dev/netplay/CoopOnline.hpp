#pragma once
#include "../mod.hpp"

class CoopOnline : public Mod {
public:
    CoopOnline() = default;
    std::string get_mod_name() override { return "CoopOnline"; };
    // std::vector<std::string> get_search_terms() override { return {"mod sample"}; }
    // Mod::ModType get_mod_type() override { return SLOW; };

    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_stage_start() override;
    void on_stage_end() override;
private:
    std::unique_ptr<Patch> patch1;
    std::unique_ptr<FunctionHook> hook1;
};

// The online part of the Co-op panel (Coop::on_gui_frame) and what it asks of netplay.
namespace coop_online {
bool online();                      // the panel's Online mode (netplay's online component)
void set_online(bool on);           // off: leaves the lobby and the session
bool rules_from_host();             // online, this machine isn't player 1: the players and rules are the host's
int local_player();                 // the player this machine plays online (0 otherwise)
bool session_armed();               // online: the next stage load starts a session (the host started it for everyone)
const char* player_status(int player);   // "this machine", "waiting", "connected, in a stage", ...
void draw_panel();                  // this machine's player, the addresses, Connect / Leave, split screen, Advanced
} // namespace coop_online
