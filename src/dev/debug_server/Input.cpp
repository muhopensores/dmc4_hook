#include "Tools.hpp"
#include "Server.hpp"
#include "Memory.hpp"
#include "../../sdk/Devil4.hpp"
#include "../../sdk/sArea.hpp"
#include "../../sdk/FrameHooks.hpp"
#include "../../utility/FunctionHook.hpp"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <vector>

// S3 tools (plan Vibed/GameMCP/README.md): input, pause, step, wait.
//
// Input goes into sDevil4Pad's logical pad 0 (mPadInfo[0], Vibed/RE/sDevil4Pad_struct.md) right after
// sDevil4Pad::move (0x485050) fills it from the hardware each frame: every reader takes it from there, the ~80 menu
// functions and gameplay alike (cPeripheral::update copies it into the player, Vibed/RE/player_pad_input.md). The
// queued buttons are ORed in (the real pad keeps working), with their own trigger / release edges and full pressure;
// sticks replace the real ones while given. It works whether or not the window has focus (the game zeroes the pad
// when it hasn't, before this runs).
//
// Pause and step use the game's own pause (aRoom::setEventPause, as the pause menu): see pause / step below.

namespace debug_server {
namespace {

using namespace mem;

constexpr uintptr_t kPadMove = 0x485050;          // sDevil4Pad::move, thiscall
constexpr uintptr_t kPadInfo0 = 0x5D0;            // sDevil4Pad::mPadInfo[0] (cPadInfo, 0x200)
constexpr uintptr_t kBtnOn = kPadInfo0 + 0x14;    // kBtnInfo mBtn: on, trg, rel, rep
constexpr uintptr_t kAnlg = kPadInfo0 + 0x24;     // kAnlgInfo mAnlg[2]: left, right (x, y floats)
constexpr uintptr_t kPress = kPadInfo0 + 0x34;    // kPressInfo mPress: 15 floats

struct Button {
    const char* name;
    uint32_t bit;
    int press;   // index into kPressInfo, -1 = none (Start)
};
// bits as Macro's PAD_BUTTON_*; kPressInfo order: Lup Lright Ldown Lleft (d-pad), Rup Rright Rdown Rleft (Y B A X),
// L1 L2 R1 R2 Select L3 R3
const Button kButtons[] = {
    {"select", 0x0001, 12}, {"l3", 0x0002, 13},  {"r3", 0x0004, 14},   {"start", 0x0008, -1},
    {"up", 0x0010, 0},      {"right", 0x0020, 1}, {"down", 0x0040, 2},  {"left", 0x0080, 3},
    {"l1", 0x0100, 8},      {"r1", 0x0200, 10},  {"l2", 0x0400, 9},    {"r2", 0x0800, 11},
    {"y", 0x1000, 4},       {"b", 0x2000, 5},    {"a", 0x4000, 6},     {"x", 0x8000, 7},
};
const std::pair<const char*, const char*> kAliases[] = {
    {"back", "select"}, {"ls", "l3"}, {"rs", "r3"}, {"lb", "l1"}, {"rb", "r1"}, {"lt", "l2"}, {"rt", "r2"},
    {"triangle", "y"}, {"circle", "b"}, {"cross", "a"}, {"square", "x"},
    {"dpad_up", "up"}, {"dpad_right", "right"}, {"dpad_down", "down"}, {"dpad_left", "left"},
};

uint32_t button_bit(std::string n) {
    n = lower(n);
    for (const auto& [a, b] : kAliases)
        if (n == a) n = b;
    for (const Button& b : kButtons)
        if (n == b.name) return b.bit;
    throw std::runtime_error("no button '" + n + "' (a b x y, l1 r1 l2 r2, l3 r3, start select, up down left right; "
                             "Xbox / PlayStation names work too)");
}

uint32_t buttons_of(const json& j) {
    if (j.is_null()) return 0;
    long long n = 0;
    if (parse_number(j, n) && !j.is_string()) return (uint32_t)n;
    if (j.is_string()) return button_bit(j.get<std::string>());
    if (j.is_array()) {
        uint32_t m = 0;
        for (const json& b : j) m |= buttons_of(b);
        return m;
    }
    throw std::runtime_error("buttons: names, a list of names or a bit mask");
}

struct Step {
    uint32_t buttons = 0;
    int frames = 1;
    bool has_left = false, has_right = false;
    float left[2] = {}, right[2] = {};
};

std::mutex g_mutex;
std::deque<Step> g_queue;
int g_left_in_step = 0;          // frames still to run of g_queue.front()
uint32_t g_prev = 0;             // the buttons injected last frame (for edges)
std::atomic<long long> g_queued_frames{0};
std::atomic<long long> g_pad_frames{0};

std::unique_ptr<FunctionHook> g_hook;
using PadMove = int(__fastcall*)(void* self, void* edx);
PadMove g_original = nullptr;

void apply(uintptr_t pad) {
    std::lock_guard<std::mutex> lock(g_mutex);
    uint32_t now = 0;
    const Step* s = nullptr;
    if (!g_queue.empty()) {
        if (g_left_in_step <= 0) g_left_in_step = g_queue.front().frames;
        s = &g_queue.front();
        now = s->buttons;
    }
    if (now || g_prev || s) {
        uint32_t btn[4];   // on, trg, rel, rep
        if (rd(pad + kBtnOn, btn)) {
            const uint32_t pressed = now & ~g_prev, released = g_prev & ~now;
            btn[0] |= now;
            btn[1] |= pressed;
            btn[2] |= released & ~btn[0];
            btn[3] |= pressed;
            safe_write(pad + kBtnOn, btn, sizeof(btn));
        }
        if (now) {
            float press[15];
            if (rd(pad + kPress, press)) {
                for (const Button& b : kButtons)
                    if ((now & b.bit) && b.press >= 0) press[b.press] = 1.0f;
                safe_write(pad + kPress, press, sizeof(press));
            }
        }
        if (s && (s->has_left || s->has_right)) {
            float anlg[4];
            if (rd(pad + kAnlg, anlg)) {
                if (s->has_left) anlg[0] = s->left[0], anlg[1] = s->left[1];
                if (s->has_right) anlg[2] = s->right[0], anlg[3] = s->right[1];
                safe_write(pad + kAnlg, anlg, sizeof(anlg));
            }
        }
    }
    g_prev = now;
    if (s && --g_left_in_step <= 0) {
        g_queue.pop_front();
        g_left_in_step = 0;
    }
    if (g_queued_frames.load() > 0) g_queued_frames--;
    if (g_queue.empty()) g_queued_frames = 0;
}

int __fastcall pad_move(void* self, void* edx) {
    const int r = g_original(self, edx);
    ++g_pad_frames;
    apply(reinterpret_cast<uintptr_t>(self));
    return r;
}

bool install_hook() {
    if (g_hook) return g_hook->is_valid();
    g_hook = std::make_unique<FunctionHook>(kPadMove, &pad_move);
    if (!g_hook->create()) {
        spdlog::error("DebugServer: can't hook sDevil4Pad::move (0x485050): the input tool won't work");
        return false;
    }
    g_original = g_hook->get_original<std::remove_pointer_t<PadMove>>();
    return true;
}

Step parse_step(const json& j) {
    Step s;
    s.buttons = j.contains("buttons") ? buttons_of(j["buttons"]) : 0;
    s.frames = std::clamp(j.value("frames", 2), 1, 60 * 60);
    auto stick = [&](const char* key, bool& has, float* v) {
        if (!j.contains(key)) return;
        const json& a = j[key];
        if (!a.is_array() || a.size() != 2) throw std::runtime_error(std::string(key) + ": [x, y], -1 to 1");
        v[0] = std::clamp(a[0].get<float>(), -1.0f, 1.0f);
        v[1] = std::clamp(a[1].get<float>(), -1.0f, 1.0f);
        has = true;
    };
    stick("left", s.has_left, s.left);
    stick("right", s.has_right, s.right);
    return s;
}

bool wait_for(const std::function<bool()>& done, double seconds) {
    const auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
    while (!done()) {
        if (std::chrono::steady_clock::now() > end) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    return true;
}

json input_tool(const json& a) {
    if (!g_original) throw std::runtime_error("the pad hook isn't installed (see the log)");
    std::vector<Step> steps;
    if (a.contains("sequence")) {
        if (!a["sequence"].is_array() || a["sequence"].empty()) throw std::runtime_error("sequence: a list of steps");
        for (const json& j : a["sequence"]) steps.push_back(parse_step(j));
    } else if (!a.value("release", false)) {
        steps.push_back(parse_step(a));
    }
    long long frames = 0;
    for (const Step& s : steps) frames += s.frames;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        if (a.value("release", false) || !a.value("append", false)) {
            g_queue.clear();
            g_left_in_step = 0;
            g_queued_frames = 0;
        }
        for (const Step& s : steps) g_queue.push_back(s);
        g_queued_frames += frames;
    }
    json out{{"steps", steps.size()}, {"frames", frames}, {"pad_frame", g_pad_frames.load()}};
    if (a.value("wait", false) && frames > 0) {
        const bool done = wait_for([] { return g_queued_frames.load() <= 0; }, std::min(60.0, frames / 20.0 + 5.0));
        out["finished"] = done;
        out["frame"] = (long long)frame_hooks::frames();
    }
    return out;
}

// ---- pause / step ----
//
// The game's own pause (the pause menu's, messages', events'): aRoom::setEventPause (0x40DEE0; al = kind, 0 = the
// game lines GameMoveLineTbl 0xDF3AD4: 1, 7-11, 13-16, 18-21, 23-25, 29; push on / off; retn 4) sets or clears
// MoveLine::mPause on those lines and pauses the effects on lines 19-21. The room's +0x31 says a pause is on: the pause
// menu (aRoom::pauseInChk 0x40DE10) won't open over it, and this tool won't pause over the game's own pause.

constexpr uintptr_t kSetEventPause = 0x40DEE0;
constexpr uintptr_t kRoomPaused = 0x31;   // aRoom: u8, a pause (menu, message, event) is on

std::atomic<int> g_step_left{0};
bool g_paused = false;        // this tool's pause (game thread)
uintptr_t g_paused_room = 0;  // the room it was set in

void set_event_pause(bool on) {
    const uint32_t arg = on ? 1 : 0;
    const uintptr_t fn = kSetEventPause;
    __asm {
        push arg
        xor eax, eax
        call fn
    }
}

uintptr_t current_room() {   // the room layer if it's an aRoom (not the mission title card), else 0
    sArea* area = devil4_sdk::get_sArea();
    const uintptr_t r = area ? reinterpret_cast<uintptr_t>(area->aRoomPtr) : 0;
    const uintptr_t dti = r ? dti_of(r) : 0;
    return dti && dti_is_a(dti, "aroom") ? r : 0;
}

void forget_if_room_changed() {
    if (g_paused && current_room() != g_paused_room) {   // a new room isn't paused
        g_paused = false;
        g_paused_room = 0;
        g_step_left = 0;
    }
}

json pause_tool(const json& a) {
    const bool on = a.value("on", true);
    return run_on_game_thread([on] {
        forget_if_room_changed();
        const uintptr_t room = current_room();
        if (!room) throw std::runtime_error("no room (pause works in a mission's room)");
        uint8_t flag = 0;
        rd(room + kRoomPaused, flag);
        if (on) {
            if (g_paused) return json{{"paused", true}, {"note", "already paused by this tool"}};
            if (flag) throw std::runtime_error("the game is paused already (pause menu, a message or an event)");
            const uint8_t one = 1;
            safe_write(room + kRoomPaused, &one, 1);
            set_event_pause(true);
            g_paused = true;
            g_paused_room = room;
            g_step_left = 0;
            return json{{"paused", true}, {"frame", (long long)frame_hooks::frames()}};
        }
        if (!g_paused) return json{{"paused", flag != 0}, {"note", "not paused by this tool (nothing done)"}};
        g_step_left = 0;
        set_event_pause(false);
        const uint8_t zero = 0;
        safe_write(room + kRoomPaused, &zero, 1);
        g_paused = false;
        g_paused_room = 0;
        return json{{"paused", false}, {"frame", (long long)frame_hooks::frames()}};
    });
}

json step_tool(const json& a) {
    const int frames = std::clamp(a.value("frames", 1), 1, 3600);
    const json r = run_on_game_thread([frames] {
        forget_if_room_changed();
        if (!g_paused) throw std::runtime_error("not paused by this tool (pause first)");
        if (g_step_left > 0) throw std::runtime_error("a step is running");
        set_event_pause(false);   // the room's paused flag stays on: Start can't open the menu mid-step
        g_step_left = frames;     // counted down by input_frame from the next frame hook on
        return json{{"frames", frames}, {"from_frame", (long long)frame_hooks::frames()}};
    });
    if (!a.value("wait", true)) return r;
    json out = r;
    out["finished"] = wait_for([] { return g_step_left.load() <= 0; }, frames / 20.0 + 5.0);
    out["frame"] = (long long)frame_hooks::frames();
    return out;
}

json wait_tool(const json& a) {
    const long long start = (long long)frame_hooks::frames();
    long long until = start + std::clamp(a.value("frames", 1), 0, 36000);
    if (a.contains("until_frame")) until = a["until_frame"].get<long long>();
    const double timeout = std::clamp(a.value("timeout", 30.0), 0.1, 300.0);
    const bool reached = wait_for([until] { return (long long)frame_hooks::frames() >= until; }, timeout);
    return json{{"reached", reached}, {"frame", (long long)frame_hooks::frames()}, {"waited", (long long)frame_hooks::frames() - start}};
}

json obj(std::initializer_list<std::pair<const char*, json>> props) {
    json p = json::object();
    for (const auto& [k, v] : props) p[k] = v;
    return json{{"type", "object"}, {"properties", p}};
}
json prop(const char* type, const char* description) { return json{{"type", type}, {"description", description}}; }

} // namespace

void input_frame() {
    // the frame hook (FRAME_END), before this frame's jobs: a step set by a job runs from the next frame
    forget_if_room_changed();
    if (g_step_left > 0 && --g_step_left == 0 && g_paused) set_event_pause(true);
}

json input_status() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return json{{"input_frames_queued", g_queued_frames.load()}, {"paused", g_paused}, {"stepping", g_step_left.load()}};
}

