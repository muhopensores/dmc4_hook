#include "HostRules.hpp"

#include <atomic>
#include <cstring>
#include <mutex>

namespace host_rules {
namespace {

struct Entry {
    const char* name;
    uint8_t* data;
    size_t bytes;
    size_t offset;   // in a version's bytes
};

std::vector<Entry> g_entries;
size_t g_total = 0;
Role g_role = Role::None;
bool g_session = false;

// The host's GUI copy, and a guest's own values while it plays by the host's.
std::vector<uint8_t> g_ui;
std::vector<uint8_t> g_own;

// Every version's bytes, by version (1-255). A version is written once, before anyone may use it: the host's input
// names it only after every guest said it has it, so the tick (the game thread) reads bytes that no longer change.
uint8_t g_blobs[256][kMaxBytes];
std::atomic<bool> g_have[256];
uint8_t g_latest = 0;      // host: the newest version; guest: the newest received
uint8_t g_in_force = 0;    // host: what its input carries
std::mutex g_mutex;        // add / set_role against the frame calls

const Entry* find(const void* data)
{
    for (const Entry& e : g_entries) {
        if (e.data == data) {
            return &e;
        }
    }
    return nullptr;
}

void read_variables(uint8_t* out)
{
    for (const Entry& e : g_entries) {
        std::memcpy(out + e.offset, e.data, e.bytes);
    }
}

void write_variables(const uint8_t* in)
{
    for (const Entry& e : g_entries) {
        std::memcpy(e.data, in + e.offset, e.bytes);
    }
}

void forget_versions()
{
    for (auto& h : g_have) {
        h.store(false, std::memory_order_relaxed);
    }
    g_latest = g_in_force = 0;
}

uint8_t next_version(uint8_t v)
{
    return v >= 255 ? 1 : (uint8_t)(v + 1);
}

} // namespace

int add(const char* name, void* data, size_t bytes)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!data || !bytes || g_total + bytes > kMaxBytes || g_role != Role::None) {
        return 0;
    }
    g_entries.push_back({name, (uint8_t*)data, bytes, g_total});
    g_total += bytes;
    return (int)g_entries.size();
}

size_t bytes()
{
    return g_total;
}

void* ui_ptr(void* data)
{
    if (g_role == Role::Host) {
        if (const Entry* e = find(data)) {
            return g_ui.data() + e->offset;
        }
    }
    return data;
}

const void* own_ptr(const void* data)
{
    const Entry* e = g_role == Role::None ? nullptr : find(data);
    if (!e) {
        return data;
    }
    return (g_role == Role::Host ? g_ui.data() : g_own.data()) + e->offset;
}

bool from_host()
{
    return g_role == Role::Guest;
}

void set_role(Role role)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    if (role == g_role) {
        return;
    }
    if (g_role == Role::Host) {
        write_variables(g_ui.data());   // the host keeps its latest edits
    } else if (g_role == Role::Guest) {
        write_variables(g_own.data());  // a guest gets its own values back
    }
    forget_versions();
    g_role = role;
    if (role == Role::Host) {
        g_ui.assign(g_total, 0);
        read_variables(g_ui.data());
        g_latest = 1;
        std::memcpy(g_blobs[1], g_ui.data(), g_total);
        g_have[1].store(true, std::memory_order_release);
        g_in_force = 1;
    } else if (role == Role::Guest) {
        g_own.assign(g_total, 0);
        read_variables(g_own.data());
    }
}

void set_session(bool running)
{
    g_session = running;
    if (!running && g_role == Role::Host) {
        write_variables(g_ui.data());   // between sessions the host's own game uses its edits at once
    }
}

void host_frame()
{
    if (g_role != Role::Host) {
        return;
    }
    if (std::memcmp(g_ui.data(), g_blobs[g_latest], g_total) != 0) {
        const uint8_t v = next_version(g_latest);
        g_have[v].store(false, std::memory_order_relaxed);
        std::memcpy(g_blobs[v], g_ui.data(), g_total);
        g_have[v].store(true, std::memory_order_release);
        g_latest = v;
    }
    if (!g_session) {
        write_variables(g_ui.data());
    }
}

uint8_t latest()
{
    return g_role == Role::None ? 0 : g_latest;
}

bool blob(uint8_t version, std::vector<uint8_t>& out)
{
    if (!version || !g_have[version].load(std::memory_order_acquire)) {
        return false;
    }
    out.assign(g_blobs[version], g_blobs[version] + g_total);
    return true;
}

void set_in_force(uint8_t version)
{
    if (version && g_have[version].load(std::memory_order_acquire)) {
        g_in_force = version;
    }
}

uint8_t in_force()
{
    return g_role == Role::Host ? g_in_force : 0;
}

void receive(uint8_t version, const uint8_t* data, size_t size)
{
    if (g_role != Role::Guest || !version || size != g_total) {
        return;
    }
    if (!g_have[version].load(std::memory_order_acquire) || std::memcmp(g_blobs[version], data, size) != 0) {
        g_have[version].store(false, std::memory_order_relaxed);
        std::memcpy(g_blobs[version], data, size);
        g_have[version].store(true, std::memory_order_release);
    }
    g_latest = version;
    if (!g_session) {
        write_variables(g_blobs[version]);   // between sessions: the host's values at once (the GUI shows them)
    }
}

uint8_t newest_received()
{
    return g_role == Role::Guest ? g_latest : 0;
}

void tick(uint8_t version)
{
    if (g_role == Role::None || !version || !g_have[version].load(std::memory_order_acquire)) {
        return;
    }
    write_variables(g_blobs[version]);
}

} // namespace host_rules
