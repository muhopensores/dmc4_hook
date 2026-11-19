#include "PluginHost.hpp"
#include "PluginGame.hpp"
#include "PluginPlayers.hpp"
#include "PluginRooms.hpp"
#include "PluginMods.hpp"
#include "PluginMenus.hpp"
#include "PluginUnits.hpp"
#include "PluginThink.hpp"
#include "../../../plugin_sdk/include/dmc4_plugin.h"
#include "../../mod.hpp"
#include "imgui/imgui_internal.h"
#include "utility/Config.hpp"
#include "utility/FunctionHook.hpp"
#include "utility/Patch.hpp"
#include <spdlog/spdlog.h>
#include <malloc.h>
#include <algorithm>
#include <atomic>
#include <cstring>
#include <deque>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace {

constexpr size_t kLogLines = 100;

struct LogLine {
    int level;
    std::string text;
};

enum class State { Off, Failed, Running, Stopped };

} // namespace

// the hook's record of a plugin (opaque to plugins: dmc4_plugin.h only declares it)
struct dmc4_plugin {
    std::string file;      // "hello_plugin.dll"
    std::string stem;      // "hello_plugin": its settings are "plugin.<stem>.<key>"
    std::string path;
    HMODULE module = nullptr;
    bool enabled = true;   // load at start: the user's switch, applied at the next start
    State state = State::Off;
    std::string error;
    std::string name, version, author, description;   // copied from dmc4_plugin_info after init
    std::deque<LogLine> log;
    std::map<std::string, std::string> config;
    std::vector<std::unique_ptr<FunctionHook>> hooks;
    std::vector<std::pair<int, std::unique_ptr<Patch>>> patches;   // api->patch, undone when it stops
    int next_patch = 1;
    bool warned_imgui = false;   // its panel left ImGui unbalanced (warned once)
};

namespace plugins {
namespace fs = std::filesystem;

namespace {

// g_mutex guards every plugin's fields above, g_callbacks and g_disabled. No plugin code runs while it is held
// (plugins call back into the api, which takes it). g_plugins itself only changes in load_all, before anything else
// runs, so it is read without the lock.
std::mutex g_mutex;
std::vector<std::unique_ptr<dmc4_plugin>> g_plugins;
std::set<std::string> g_disabled;
std::string g_folder;
bool g_loaded = false;
std::atomic<uint64_t> g_frame{0};

struct Callback {
    int id;
    dmc4_plugin* owner;
    dmc4_callback fn;
    void* user;
    bool frame;   // on_frame, else on_panel
};
std::vector<Callback> g_callbacks;
int g_next_id = 1;

void add_log(dmc4_plugin& p, int level, const std::string& text) {
    if (level >= DMC4_LOG_ERROR) spdlog::error("[{}] {}", p.file, text);
    else if (level == DMC4_LOG_WARN) spdlog::warn("[{}] {}", p.file, text);
    else spdlog::info("[{}] {}", p.file, text);
    std::lock_guard<std::mutex> lock(g_mutex);
    p.log.push_back({level, text});
    while (p.log.size() > kLogLines) p.log.pop_front();
}

// the plugin stops: its callbacks go, its held mod settings and patches are undone, the DLL stays loaded (its hooks may
// still be in the game's code)
void stop(dmc4_plugin& p, State state, const std::string& message) {
    add_log(p, DMC4_LOG_ERROR, message);
    release_mods(&p);
    release_menus(&p);
    release_units(&p);
    release_think(&p);
    std::lock_guard<std::mutex> lock(g_mutex);
    p.patches.clear();
    p.state = state;
    p.error = message;
    g_callbacks.erase(std::remove_if(g_callbacks.begin(), g_callbacks.end(), [&](const Callback& c) { return c.owner == &p; }),
                      g_callbacks.end());
}

// ---- crash isolation (no C++ objects in these functions: __try can't unwind them) ----

struct Crash {
    DWORD code = 0;
    void* address = nullptr;
};

int seh_filter(EXCEPTION_POINTERS* e, Crash* c) {
    c->code = e->ExceptionRecord->ExceptionCode;
    c->address = e->ExceptionRecord->ExceptionAddress;
    return EXCEPTION_EXECUTE_HANDLER;
}

bool guarded_call(dmc4_callback fn, void* user, Crash* c) {
    __try {
        fn(user);
        return true;
    } __except (seh_filter(GetExceptionInformation(), c)) {
        return false;
    }
}

using AbiFn = uint32_t(__cdecl*)(void);
using InitFn = int(__cdecl*)(const dmc4_api*, dmc4_plugin*, dmc4_plugin_info*);

bool guarded_abi(AbiFn fn, uint32_t* out, Crash* c) {
    __try {
        *out = fn();
        return true;
    } __except (seh_filter(GetExceptionInformation(), c)) {
        return false;
    }
}

bool guarded_init(InitFn fn, const dmc4_api* api, dmc4_plugin* self, dmc4_plugin_info* info, int* rc, Crash* c) {
    __try {
        *rc = fn(api, self, info);
        return true;
    } __except (seh_filter(GetExceptionInformation(), c)) {
        return false;
    }
}

// "access violation at hello_plugin.dll+0x1234"
std::string describe(const Crash& c) {
    if (c.code == EXCEPTION_STACK_OVERFLOW) _resetstkoflw();   // put the guard page back, or the next overflow kills the game
    if (c.code == 0xE06D7363) return "an uncaught C++ exception";   // raised in KERNELBASE: its address says nothing
    const char* what = c.code == EXCEPTION_ACCESS_VIOLATION   ? "access violation"
                     : c.code == EXCEPTION_STACK_OVERFLOW      ? "stack overflow"
                     : c.code == EXCEPTION_INT_DIVIDE_BY_ZERO  ? "division by zero"
                     : c.code == EXCEPTION_ILLEGAL_INSTRUCTION ? "illegal instruction"
                                                               : nullptr;
    char head[64];
    if (what) snprintf(head, sizeof head, "%s", what);
    else snprintf(head, sizeof head, "exception 0x%08lX", (unsigned long)c.code);
    HMODULE mod = nullptr;
    char name[MAX_PATH] = "";
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)c.address, &mod) && GetModuleFileNameA(mod, name, MAX_PATH)) {
        char line[MAX_PATH + 128];
        snprintf(line, sizeof line, "%s at %s+0x%X", head, fs::path(name).filename().string().c_str(),
                 unsigned((uintptr_t)c.address - (uintptr_t)mod));
        return line;
    }
    char line[128];
    snprintf(line, sizeof line, "%s at 0x%p", head, c.address);
    return line;
}

