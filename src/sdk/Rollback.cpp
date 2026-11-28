#include "Rollback.hpp"

#include <cstring>
#include <mutex>

namespace rollback {
namespace {

struct Participant {
    int id;
    const char* name;
    SaveFn save;
    LoadFn load;
    void* data;
    size_t bytes;
};

std::mutex g_lock;
std::vector<Participant> g_parts;
int g_next = 1;

void put_u32(std::vector<uint8_t>& out, uint32_t v)
{
    const uint8_t* b = (const uint8_t*)&v;
    out.insert(out.end(), b, b + 4);
}

} // namespace

int add(const char* name, SaveFn save, LoadFn load)
{
    std::lock_guard<std::mutex> lock(g_lock);
    const int id = g_next++;
    g_parts.push_back({id, name, save, load, nullptr, 0});
    return id;
}

int add_range(const char* name, void* data, size_t bytes)
{
    std::lock_guard<std::mutex> lock(g_lock);
    const int id = g_next++;
    g_parts.push_back({id, name, nullptr, nullptr, data, bytes});
    return id;
}

void remove(int id)
{
    std::lock_guard<std::mutex> lock(g_lock);
    for (size_t i = 0; i < g_parts.size(); ++i) {
        if (g_parts[i].id == id) {
            g_parts.erase(g_parts.begin() + i);
            return;
        }
    }
}

void (*diag_writer)(const char* text) = nullptr;

void entry_hashes(std::vector<std::pair<const char*, uint32_t>>& out, uintptr_t lo, uintptr_t hi)
{
    std::lock_guard<std::mutex> lock(g_lock);
    out.clear();
    std::vector<uint8_t> part;
    for (const Participant& p : g_parts) {
        part.clear();
        if (p.save) {
            p.save(part);
        } else {
            part.assign((const uint8_t*)p.data, (const uint8_t*)p.data + p.bytes);
        }
        uint32_t h = 2166136261u;
        for (size_t k = 0; k + 4 <= part.size(); k += 4) {
            uint32_t w;
            std::memcpy(&w, &part[k], 4);
            if (w >= lo && w < hi) w = 0;
            h = (h ^ w) * 16777619u;
        }
        out.emplace_back(p.name, h);
    }
}

// Each participant: u32 id, u32 bytes, the bytes. A load skips ids no longer registered and sizes that changed.
void capture(std::vector<uint8_t>& out)
{
    std::lock_guard<std::mutex> lock(g_lock);
    out.clear();
    std::vector<uint8_t> part;
    for (const Participant& p : g_parts) {
        part.clear();
        if (p.save) {
            p.save(part);
        } else {
            part.assign((const uint8_t*)p.data, (const uint8_t*)p.data + p.bytes);
        }
        put_u32(out, (uint32_t)p.id);
        put_u32(out, (uint32_t)part.size());
        out.insert(out.end(), part.begin(), part.end());
    }
}

void apply(const std::vector<uint8_t>& in)
{
    std::lock_guard<std::mutex> lock(g_lock);
    size_t at = 0;
    while (at + 8 <= in.size()) {
        uint32_t id, bytes;
        std::memcpy(&id, &in[at], 4);
        std::memcpy(&bytes, &in[at + 4], 4);
        at += 8;
        if (at + bytes > in.size()) {
            return;
        }
        for (const Participant& p : g_parts) {
            if ((uint32_t)p.id != id) {
                continue;
            }
            if (p.load) {
                p.load(&in[at], bytes);
            } else if (bytes == p.bytes) {
                std::memcpy(p.data, &in[at], bytes);
            }
            break;
        }
        at += bytes;
    }
}

int count()
{
    std::lock_guard<std::mutex> lock(g_lock);
    return (int)g_parts.size();
}

} // namespace rollback
