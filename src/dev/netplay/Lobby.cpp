#include "Lobby.hpp"
#include "NetLink.hpp"
#include "../../sdk/HostRules.hpp"

#include "gekkonet.h"

#include <windows.h>
#include <algorithm>
#include <cstring>
#include <vector>

namespace lobby {
namespace {

constexpr uint8_t kHello = net_link::kFirstMessageKind;
constexpr uint8_t kRules = net_link::kFirstMessageKind + 1;
constexpr uint8_t kBye = net_link::kFirstMessageKind + 2;
constexpr uint8_t kStart = net_link::kFirstMessageKind + 3;
constexpr uint8_t kStartAck = net_link::kFirstMessageKind + 4;
constexpr uint8_t kRoster = net_link::kFirstMessageKind + 5;
constexpr double kStartEvery = 250.0;
constexpr uint32_t kMagic = 0x3259424Cu;   // "LBY2"
constexpr double kHelloEvery = 500.0;
constexpr double kRulesEvery = 1000.0;
constexpr double kTimeout = 2000.0;
constexpr uint8_t kNobody = 0xFF;          // a joining machine's hello before the host gave it a player
constexpr int kPortsTried = 16;            // Join: the ports tried from Config::port on
constexpr size_t kAddressBytes = 48;

#pragma pack(push, 1)
struct Hello {
    uint32_t magic;
    uint32_t build;
    uint8_t player;
    uint8_t character;
    uint8_t where;
    uint8_t rules_version;
};
struct RulesMessage {
    uint32_t magic;
    uint32_t serial;
    Rules rules;
    uint8_t version;    // the host rules version whose bytes follow (0: none)
    uint8_t spare;
    uint16_t bytes;
};
struct RosterMessage {
    uint32_t magic;
    uint32_t serial;
    uint8_t players;
    uint8_t you;
    uint8_t spare[2];
    char address[kMaxPlayers][kAddressBytes];   // as the host sees each player ("" for the host itself)
};
struct StartMessage {
    uint32_t magic;
    Start start;
};
struct StartAck {
    uint32_t magic;
    uint16_t id;
    uint8_t player;
    uint8_t spare;
};
#pragma pack(pop)

bool g_open = false;
bool g_socket = false;   // the lobby holds the port (no GekkoNet session made its own)
bool g_session = false;
Config g_cfg;
uint16_t g_port = 0;
// the roster
int g_players = 0;
int g_local = -1;
std::string g_address[kMaxPlayers];
uint32_t g_roster_serial = 0;
bool g_gone[kMaxPlayers] = {};   // host: left during a session, taken out of the roster when it ends

Peer g_peers[kMaxPlayers];
double g_seen[kMaxPlayers] = {};
uint8_t g_self_character = 0;
Where g_self_where = Where::Elsewhere;
double g_last_hello = -1e9;
double g_last_rules = -1e9;
Rules g_rules;
uint32_t g_rules_serial = 0;
uint8_t g_rules_version_sent = 0;
bool g_have_rules = false;
uint32_t g_taken_serial = 0;         // the others: the serial of the rules taken (a late datagram carries older ones)
Start g_start;                       // host: the one being sent
bool g_start_sending = false;
bool g_start_confirmed[kMaxPlayers] = {};
double g_last_start = -1e9;
uint16_t g_taken_start = 0;          // the others: the last start taken
bool g_start_ready = false;
Start g_start_in;

double now_ms()
{
    LARGE_INTEGER t, f;
    QueryPerformanceCounter(&t);
    QueryPerformanceFrequency(&f);
    return (double)t.QuadPart * 1000.0 / (double)f.QuadPart;
}

bool take_socket()
{
    GekkoNetAdapter* adapter = gekko_default_adapter(g_port);
    if (!adapter) {
        return false;
    }
    net_link::wrap(adapter);
    g_socket = true;
    return true;
}

void roster_changed()
{
    ++g_roster_serial;
    g_last_hello = -1e9;   // the hellos (and the host's roster) go out at the next update
}

// host: the joined player at this address
int slot_of(const std::string& from)
{
    for (int i = 1; i < g_players; ++i) {
        if (g_address[i] == from) {
            return i;
        }
    }
    return -1;
}

// host: player `slot` leaves the roster (the ones after it move up); during a session, once it ends
void drop(int slot)
{
    g_peers[slot] = Peer{};
    if (g_session) {
        g_gone[slot] = true;
        return;
    }
    for (int i = slot; i + 1 < g_players; ++i) {
        g_address[i] = g_address[i + 1];
        g_peers[i] = g_peers[i + 1];
        g_seen[i] = g_seen[i + 1];
        g_gone[i] = g_gone[i + 1];
    }
    --g_players;
    g_address[g_players].clear();
    g_peers[g_players] = Peer{};
    g_gone[g_players] = false;
    roster_changed();
}

void send_all(uint8_t kind, const void* data, uint32_t bytes)
{
    if (g_local < 0) {
        net_link::send_message(g_cfg.host_address, kind, data, bytes);   // joining: only the host knows this machine
        return;
    }
    for (int i = 0; i < g_players && i < kMaxPlayers; ++i) {
        if (i != g_local && !g_address[i].empty()) {
            net_link::send_message(g_address[i], kind, data, bytes);
        }
    }
}

void send_roster()
{
    RosterMessage r{};
    r.magic = kMagic;
    r.serial = g_roster_serial;
    r.players = (uint8_t)g_players;
    for (int i = 1; i < g_players; ++i) {
        strncpy_s(r.address[i], g_address[i].c_str(), _TRUNCATE);
    }
    for (int i = 1; i < g_players; ++i) {
        if (!g_gone[i]) {
            r.you = (uint8_t)i;
            net_link::send_message(g_address[i], kRoster, &r, sizeof r);
        }
    }
}

void take_roster(const net_link::Message& m)
{
    RosterMessage r;
    std::memcpy(&r, m.data.data(), sizeof r);
    if (r.magic != kMagic || r.players < 2 || r.players > kMaxPlayers || r.you < 1 || r.you >= r.players) {
        return;
    }
    std::string address[kMaxPlayers];
    address[0] = m.from;   // the host, as this machine reaches it
    for (int i = 1; i < r.players; ++i) {
        if (i != r.you) {
            r.address[i][kAddressBytes - 1] = 0;
            address[i] = r.address[i];
        }
    }
    bool same = r.players == g_players && r.you == g_local;
    for (int i = 0; same && i < kMaxPlayers; ++i) {
        same = address[i] == g_address[i];
    }
    if (same) {
        return;
    }
    if (r.you != g_local) {
        for (Peer& p : g_peers) {
            p = Peer{};   // the players moved: their hellos say again who is where
        }
    }
    g_players = r.players;
    g_local = r.you;
    for (int i = 0; i < kMaxPlayers; ++i) {
        g_address[i] = address[i];
    }
    roster_changed();
}

void receive(double t)
{
    net_link::Message m;
    while (net_link::take_message(m)) {
        if (m.kind == kHello && m.data.size() == sizeof(Hello)) {
            Hello h;
            std::memcpy(&h, m.data.data(), sizeof h);
            if (h.magic != kMagic) {
                continue;
            }
            int player = h.player;
            if (g_cfg.kind == Kind::Host) {
                player = slot_of(m.from);
                if (player < 0) {
                    if (g_players >= kMaxPlayers) {
                        continue;   // full
                    }
                    player = g_players++;
                    g_address[player] = m.from;
                    g_gone[player] = false;
                    roster_changed();
                }
            } else if (player >= kMaxPlayers || player == g_local || player >= g_players) {
                continue;
            }
            Peer& p = g_peers[player];
            p.connected = true;
            p.build = h.build;
            p.character = h.character;
            p.where = (Where)h.where;
            p.rules_version = h.rules_version;
            g_seen[player] = t;
        } else if (m.kind == kRoster && m.data.size() == sizeof(RosterMessage) && g_cfg.kind == Kind::Join) {
            take_roster(m);
        } else if (m.kind == kRules && m.data.size() >= sizeof(RulesMessage) && !host()) {
            RulesMessage r;
            std::memcpy(&r, m.data.data(), sizeof r);
            if (r.magic != kMagic) {
                continue;
            }
            // newer than the rules taken; a much lower serial is a host that started over
            if (!g_have_rules || r.serial >= g_taken_serial || g_taken_serial - r.serial > 64) {
                g_rules = r.rules;
                g_have_rules = true;
                g_taken_serial = r.serial;
            }
            if (r.version && m.data.size() == sizeof r + r.bytes) {
                host_rules::receive(r.version, m.data.data() + sizeof r, r.bytes);
            }
        } else if (m.kind == kStart && m.data.size() == sizeof(StartMessage) && !host() && g_local > 0) {
            StartMessage s;
            std::memcpy(&s, m.data.data(), sizeof s);
            if (s.magic != kMagic) {
                continue;
            }
            const StartAck ack{kMagic, s.start.id, (uint8_t)g_local, 0};
            net_link::send_message(m.from, kStartAck, &ack, sizeof ack);
            if (s.start.id != g_taken_start) {
                g_taken_start = s.start.id;
                g_start_in = s.start;
                g_start_ready = true;
            }
        } else if (m.kind == kStartAck && m.data.size() == sizeof(StartAck) && host()) {
            StartAck a;
            std::memcpy(&a, m.data.data(), sizeof a);
            if (a.magic == kMagic && a.id == g_start.id && a.player < kMaxPlayers) {
                g_start_confirmed[a.player] = true;
            }
        } else if (m.kind == kBye && m.data.size() == 1) {
            if (g_cfg.kind == Kind::Host) {
                const int slot = slot_of(m.from);
                if (slot > 0) {
                    drop(slot);
                }
            } else if (m.data[0] < kMaxPlayers) {
                g_peers[m.data[0]] = Peer{};
            }
        }
    }
}

} // namespace

uint32_t build_id()
{
    static const uint32_t id = [] {
        HMODULE self = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&build_id, &self);
        if (!self) {
            return 0u;
        }
        const auto* dos = (const IMAGE_DOS_HEADER*)self;
        const auto* nt = (const IMAGE_NT_HEADERS*)((const uint8_t*)self + dos->e_lfanew);
        return (uint32_t)nt->FileHeader.TimeDateStamp;
    }();
    return id;
}