std::string last_error_text() {
    const DWORD err = GetLastError();
    char* msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, 0,
                   (LPSTR)&msg, 0, nullptr);
    std::string text = msg ? msg : "";
    if (msg) LocalFree(msg);
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r' || text.back() == ' ')) text.pop_back();
    return text + " (error " + std::to_string(err) + ")";
}

// ---- the api (plain C functions; a NULL self or string is ignored) ----

void __cdecl api_log(dmc4_plugin* self, int level, const char* text) {
    if (self && text) add_log(*self, level, text);
}

int add_callback(dmc4_plugin* self, dmc4_callback fn, void* user, bool frame) {
    if (!self || !fn) return 0;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (self->state != State::Running) return 0;
    const int id = g_next_id++;
    g_callbacks.push_back({id, self, fn, user, frame});
    return id;
}
int __cdecl api_on_frame(dmc4_plugin* self, dmc4_callback fn, void* user) { return add_callback(self, fn, user, true); }
int __cdecl api_on_panel(dmc4_plugin* self, dmc4_callback fn, void* user) { return add_callback(self, fn, user, false); }

void __cdecl api_remove_callback(dmc4_plugin* self, int id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_callbacks.erase(std::remove_if(g_callbacks.begin(), g_callbacks.end(),
                                     [&](const Callback& c) { return c.owner == self && c.id == id; }),
                      g_callbacks.end());
}

uint64_t __cdecl api_frame_count() { return g_frame.load(); }

const char* __cdecl api_config_get(dmc4_plugin* self, const char* key) {
    if (!self || !key) return nullptr;
    thread_local std::unordered_map<dmc4_plugin*, std::string> returned;   // valid until this plugin's next call here
    std::lock_guard<std::mutex> lock(g_mutex);
    const auto it = self->config.find(key);
    if (it == self->config.end()) return nullptr;
    std::string& out = returned[self];
    out = it->second;
    return out.c_str();
}

void __cdecl api_config_set(dmc4_plugin* self, const char* key, const char* value) {
    if (!self || !key || !*key) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    if (value) self->config[key] = value;
    else self->config.erase(key);
}

void* __cdecl api_imgui_context() { return ImGui::GetCurrentContext(); }

void __cdecl api_imgui_allocators(void** alloc_fn, void** free_fn, void** user_data) {
    ImGuiMemAllocFunc a = nullptr;
    ImGuiMemFreeFunc f = nullptr;
    void* u = nullptr;
    ImGui::GetAllocatorFunctions(&a, &f, &u);
    if (alloc_fn) *alloc_fn = (void*)a;
    if (free_fn) *free_fn = (void*)f;
    if (user_data) *user_data = u;
}

