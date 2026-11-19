#include "PluginMods.hpp"
#include "PluginHost.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../ModFramework.hpp"
#include "../../Mods.hpp"
#include "utility/Config.hpp"
#include <algorithm>
#include <cctype>
#include <map>
#include <mutex>
#include <string>
#include <tuple>
#include <vector>

namespace plugins {
namespace {

struct Held {
    dmc4_plugin* owner;
    std::string mod, key;    // the mod's own name, the key as given
    std::string user;        // the user's value when the first hold began
    std::string value;       // the held value
};

std::recursive_mutex g_mutex;
std::vector<Held> g_held;    // one entry per (mod, key); a second plugin holding it takes it over
std::map<dmc4_plugin*, std::string> g_last_get;

std::string lower(std::string s) {
    for (char& c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

Mod* mod_named(const char* name) {
    if (!name || !g_framework || !g_framework->get_mods()) return nullptr;
    const std::string n = lower(name);
    for (const auto& m : g_framework->get_mods()->get_mods())
        if (lower(m->get_mod_name()) == n) return m.get();
    return nullptr;
}

bool current(Mod* m, const std::string& key, std::string* out) {
    utility::Config cfg{};
    m->on_config_save(cfg);
    const auto v = cfg.get(key);
    if (!v) return false;
    if (out) *out = *v;
    return true;
}

void apply(Mod* m, const std::string& key, const std::string& value) {
    utility::Config cfg{};
    m->on_config_save(cfg);
    cfg.set(key, value);
    m->on_config_load(cfg);   // as at startup: the mod applies its settings
}

auto find(Mod* m, const std::string& key) {
    return std::find_if(g_held.begin(), g_held.end(), [&](const Held& h) {
        return h.mod == m->get_mod_name() && h.key == key;
    });
}

const char* __cdecl api_get(dmc4_plugin* self, const char* mod, const char* key) {
    Mod* m = mod_named(mod);
    std::string v;
    if (!self || !m || !key || !current(m, key, &v)) return nullptr;
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    std::string& slot = g_last_get[self];
    slot = v;
    return slot.c_str();
}

int __cdecl api_hold(dmc4_plugin* self, const char* mod, const char* key, const char* value) {
    if (!self || !key || !value) return 0;
    Mod* m = mod_named(mod);
    std::string user;
    if (!m || !current(m, key, &user)) {
        plugin_log(self, DMC4_LOG_WARN, std::string("mods->hold: no setting ") + (mod ? mod : "?") + " / " + key);
        return 0;
    }
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    auto it = find(m, key);
    if (it == g_held.end()) g_held.push_back({self, m->get_mod_name(), key, user, value});
    else { it->owner = self; it->value = value; }
    apply(m, key, value);
    return 1;
}

void release_one(std::vector<Held>::iterator it) {
    if (Mod* m = mod_named(it->mod.c_str())) apply(m, it->key, it->user);
    g_held.erase(it);
}

void __cdecl api_release(dmc4_plugin* self, const char* mod, const char* key) {
    Mod* m = mod_named(mod);
    if (!self || !m || !key) return;
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    auto it = find(m, key);
    if (it != g_held.end() && it->owner == self) release_one(it);
}

void __cdecl api_release_all(dmc4_plugin* self) { release_mods(self); }

const dmc4_mods_api g_mods = {
    sizeof(dmc4_mods_api),
    api_get,
    api_hold,
    api_release,
    api_release_all,
};

} // namespace

const dmc4_mods_api* mods_api() { return &g_mods; }

void release_mods(dmc4_plugin* self) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    for (auto it = g_held.begin(); it != g_held.end();) {
        if (it->owner != self) { ++it; continue; }
        const size_t at = it - g_held.begin();
        release_one(it);
        it = g_held.begin() + at;
    }
}

void saved_mods(utility::Config& cfg) {
    std::lock_guard<std::recursive_mutex> lock(g_mutex);
    for (const Held& h : g_held) cfg.set(h.key, h.user);
}

} // namespace plugins