bool open(const Config& cfg)
{
    close();
    g_cfg = cfg;
    for (Peer& p : g_peers) {
        p = Peer{};
    }
    for (bool& g : g_gone) {
        g = false;
    }
    for (std::string& a : g_address) {
        a.clear();
    }
    if (cfg.kind == Kind::Fixed) {
        g_players = std::clamp(cfg.players, 1, kMaxPlayers);
        g_local = std::clamp(cfg.local, 0, g_players - 1);
        for (int i = 0; i < g_players; ++i) {
            if (i != g_local) {
                g_address[i] = cfg.address[i];
            }
        }
    } else if (cfg.kind == Kind::Host) {
        g_players = 1;
        g_local = 0;
    } else {
        g_players = 0;
        g_local = -1;
    }
    ++g_roster_serial;
    g_have_rules = false;
    g_taken_serial = 0;
    g_rules_version_sent = 0;
    g_start_sending = false;
    g_start_ready = false;
    g_taken_start = 0;   // a new host numbers its starts from 1 again
    g_last_hello = g_last_rules = -1e9;
    const int tries = cfg.kind == Kind::Join ? kPortsTried : 1;
    for (int k = 0; k < tries; ++k) {
        g_port = (uint16_t)(cfg.port + k);
        if (take_socket()) {
            g_open = true;
            return true;
        }
    }
    g_players = 0;
    g_local = -1;
    return false;
}