int install(dmc4_plugin* self, uintptr_t address, void* detour, uint32_t size, uintptr_t* out) {
    if (!self || !address || !detour) return 0;
    auto hook = std::make_unique<FunctionHook>(address, detour);
    if (!hook->create()) {
        add_log(*self, DMC4_LOG_ERROR, fmt::format("couldn't hook 0x{:X} (already hooked, or not code)", address));
        return 0;
    }
    if (out) *out = size ? address + size : hook->get_original();
    add_log(*self, DMC4_LOG_INFO, fmt::format("hooked 0x{:X}", address));
    std::lock_guard<std::mutex> lock(g_mutex);
    self->hooks.push_back(std::move(hook));
    return 1;
}
int __cdecl api_patch(dmc4_plugin* self, uintptr_t address, const void* bytes, uint32_t size, const void* expect) {
    if (!self || !address || !bytes || !size) return 0;
    if (IsBadReadPtr(reinterpret_cast<const void*>(address), size)) {
        add_log(*self, DMC4_LOG_ERROR, fmt::format("patch 0x{:X}: not readable memory", address));
        return 0;
    }
    if (expect && std::memcmp(reinterpret_cast<const void*>(address), expect, size) != 0) {
        add_log(*self, DMC4_LOG_ERROR, fmt::format("patch 0x{:X}: the bytes there aren't the expected ones (another mod, or another exe)", address));
        return 0;
    }
    std::vector<int16_t> b(size);
    for (uint32_t i = 0; i < size; ++i) b[i] = static_cast<const uint8_t*>(bytes)[i];
    auto patch = Patch::create(address, b, true);
    std::lock_guard<std::mutex> lock(g_mutex);
    const int id = self->next_patch++;
    self->patches.emplace_back(id, std::move(patch));
    return id;
}
void __cdecl api_unpatch(dmc4_plugin* self, int id) {
    if (!self) return;
    std::lock_guard<std::mutex> lock(g_mutex);
    auto& v = self->patches;
    v.erase(std::remove_if(v.begin(), v.end(), [&](const auto& e) { return e.first == id; }), v.end());   // restores
}

int __cdecl api_hook(dmc4_plugin* self, uintptr_t address, void* detour, uintptr_t* original) {
    return install(self, address, detour, 0, original);
}
int __cdecl api_hook_at(dmc4_plugin* self, uintptr_t address, void* detour, uint32_t size, uintptr_t* resume) {
    if (size < 5) {
        if (self) add_log(*self, DMC4_LOG_ERROR, fmt::format("hook_at 0x{:X}: size {} is under the 5 bytes a jump needs", address, size));
        return 0;
    }
    return install(self, address, detour, size, resume);
}

const dmc4_api g_api = {
    DMC4_PLUGIN_ABI,
    DMC4_PLUGIN_API_VERSION,
    sizeof(dmc4_api),
    api_log,
    api_on_frame,
    api_on_panel,
    api_remove_callback,
    api_frame_count,
    api_config_get,
    api_config_set,
    api_imgui_context,
    api_imgui_allocators,
    IMGUI_VERSION,
    api_hook,
    api_hook_at,
    game_api(),
    players_api(),
    rooms_api(),
    mods_api(),
    api_patch,
    api_unpatch,
    menus_api(),
    units_api(),
};

// ---- loading ----

void load(dmc4_plugin& p) {
    std::error_code ec;
    const std::wstring wpath = fs::absolute(p.path, ec).wstring();
    HMODULE m = LoadLibraryExW(wpath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);   // its own DLLs from its folder
    if (!m) {
        stop(p, State::Failed, "Windows couldn't load it: " + last_error_text());
        return;
    }
    const auto abi_fn = reinterpret_cast<AbiFn>(GetProcAddress(m, "dmc4_plugin_abi"));
    const auto init_fn = reinterpret_cast<InitFn>(GetProcAddress(m, "dmc4_plugin_init"));
    if (!abi_fn || !init_fn) {
        FreeLibrary(m);
        stop(p, State::Failed, "not a dmc4_hook plugin (it exports no dmc4_plugin_abi / dmc4_plugin_init)");
        return;
    }
    uint32_t abi = 0;
    Crash crash;
    if (!guarded_abi(abi_fn, &abi, &crash)) {
        stop(p, State::Failed, "crashed in dmc4_plugin_abi: " + describe(crash));
        return;
    }
    if (abi != DMC4_PLUGIN_ABI) {
        FreeLibrary(m);
        stop(p, State::Failed, fmt::format("built for plugin ABI {}; this dmc4_hook has ABI {} (rebuild it with the current "
                                           "plugin_sdk)", abi, DMC4_PLUGIN_ABI));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        p.module = m;
        p.state = State::Running;   // before init: it registers its callbacks there
    }
    dmc4_plugin_info info{};
    info.size = sizeof info;
    int rc = 0;
    if (!guarded_init(init_fn, &g_api, &p, &info, &rc, &crash)) {
        stop(p, State::Stopped, "crashed in dmc4_plugin_init: " + describe(crash));
        return;
    }
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        p.name = info.name ? info.name : "";
        p.version = info.version ? info.version : "";
        p.author = info.author ? info.author : "";
        p.description = info.description ? info.description : "";
    }
    if (rc != 0) {
        stop(p, State::Stopped, fmt::format("dmc4_plugin_init returned {} (the plugin refused to start)", rc));
        return;
    }
    add_log(p, DMC4_LOG_INFO, "loaded" + (p.version.empty() ? std::string() : ", version " + p.version));
}