void register_input() {
    install_hook();
    json stick = json{{"type", "array"}, {"description", "[x, y], -1 to 1, replaces the pad's stick while held"}};
    json step_props = obj({{"buttons", json{{"description", "a name, a list of names or a bit mask: a b x y, l1 r1 l2 r2, "
                                                             "l3 r3, start select, up down left right (lb, rt, cross, "
                                                             "triangle ... work too)"}}},
                           {"frames", prop("integer", "frames held (default 2)")},
                           {"left", stick}, {"right", stick}})["properties"];
    json props = step_props;
    props["sequence"] = json{{"type", "array"}, {"items", json{{"type", "object"}, {"properties", step_props}}},
                             {"description", "steps one after another (a step without buttons = a gap)"}};
    props["append"] = prop("boolean", "add after what's queued (default: replace it)");
    props["release"] = prop("boolean", "drop everything queued");
    props["wait"] = prop("boolean", "return when the input has run (default false)");
    add_tool({"input",
              "Press pad buttons and move sticks as player 1's pad, from the next frame: buttons held for frames, or a "
              "sequence of steps. Works in menus and gameplay, with or without window focus; the real pad keeps "
              "working (buttons are ORed in). Each press gets its trigger edge and full pressure.",
              json{{"type", "object"}, {"properties", props}}, false, input_tool, true});
    add_tool({"pause",
              "Pause (on = true, default) or resume (on = false) the game the way its pause menu does (aRoom::"
              "setEventPause: the game's move lines and effects stop), without the menu; the pause menu can't open "
              "over it. Refused while the game's own pause (menu, message, event) is on. Needs a room; a room change "
              "ends it.",
              obj({{"on", prop("boolean", "true pause, false resume")}}), false, pause_tool, true});
    add_tool({"step",
              "While paused: run the units for `frames` frames (default 1) and pause again; returns when done unless "
              "wait = false.",
              obj({{"frames", prop("integer", "frames to run (1-3600)")}, {"wait", prop("boolean", "default true")}}),
              false, step_tool, true});
    add_tool({"wait",
              "Wait for game frames (the frame hook's count): `frames` from now (default 1) or until_frame; timeout "
              "in seconds (default 30). Doesn't hold the game.",
              obj({{"frames", prop("integer", "frames to wait")}, {"until_frame", prop("integer", "a frame count")},
                   {"timeout", prop("number", "seconds")}}),
              false, wait_tool});
}

} // namespace debug_server