void close()
{
    if (!g_open) {
        return;
    }
    const uint8_t self = (uint8_t)(g_local < 0 ? kNobody : g_local);
    send_all(kBye, &self, 1);
    if (g_socket) {
        net_link::wrap(nullptr);
        gekko_default_adapter_destroy();
        g_socket = false;
    }
    g_open = false;
    for (Peer& p : g_peers) {
        p = Peer{};
    }
    g_have_rules = false;
    g_players = 0;
    g_local = -1;
    ++g_roster_serial;
}

bool is_open()
{
    return g_open;
}

Kind kind()
{
    return g_cfg.kind;
}

int players()
{
    return g_players;
}

int local()
{
    return g_local;
}

std::string address(int player)
{
    return player >= 0 && player < kMaxPlayers ? g_address[player] : std::string();
}

uint16_t port()
{
    return g_port;
}

uint32_t roster_serial()
{
    return g_roster_serial;
}

void on_session_start()
{
    g_socket = false;
    g_session = true;
}

void on_session_end()
{
    g_socket = false;
    g_session = false;
    if (g_cfg.kind == Kind::Host) {
        for (int i = g_players - 1; i >= 1; --i) {
            if (g_gone[i]) {
                drop(i);
            }
        }
    }
}

void update(bool session_running)
{
    if (!g_open) {
        return;
    }
    const double t = now_ms();
    if (!session_running) {
        if (!g_socket && !take_socket()) {
            return;
        }
        net_link::poll();
    }
    receive(t);
    for (int i = 0; i < kMaxPlayers; ++i) {
        if (g_peers[i].connected && t - g_seen[i] > kTimeout) {
            if (g_cfg.kind == Kind::Host && i > 0) {
                drop(i);
            } else {
                g_peers[i] = Peer{};
            }
        }
    }
    if (t - g_last_hello >= kHelloEvery) {
        g_last_hello = t;
        const Hello h{kMagic, build_id(), (uint8_t)(g_local < 0 ? kNobody : g_local), g_self_character,
            (uint8_t)g_self_where, host_rules::newest_received()};
        send_all(kHello, &h, sizeof h);
        if (g_cfg.kind == Kind::Host) {
            send_roster();
        }
    }
    if (host() && (t - g_last_rules >= kRulesEvery || host_rules::latest() != g_rules_version_sent)) {
        g_last_rules = t;
        g_rules_version_sent = host_rules::latest();
        RulesMessage r{kMagic, g_rules_serial, g_rules, 0, 0, 0};
        std::vector<uint8_t> values;
        if (host_rules::blob(g_rules_version_sent, values)) {
            r.version = g_rules_version_sent;
            r.bytes = (uint16_t)values.size();
        }
        std::vector<uint8_t> out(sizeof r);
        std::memcpy(out.data(), &r, sizeof r);
        out.insert(out.end(), values.begin(), values.end());
        send_all(kRules, out.data(), (uint32_t)out.size());
    }
    if (g_start_sending && t - g_last_start >= kStartEvery) {
        g_last_start = t;
        if (start_confirmed()) {
            g_start_sending = false;
        } else {
            const StartMessage s{kMagic, g_start};
            for (int i = 0; i < g_players && i < kMaxPlayers; ++i) {
                if (i != g_local && !g_start_confirmed[i] && !g_address[i].empty()) {
                    net_link::send_message(g_address[i], kStart, &s, sizeof s);
                }
            }
        }
    }
}