// runs the callbacks of one kind (and of one plugin, for panels); no lock held while plugin code runs
void run(bool frame, dmc4_plugin* only = nullptr) {
    std::vector<Callback> list;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const Callback& c : g_callbacks)
            if (c.frame == frame && (!only || c.owner == only)) list.push_back(c);
    }
    for (const Callback& c : list) {
        {
            std::lock_guard<std::mutex> lock(g_mutex);   // removed by an earlier callback, or its plugin stopped
            if (std::none_of(g_callbacks.begin(), g_callbacks.end(), [&](const Callback& x) { return x.id == c.id; })) continue;
        }
        Crash crash;
        if (frame) {
            if (!guarded_call(c.fn, c.user, &crash)) stop(*c.owner, State::Stopped, "crashed in a frame callback: " + describe(crash));
            continue;
        }
        ImGuiErrorRecoveryState before, after;
        ImGui::ErrorRecoveryStoreState(&before);
        const bool ok = guarded_call(c.fn, c.user, &crash);
        ImGui::ErrorRecoveryStoreState(&after);
        const bool balanced = std::memcmp(&before, &after, sizeof before) == 0;
        if (!balanced) {   // unclosed trees / IDs / windows (a crash, or a missing Pop): closed here, quietly
            ImGuiIO& io = ImGui::GetIO();   // (ImGui's own check would assert in debug builds)
            const bool assert_on = io.ConfigErrorRecoveryEnableAssert;
            io.ConfigErrorRecoveryEnableAssert = false;
            ImGui::ErrorRecoveryTryToRecoverState(&before);
            io.ConfigErrorRecoveryEnableAssert = assert_on;
        }
        if (!ok) stop(*c.owner, State::Stopped, "crashed in its menu panel: " + describe(crash));
        else if (!balanced && !c.owner->warned_imgui) {
            c.owner->warned_imgui = true;
            add_log(*c.owner, DMC4_LOG_WARN, "its menu panel left an ImGui Push / Begin / TreeNode without its Pop / End "
                                             "(closed by the hook; logged once)");
        }
    }
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return (char)std::tolower(c); });
    return s;
}

} // namespace

void load_all(const char* folder, const utility::Config& cfg) {
    if (g_loaded) return;
    g_loaded = true;
    g_folder = folder;
    {
        std::stringstream ss(cfg.get("plugins_disabled").value_or(""));
        std::string n;
        while (std::getline(ss, n, '|'))
            if (!n.empty()) g_disabled.insert(lower(n));
    }
    std::vector<fs::path> files;
    std::error_code ec;
    if (fs::is_directory(g_folder, ec)) {
        for (const fs::directory_entry& e : fs::directory_iterator(g_folder, ec))
            if (e.is_regular_file(ec) && lower(e.path().extension().string()) == ".dll") files.push_back(e.path());
    }
    std::sort(files.begin(), files.end(), [](const fs::path& a, const fs::path& b) {
        return lower(a.filename().string()) < lower(b.filename().string());
    });
    for (const fs::path& f : files) {
        auto p = std::make_unique<dmc4_plugin>();
        p->file = f.filename().string();
        p->stem = f.stem().string();
        p->path = f.string();
        p->enabled = !g_disabled.count(lower(p->file));
        const std::string prefix = "plugin." + p->stem + ".";
        for (const auto& [key, value] : cfg.get_key_values())
            if (key.size() > prefix.size() && key.compare(0, prefix.size(), prefix) == 0) p->config[key.substr(prefix.size())] = value;
        g_plugins.push_back(std::move(p));
    }
    for (auto& up : g_plugins)
        if (up->enabled) load(*up);
    spdlog::info("Plugins: {} plugin DLL(s) in {}", g_plugins.size(), g_folder);
}

