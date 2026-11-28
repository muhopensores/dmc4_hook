#include "NetLink.hpp"

#include "gekkonet.h"

#include <algorithm>
#include <cstring>
#include <deque>

namespace net_link {
namespace {

constexpr uint8_t kTag0 = 0xD4;
constexpr uint8_t kTag1 = 'R';
constexpr uint8_t kVersion = 1;
constexpr uint8_t kKindOffer = 1;
constexpr uint8_t kKindReply = 2;
constexpr uint32_t kChunkBytes = 1200;
// A resync offer: the packed battle state (40-160 KB in play; BattleSnapshot.hpp's worst case, every unit slot full,
// is ~6 MB) and the units' image. 8 MiB is 6,991 chunks, within the chunk count's 16 bits.
constexpr uint32_t kMaxState = 8u << 20;
constexpr size_t kMaxReplies = 64;
constexpr size_t kMaxMessages = 256;
constexpr uint32_t kMaxMessageBytes = 1200;

#pragma pack(push, 1)
struct Head {
    uint8_t tag0;
    uint8_t tag1;
    uint8_t kind;
    uint8_t version;
    uint16_t id;
    uint16_t chunk;
    uint16_t chunks;
    uint16_t spare;
    int32_t frame;
    uint32_t total;   // offer: the state's bytes; reply: the status
    uint32_t hash;    // offer: FNV-1a of the whole state; reply: the value
};
#pragma pack(pop)
static_assert(sizeof(Head) == 24, "net_link::Head is the wire header");

GekkoNetAdapter* g_inner = nullptr;
GekkoNetAdapter g_outer{};

struct Assembly {
    bool active = false;
    std::string from;
    uint16_t id = 0;
    int32_t frame = 0;
    uint32_t total = 0;
    uint32_t hash = 0;
    uint16_t chunks = 0;
    uint16_t have = 0;
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> got;
};
Assembly g_assembly;
bool g_offer_ready = false;
Offer g_offer;
std::deque<Reply> g_replies;
std::deque<Message> g_messages;

uint32_t fnv(const uint8_t* p, size_t n)
{
    uint32_t h = 2166136261u;
    for (size_t k = 0; k < n; ++k) {
        h = (h ^ p[k]) * 16777619u;
    }
    return h;
}

void send_raw(const std::string& to, const std::vector<uint8_t>& packet)
{
    if (!g_inner || !g_inner->send_data || to.empty()) {
        return;
    }
    GekkoNetAddress addr{};
    addr.data = (void*)to.data();
    addr.size = (unsigned int)to.size();
    g_inner->send_data(&addr, (const char*)packet.data(), (int)packet.size());
}

void on_offer_chunk(const std::string& from, const Head& h, const uint8_t* body, uint32_t n)
{
    if (h.total == 0 || h.total > kMaxState || h.chunks == 0 || h.chunk >= h.chunks ||
        (uint32_t)h.chunks != (h.total + kChunkBytes - 1) / kChunkBytes) {
        return;
    }
    Assembly& a = g_assembly;
    if (!a.active || a.id != h.id || a.from != from) {
        // a newer offer replaces the one being put together
        a = Assembly{};
        a.active = true;
        a.from = from;
        a.id = h.id;
        a.frame = h.frame;
        a.total = h.total;
        a.hash = h.hash;
        a.chunks = h.chunks;
        a.bytes.assign(h.total, 0);
        a.got.assign(h.chunks, 0);
    }
    if (a.total != h.total || a.chunks != h.chunks || a.got[h.chunk]) {
        return;
    }
    const uint32_t at = (uint32_t)h.chunk * kChunkBytes;
    if (n != std::min(kChunkBytes, a.total - at)) {
        return;
    }
    std::memcpy(a.bytes.data() + at, body, n);
    a.got[h.chunk] = 1;
    if (++a.have < a.chunks) {
        return;
    }
    if (fnv(a.bytes.data(), a.total) == a.hash) {
        g_offer.from = a.from;
        g_offer.id = a.id;
        g_offer.frame = a.frame;
        g_offer.state = std::move(a.bytes);
        g_offer_ready = true;
    }
    a = Assembly{};
}

// true: the datagram was this link's (taken here, never shown to GekkoNet)
bool handle(const GekkoNetResult* r)
{
    if (!r || !r->data || r->data_len < sizeof(Head)) {
        return false;
    }
    const uint8_t* d = (const uint8_t*)r->data;
    if (d[0] != kTag0 || d[1] != kTag1) {
        return false;
    }
    Head h;
    std::memcpy(&h, d, sizeof h);
    if (h.version != kVersion) {
        return true;
    }
    const std::string from((const char*)r->addr.data, r->addr.size);
    if (h.kind == kKindOffer) {
        on_offer_chunk(from, h, d + sizeof h, r->data_len - (uint32_t)sizeof h);
    } else if (h.kind == kKindReply && g_replies.size() < kMaxReplies) {
        Reply reply;
        reply.from = from;
        reply.id = h.id;
        reply.frame = h.frame;
        reply.status = (Status)h.total;
        reply.value = (int32_t)h.hash;
        g_replies.push_back(reply);
    } else if (h.kind >= kFirstMessageKind && h.total == r->data_len - sizeof h && h.total <= kMaxMessageBytes &&
        g_messages.size() < kMaxMessages) {
        Message m;
        m.from = from;
        m.kind = h.kind;
        m.data.assign(d + sizeof h, d + r->data_len);
        g_messages.push_back(std::move(m));
    }
    return true;
}

void outer_send(GekkoNetAddress* addr, const char* data, int length)
{
    if (g_inner && g_inner->send_data) {
        g_inner->send_data(addr, data, length);
    }
}

// a datagram GekkoNet's adapter handed over, with its address and data
void free_result(GekkoNetResult* r)
{
    if (g_inner && g_inner->free_data) {
        g_inner->free_data(r->addr.data);
        g_inner->free_data(r->data);
        g_inner->free_data(r);
    }
}

GekkoNetResult** outer_receive(int* length)
{
    *length = 0;
    if (!g_inner || !g_inner->receive_data) {
        return nullptr;
    }
    int n = 0;
    GekkoNetResult** results = g_inner->receive_data(&n);
    int kept = 0;
    for (int i = 0; i < n; ++i) {
        GekkoNetResult* r = results[i];
        if (handle(r)) {
            free_result(r);
        } else {
            results[kept++] = r;
        }
    }
    *length = kept;
    return results;
}

void outer_free(void* data)
{
    if (g_inner && g_inner->free_data) {
        g_inner->free_data(data);
    }
}

} // namespace

GekkoNetAdapter* wrap(GekkoNetAdapter* inner)
{
    reset();
    g_inner = inner;
    if (!inner) {
        return nullptr;
    }
    g_outer.send_data = outer_send;
    g_outer.receive_data = outer_receive;
    g_outer.free_data = outer_free;
    return &g_outer;
}

void reset()
{
    g_assembly = Assembly{};
    g_offer_ready = false;
    g_offer = Offer{};
    g_replies.clear();
    g_messages.clear();
}

bool send_offer(const std::string& to, uint16_t id, int32_t frame, const uint8_t* state, uint32_t bytes)
{
    if (!state || bytes == 0 || bytes > kMaxState) {
        return false;
    }
    const uint16_t chunks = (uint16_t)((bytes + kChunkBytes - 1) / kChunkBytes);
    const uint32_t hash = fnv(state, bytes);
    std::vector<uint8_t> packet;
    for (uint16_t c = 0; c < chunks; ++c) {
        const uint32_t at = (uint32_t)c * kChunkBytes;
        const uint32_t n = std::min(kChunkBytes, bytes - at);
        const Head h{kTag0, kTag1, kKindOffer, kVersion, id, c, chunks, 0, frame, bytes, hash};
        packet.resize(sizeof h + n);
        std::memcpy(packet.data(), &h, sizeof h);
        std::memcpy(packet.data() + sizeof h, state + at, n);
        send_raw(to, packet);
    }
    return true;
}

void send_reply(const std::string& to, const Reply& reply)
{
    const Head h{kTag0, kTag1, kKindReply, kVersion, reply.id, 0, 1, 0, reply.frame, (uint32_t)reply.status,
        (uint32_t)reply.value};
    std::vector<uint8_t> packet(sizeof h);
    std::memcpy(packet.data(), &h, sizeof h);
    send_raw(to, packet);
}

bool take_offer(Offer& out)
{
    if (!g_offer_ready) {
        return false;
    }
    out = std::move(g_offer);
    g_offer = Offer{};
    g_offer_ready = false;
    return true;
}

void send_message(const std::string& to, uint8_t kind, const void* data, uint32_t bytes)
{
    if (kind < kFirstMessageKind || bytes > kMaxMessageBytes) {
        return;
    }
    const Head h{kTag0, kTag1, kind, kVersion, 0, 0, 1, 0, 0, bytes, 0};
    std::vector<uint8_t> packet(sizeof h + bytes);
    std::memcpy(packet.data(), &h, sizeof h);
    if (bytes) {
        std::memcpy(packet.data() + sizeof h, data, bytes);
    }
    send_raw(to, packet);
}

bool take_message(Message& out)
{
    if (g_messages.empty()) {
        return false;
    }
    out = std::move(g_messages.front());
    g_messages.pop_front();
    return true;
}

void poll()
{
    if (!g_inner) {
        return;
    }
    int n = 0;
    GekkoNetResult** results = outer_receive(&n);
    for (int i = 0; i < n; ++i) {
        free_result(results[i]);
    }
}

bool take_reply(Reply& out)
{
    if (g_replies.empty()) {
        return false;
    }
    out = g_replies.front();
    g_replies.pop_front();
    return true;
}

} // namespace net_link