void start(const Start& s)
{
    g_start = s;
    g_start_sending = true;
    g_last_start = -1e9;
    for (bool& c : g_start_confirmed) {
        c = false;
    }
}

bool start_confirmed()
{
    for (int i = 0; i < g_players && i < kMaxPlayers; ++i) {
        if (i != g_local && !g_start_confirmed[i]) {
            return false;
        }
    }
    return true;
}

bool take_start(Start& out)
{
    if (!g_start_ready) {
        return false;
    }
    g_start_ready = false;
    out = g_start_in;
    return true;
}

void set_self(uint8_t character, Where where)
{
    g_self_character = character;
    g_self_where = where;
}

const Peer& peer(int player)
{
    static const Peer none{};
    return player >= 0 && player < kMaxPlayers ? g_peers[player] : none;
}

bool all_connected()
{
    if (!g_open || g_local < 0 || g_players < 2) {
        return false;
    }
    for (int i = 0; i < g_players && i < kMaxPlayers; ++i) {
        if (i != g_local && !g_peers[i].connected) {
            return false;
        }
    }
    return true;
}

bool host()
{
    return g_local == 0;
}

void set_rules(const Rules& rules)
{
    if (std::memcmp(&rules, &g_rules, sizeof rules) != 0) {
        g_rules = rules;
        ++g_rules_serial;
        g_last_rules = -1e9;   // out at the next update
    }
}

bool rules(Rules& out)
{
    if (!g_have_rules) {
        return false;
    }
    out = g_rules;
    return true;
}

} // namespace lobby