bool run_guarded(dmc4_plugin* self, dmc4_callback fn, void* user, const char* what) {
    if (!self || !fn) return false;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (self->state != State::Running) return false;
    }
    Crash crash;
    if (guarded_call(fn, user, &crash)) return true;
    stop(*self, State::Stopped, std::string("crashed in ") + what + ": " + describe(crash));
    return false;
}

void plugin_log(dmc4_plugin* self, int level, const std::string& text) {
    if (self) add_log(*self, level, text);
}

std::string plugin_file(const dmc4_plugin* self) { return self ? self->file : std::string(); }

std::vector<Info> list() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<Info> out;
    for (auto& up : g_plugins) {
        const dmc4_plugin& p = *up;
        Info i;
        i.file = p.file;
        i.name = p.name;
        i.version = p.version;
        i.author = p.author;
        i.description = p.description;
        i.state = p.state == State::Off ? "off" : p.state == State::Failed ? "failed"
                : p.state == State::Running ? "running" : "stopped";
        i.error = p.error;
        i.enabled = p.enabled;
        i.hooks = (int)p.hooks.size();
        for (const LogLine& l : p.log) i.log.push_back(l.text);
        out.push_back(std::move(i));
    }
    return out;
}

void frame() {
    ++g_frame;
    units_frame();
    run(true);
}

void draw() {
    static bool restart_note = false;
    if (!g_loaded) {
        ImGui::TextDisabled("%s", _("Plugins weren't loaded at this start (switched off)."));
        return;
    }
    if (g_plugins.empty()) {
        ImGui::TextDisabled(_("no plugin DLLs in %s"), g_folder.empty() ? "plugins" : g_folder.c_str());
        return;
    }
    for (auto& up : g_plugins) {
        dmc4_plugin& p = *up;
        std::string title, error, author, description;
        State state;
        bool enabled;
        {
            std::lock_guard<std::mutex> lock(g_mutex);
            title = p.name.empty() ? p.file : p.name + (p.version.empty() ? "" : " " + p.version) + " (" + p.file + ")";
            error = p.error;
            author = p.author;
            description = p.description;
            state = p.state;
            enabled = p.enabled;
        }
        ImGui::PushID(&p);
        bool on = enabled;
        if (ImGui::Checkbox("##load", &on)) {
            std::lock_guard<std::mutex> lock(g_mutex);
            p.enabled = on;
            if (on) g_disabled.erase(lower(p.file));
            else g_disabled.insert(lower(p.file));
            restart_note = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", _("Load this plugin when the game starts (takes effect at the next start)"));
        ImGui::SameLine();
        const bool open = ImGui::TreeNode("##plugin", "%s", title.c_str());
        ImGui::SameLine();
        switch (state) {
        case State::Running: ImGui::TextDisabled("%s", _("running")); break;
        case State::Off: ImGui::TextDisabled("%s", _("not loaded (switched off)")); break;
        case State::Failed: ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", _("couldn't load")); break;
        case State::Stopped: ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", _("stopped")); break;
        }
        if (open) {
            if (!author.empty()) ImGui::TextDisabled(_("by %s"), author.c_str());
            if (!description.empty()) ImGui::TextWrapped("%s", description.c_str());
            if (!error.empty()) ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", error.c_str());
            if (state == State::Running) run(false, &p);
            if (ImGui::TreeNode(_("Log"))) {
                std::lock_guard<std::mutex> lock(g_mutex);
                for (const LogLine& l : p.log) {
                    const ImVec4 colour = l.level >= DMC4_LOG_ERROR  ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f)
                                        : l.level == DMC4_LOG_WARN   ? ImVec4(1.0f, 0.8f, 0.3f, 1.0f)
                                                                     : ImGui::GetStyleColorVec4(ImGuiCol_TextDisabled);
                    ImGui::TextColored(colour, "%s", l.text.c_str());
                }
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
        ImGui::PopID();
    }
    if (restart_note) ImGui::TextDisabled("%s", _("Switching a plugin on or off takes effect at the next start."));
}

void save_config(utility::Config& cfg) {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::string disabled;
    for (const std::string& n : g_disabled) disabled += (disabled.empty() ? "" : "|") + n;
    cfg.set("plugins_disabled", disabled);
    for (auto& up : g_plugins)
        for (const auto& [key, value] : up->config) cfg.set("plugin." + up->stem + "." + key, value);
    saved_mods(cfg);   // settings plugins hold: the user's own values go to the file
}

} // namespace plugins
