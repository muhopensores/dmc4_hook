#pragma once
// lobby - online co-op players finding each other and agreeing on a session (Vibed\Coop\netplay_lobby.md).
//
// Opening it opens this machine's UDP port at once (GekkoNet's default adapter, wrapped by net_link) and keeps it while
// no GekkoNet session runs: between stages the session ends and starts again, and the lobby takes the port back. The
// host (player 1) hands out the players: the others join with its address only, and the host tells each which player it
// plays and where the others are (the roster), in join order, packed again between stages when one leaves. Every
// machine says hello to every other twice a second (which player it plays, its character, where it is, its build, the
// host rules version it has); a player is connected while those arrive. The host sends the session's rules and the
// registered host rules (sdk/HostRules), which the others play by.

#include <cstdint>
#include <string>

namespace lobby {

constexpr int kMaxPlayers = 4;

enum class Where : uint8_t {
    Elsewhere = 0,   // title, menus other than the Mission Menu, loading
    MissionMenu = 1,
    Stage = 2,
};

// What the host decides for everyone: the players and the rules that change the simulation.
struct Rules {
    uint8_t players = 2;
    uint8_t character[kMaxPlayers] = {};   // players 2-4: 0 Nero, 1 Dante (player 1 is the game's own)
    uint8_t push = 1;
    uint8_t hits = 0;
    uint8_t players_first = 1;
    uint8_t dt_from_players = 1;
    uint8_t spare[2] = {};
};

struct Peer {
    bool connected = false;
    uint32_t build = 0;
    uint8_t character = 0;
    Where where = Where::Elsewhere;
    uint8_t rules_version = 0;   // the newest host rules version it has (host_rules)
};

// Host: this machine is player 1, the others join it. Join: the host's address only, the rest comes from the host.
// Fixed: every player's address given (the -netplay -plN command line), nobody joins.
enum class Kind : uint8_t { Fixed, Host, Join };
struct Config {
    Kind kind = Kind::Fixed;
    int local = 0;                        // Fixed: the player this machine plays
    int players = 2;                      // Fixed
    std::string address[kMaxPlayers];     // Fixed: the others' "ip:port"; this machine's unused
    std::string host_address;             // Join: the host's "ip:port"
    uint16_t port = 7000;                 // this machine's (Join: the first free one from here)
};

bool open(const Config& cfg);   // false: no port could be opened
void close();
bool is_open();
Kind kind();

// The roster: who plays which player, and where.
int players();                    // Host: itself and those who joined; Join: the host's word (0 before it answers)
int local();                      // the player this machine plays (-1: joined, the host hasn't answered yet)
std::string address(int player);  // player i's "ip:port" ("" for this machine)
uint16_t port();                  // the port this machine listens on
uint32_t roster_serial();         // changes whenever the roster does

// Every frame. While no GekkoNet session runs the lobby reads the socket itself.
void update(bool session_running);
void on_session_start();   // GekkoNet made its own socket on the port
void on_session_end();     // and destroyed it: the lobby opens the port again

void set_self(uint8_t character, Where where);
const Peer& peer(int player);
bool all_connected();
uint32_t build_id();   // this DLL's link time: every machine has to run the same build

bool host();
void set_rules(const Rules& rules);   // the host's, sent to every other machine
bool rules(Rules& out);               // the other machines: the host's latest, once one arrived

// The host starts the stage for everyone: every machine launches it from its Mission Menu.
struct Start {
    uint16_t id = 0;
    uint8_t bloody_palace = 0;
    uint8_t difficulty = 3;   // the Mission Menu's row (Human 0 .. Hell And Hell 6)
    int16_t mission = 1;
    int16_t floor = 1;
    uint8_t character = 6;    // player 1's, the menu's (0-3 Dante, 4-7 Nero)
    uint8_t spare = 0;
};
void start(const Start& s);     // host: sent until every other machine confirms it
bool start_confirmed();         // host: every machine has it
bool take_start(Start& out);    // the others: the host's start, once

} // namespace lobby
