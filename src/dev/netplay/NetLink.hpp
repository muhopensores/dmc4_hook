#pragma once
// net_link - netplay's own messages on GekkoNet's UDP socket: the lobby's, and the resync's.
//
// GekkoNet exchanges inputs and one checksum per frame. Everything else goes here: the lobby's hellos and rules
// (Lobby.cpp), and the resync's state offers and replies (when the checksums of a frame disagree, the host, control 0,
// sends its packed state of a confirmed frame and the other machines take its values; CoopOnline.cpp, "resync").
// wrap() puts an adapter around GekkoNet's: send_data passes through, and receive_data hands GekkoNet every datagram
// except this link's, which start with the tag 0xD4 'R' (GekkoNet's start with its packet type, 1 to 9). A state goes
// out in 1200-byte chunks, so nothing on the way has to fragment it; the lobby's messages are one datagram each.

#include <cstdint>
#include <string>
#include <vector>

struct GekkoNetAdapter;

namespace net_link {

// The adapter to hand GekkoNet in place of `inner`, which must stay valid while the session runs.
GekkoNetAdapter* wrap(GekkoNetAdapter* inner);
// A new session: partial and unread messages are dropped.
void reset();

struct Offer {
    std::string from;             // the sender's "ip:port"
    uint16_t id = 0;
    int32_t frame = 0;
    std::vector<uint8_t> state;   // the sender's packed state of `frame`, as its checksum covers it
};

enum class Status : uint32_t {
    Applied = 0,   // the state was taken and the frames after it simulated again
    Same = 1,      // this machine's state of that frame already matched
    Refused = 2,   // value = a Refusal
};

enum class Refusal : int32_t {
    None = 0,
    NotKept,       // the frame is no longer in this machine's history
    Units,         // the two machines have different units
    Restore,       // the state could not be applied
};

struct Reply {
    std::string from;   // the sender's "ip:port"
    uint16_t id = 0;
    int32_t frame = 0;
    Status status = Status::Refused;
    int32_t value = 0;   // Applied / Same: the newest frame whose checksum this machine sent before; Refused: a Refusal
};

bool send_offer(const std::string& to, uint16_t id, int32_t frame, const uint8_t* state, uint32_t bytes);   // false: too big
void send_reply(const std::string& to, const Reply& reply);
bool take_offer(Offer& out);   // the newest complete offer, once
bool take_reply(Reply& out);   // replies in arrival order

// Small messages (the lobby's): a kind of 16 or more and up to 1200 bytes, one datagram.
struct Message {
    std::string from;
    uint8_t kind = 0;
    std::vector<uint8_t> data;
};
constexpr uint8_t kFirstMessageKind = 16;
void send_message(const std::string& to, uint8_t kind, const void* data, uint32_t bytes);
bool take_message(Message& out);   // in arrival order

// While no GekkoNet session reads the socket: take this link's datagrams and drop the rest (a peer whose session already
// started sends GekkoNet's; it repeats them until this machine's session runs).
void poll();

} // namespace net_link
