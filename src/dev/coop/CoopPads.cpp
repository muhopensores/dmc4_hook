#include "CoopPads.hpp"
#include "Coop.hpp"
#include <Xinput.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#pragma push_macro("naked")
#undef naked
#define naked_extern __declspec(naked)

namespace coop_pads {

int  keyboard_owner = 0;
bool rumble         = true;
int  remap[coop::kMaxPlayers][kButtons] = {
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
    {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15},
};

uintptr_t ret_kb_merge  = 0;
uintptr_t ret_kb_exceed = 0;
uintptr_t ret_vib       = 0;
uintptr_t ret_vib_stop  = 0;

} // namespace coop_pads

namespace {

constexpr uintptr_t kDevil4Pad        = 0xE559C4; // sDevil4Pad::mpInstance
constexpr uintptr_t kKeyboard         = 0xE559C0; // sKeyboard::mpInstance
constexpr uintptr_t kSave             = 0xE558C8; // sSave::mpInstance
constexpr uintptr_t kPlayerID         = 0x1494;   // uPlayer::mPlayerID (0 Dante, 1 Nero: the keyboard binding set)
constexpr uintptr_t kActorType        = 0xEA4;    // uActor::mActorType (setPadVibration needs 1)
constexpr uintptr_t kPadVibParam      = 0x1EC0;   // uPlayer::mPadVibParam.mpData (float table)
constexpr uintptr_t kKeysDante        = 0x174;    // sSave mDante.mActBtnKB[]
constexpr uintptr_t kKeysNero         = 0x25C;    // sSave mNero.mActBtnKB[]
constexpr uintptr_t kPeripheral       = 0x1370;   // uPlayer::mPeripheral

template <class T> inline T& at(uintptr_t base, uintptr_t off) { return *reinterpret_cast<T*>(base + off); }

using XInputGetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_STATE*);
using XInputSetStateFn = DWORD(WINAPI*)(DWORD, XINPUT_VIBRATION*);
XInputGetStateFn xinput_get = nullptr;
XInputSetStateFn xinput_set = nullptr;

// Stand-in pads for players 2-4 (index 0 unused), never constructed by the game.
alignas(16) uint8_t stand_in_storage[coop::kMaxPlayers][sizeof(sDevil4Pad)];
bool stand_in_ready[coop::kMaxPlayers] = {};
bool stand_in_connected[coop::kMaxPlayers] = {};

// Control number owning the keyboard as the asm gates compare it (0xFF = nobody).
uint8_t kb_owner_byte = 0;
uintptr_t set_interface_info_kb = 0x7A9870;   // uPlayer::setInterfaceInfoKB (__stdcall uPlayer*)
uintptr_t skip_kb_exceed = 0x7A9463;           // setInterface: "Exceed key not down"
uintptr_t skip_vib_stop  = 0x7AB824;           // updateContactInfoSub: after the stopVibration call
uintptr_t update_anlg_info_fn = 0x7B0250;      // uPlayer::cPeripheral::updateAnlgInfo (esi = kAnlg)

sDevil4Pad* game_pad() { return *reinterpret_cast<sDevil4Pad**>(kDevil4Pad); }

sDevil4Pad* stand_in(int control) {
    sDevil4Pad* pad = reinterpret_cast<sDevil4Pad*>(stand_in_storage[control]);
    if (!stand_in_ready[control]) {
        std::memset(pad, 0, sizeof(sDevil4Pad));
        if (sDevil4Pad* real = game_pad()) {
            std::memcpy(pad, real, sizeof(void*)); // vtable, in case anything calls through it
        }
        pad->mActive = true;
        pad->mIsFixUserPadId = true;
        pad->mUserPadId = 0;
        pad->mPadInfo[0].mpPad = &pad->mPad[0];
        pad->mPadInfo[0].mIsEnable = true;
        pad->mPadInfo[0].mIsVibEnable = true;
        stand_in_ready[control] = true;
    }
    return pad;
}

// sub_8E4950: XInput state -> the game's button bits.
uint32_t map_buttons(const XINPUT_GAMEPAD& g, int trigger_free, int analog_free) {
    static const struct { WORD xi; uint32_t bit; } kButtons[] = {
        {XINPUT_GAMEPAD_START, 0x8},       {XINPUT_GAMEPAD_BACK, 0x1},
        {XINPUT_GAMEPAD_DPAD_UP, 0x10},    {XINPUT_GAMEPAD_DPAD_DOWN, 0x40},
        {XINPUT_GAMEPAD_DPAD_LEFT, 0x80},  {XINPUT_GAMEPAD_DPAD_RIGHT, 0x20},
        {XINPUT_GAMEPAD_Y, 0x1000},        {XINPUT_GAMEPAD_B, 0x2000},
        {XINPUT_GAMEPAD_A, 0x4000},        {XINPUT_GAMEPAD_X, 0x8000},
        {XINPUT_GAMEPAD_LEFT_SHOULDER, 0x100}, {XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x200},
        {XINPUT_GAMEPAD_LEFT_THUMB, 0x2},  {XINPUT_GAMEPAD_RIGHT_THUMB, 0x4},
    };
    uint32_t n = 0;
    for (const auto& b : kButtons) {
        if (g.wButtons & b.xi) {
            n |= b.bit;
        }
    }
    if (g.bLeftTrigger > trigger_free)  { n |= 0x400; }
    if (g.bRightTrigger > trigger_free) { n |= 0x800; }
    if (g.sThumbLY > analog_free)  { n |= 0x10000; }
    if (g.sThumbLX > analog_free)  { n |= 0x20000; }
    if (g.sThumbLY < -analog_free) { n |= 0x40000; }
    if (g.sThumbLX < -analog_free) { n |= 0x80000; }
    if (g.sThumbRY > analog_free)  { n |= 0x100000; }
    if (g.sThumbRX > analog_free)  { n |= 0x200000; }
    if (g.sThumbRY < -analog_free) { n |= 0x400000; }
    if (g.sThumbRX < -analog_free) { n |= 0x800000; }
    return n;
}

// Names of the raw pad bits 0-15 (the XInput buttons sub_8E4950 maps to them).
const char* kButtonNames[coop_pads::kButtons] = {"Back", "L3", "R3", "Start", "D-pad up", "D-pad right", "D-pad down",
                                                 "D-pad left", "LB", "RB", "LT", "RT", "Y", "B", "A", "X"};

// Button capture for the Buttons tree: the next button the player presses goes to game button capture_button.
int capture_player = -1, capture_button = -1;
uint32_t last_phys[coop::kMaxPlayers] = {};
uint32_t held_mask[coop::kMaxPlayers] = {};   // the button just bound, ignored until released

// The player's button bits as the game should see them.
uint32_t remap_bits(int control, uint32_t phys) {
    uint32_t n = phys & ~0xFFFFu;
    for (int i = 0; i < coop_pads::kButtons; i++) {
        const int src = coop_pads::remap[control][i];
        if (src >= 0 && (phys & (1u << src))) { n |= 1u << i; }
    }
    return n;
}

// Analog value of the game's LT / RT (game button 10 / 11): a trigger keeps its travel, a button reads 0 or 255.
uint8_t trigger_value(int control, int game_button, const XINPUT_GAMEPAD& g, uint32_t phys) {
    const int src = coop_pads::remap[control][game_button];
    if (src == 10) { return (phys & 0x400) ? g.bLeftTrigger : 0; }
    if (src == 11) { return (phys & 0x800) ? g.bRightTrigger : 0; }
    return (src >= 0 && (phys & (1u << src))) ? 255 : 0;
}

// Binds the first button newly pressed during a capture; the game button that had it takes this one's old button.
uint32_t capture(int control, uint32_t phys) {
    held_mask[control] &= phys;
    phys &= ~held_mask[control];
    const uint32_t pressed = phys & ~last_phys[control] & 0xFFFF;
    last_phys[control] = phys;
    if (capture_player != control) {
        return phys;
    }
    if (pressed) {
        int src = 0;
        while (!(pressed & (1u << src))) { src++; }
        int (&map)[coop_pads::kButtons] = coop_pads::remap[control];
        for (int j = 0; j < coop_pads::kButtons; j++) {
            if (j != capture_button && map[j] == src) { map[j] = map[capture_button]; }
        }
        map[capture_button] = src;
        held_mask[control] |= 1u << src;
        capture_player = capture_button = -1;
    }
    return phys & ~0xFFFFu; // the player's buttons do nothing while binding
}

// One frame of a stand-in pad: PAD_DATA as sub_8E4950 writes it, then mPadInfo[0] as cPadInfo::update does.
void fill_stand_in(int control, sDevil4Pad* pad, const XINPUT_GAMEPAD* g) {
    int trigger_free = 30, analog_free = 7849;
    if (sDevil4Pad* real = game_pad()) {
        if (real->mPad[0].Free.Trigger_free) { trigger_free = real->mPad[0].Free.Trigger_free; }
        if (real->mPad[0].Free.Analog_free)  { analog_free = real->mPad[0].Free.Analog_free; }
    }
    PAD_DATA& d = pad->mPad[0].field10_0x15c;
    const uint32_t old = d.On;
    const uint32_t phys = g ? capture(control, map_buttons(*g, trigger_free, analog_free)) : 0;
    const uint32_t n = remap_bits(control, phys);
    d.On  = n;
    d.Old = old;
    d.Trg = n & (old ^ n);
    d.Rel = (old ^ n) & ~n;
    d.Chg = old ^ n;
    d.Rep = 0;
    d.Lx = g ? g->sThumbLX : 0;
    d.Ly = g ? g->sThumbLY : 0;
    d.Rx = g ? g->sThumbRX : 0;
    d.Ry = g ? g->sThumbRY : 0;
    d.Lz = g ? trigger_value(control, 10, *g, phys) : 0;
    d.Rz = g ? trigger_value(control, 11, *g, phys) : 0;

    cPadInfo& pi = pad->mPadInfo[0];
    pi.mIsEnable = true;
    pi.mIsFake = false;
    pi.mBtn.on  = d.On;
    pi.mBtn.trg = d.Trg;
    pi.mBtn.rel = d.Rel;
    pi.mBtn.rep = d.Rep;
    pi.mAnlg[0].x = d.Lx * 0.000030518509f;
    pi.mAnlg[0].y = d.Ly * 0.000030518509f;
    pi.mAnlg[1].x = d.Rx * 0.000030518509f;
    pi.mAnlg[1].y = d.Ry * 0.000030518509f;
    kPressInfo& p = pi.mPress;
    p.L2 = d.Lz * 0.0039215689f;
    p.R2 = d.Rz * 0.0039215689f;
    auto on = [n](uint32_t bit) { return (n & bit) ? 1.0f : 0.0f; };
    p.Lup = on(0x10);
    p.Lright = on(0x20);
    p.Ldown = on(0x40);
    p.Lleft = on(0x80);
    p.Rup = on(0x1000);
    p.Rright = on(0x2000);
    p.Rdown = on(0x4000);
    p.Rleft = on(0x8000);
    p.L1 = on(0x100);
    p.R1 = on(0x200);
    p.Select = on(0x1);
    p.L3 = on(0x2);
    p.R3 = on(0x4);
}

// Rumble envelopes for players 2-4 (PAD_VIB_LIST rules: level 0-255 from start to end over the length).
struct Envelope {
    bool  on = false;
    float ms_left = 0.0f, level = 0.0f, add = 0.0f;
};
struct Rumble {
    Envelope high[8], low[8];
    WORD sent_left = 0, sent_right = 0;
    bool dirty = true;
};
Rumble rumbles[coop::kMaxPlayers];
LARGE_INTEGER last_tick = {};
LONGLONG next_retry[coop::kMaxPlayers] = {};   // QPC time of the next read of a missing pad

void add_envelope(Envelope (&list)[8], int start, int end, float ms) {
    Envelope* slot = &list[0];
    for (auto& e : list) {
        if (!e.on) { slot = &e; break; }
        if (e.ms_left < slot->ms_left) { slot = &e; }
    }
    slot->on = true;
    slot->ms_left = ms;
    slot->level = (float)start;
    slot->add = (float)(end - start) / ms;
}

float tick_envelopes(Envelope (&list)[8], float dt) {
    float level = 0.0f;
    for (auto& e : list) {
        if (!e.on) { continue; }
        e.ms_left -= dt;
        e.level += e.add * dt;
        if (e.ms_left <= 0.0f) {
            e = Envelope{};
            continue;
        }
        level = (std::max)(level, e.level);
    }
    return level;
}

void send_rumble(int control, WORD left, WORD right) {
    Rumble& r = rumbles[control];
    if (!xinput_set || (!r.dirty && left == r.sent_left && right == r.sent_right)) {
        return;
    }
    XINPUT_VIBRATION v{left, right};
    if (xinput_set((DWORD)coop::players[control].xinput, &v) == ERROR_SUCCESS) {
        r.sent_left = left;
        r.sent_right = right;
        r.dirty = false;
    }
}

void clear_rumble(int control) {
    Rumble& r = rumbles[control];
    for (auto& e : r.high) { e = Envelope{}; }
    for (auto& e : r.low) { e = Envelope{}; }
    r.dirty = true;
    send_rumble(control, 0, 0);
}

int control_of(uintptr_t player) {
    int c = at<uint8_t>(player, coop::kControlNumber);
    return (c < coop::kMaxPlayers) ? c : -1;
}

void __stdcall rumble_start(uintptr_t player, int index) {
    const int c = control_of(player);
    if (c < 1 || !coop_pads::rumble || at<int>(player, kActorType) != 1) {
        return;
    }
    sDevil4Pad* real = game_pad();
    if (real && !real->mPadInfo[0].mIsVibEnable) {
        return; // vibration turned off in the game's options
    }
    const float* data = at<const float*>(player, kPadVibParam);
    if (!data) {
        return;
    }
    const float* f = data + 5 * index + 90;
    const float ms = f[4] / 60.0f * 1000.0f;
    if (ms <= 0.0f) {
        return;
    }
    add_envelope(rumbles[c].high, (int)f[0], (int)f[1], ms);
    add_envelope(rumbles[c].low, (int)f[2], (int)f[3], ms);
}

void __stdcall rumble_stop(uintptr_t player) {
    const int c = control_of(player);
    if (c >= 1) {
        clear_rumble(c);
    }
}

bool key_down(const sKeyboard* kb, uint32_t vk) {
    return vk && ((kb->mState.on[(vk >> 5) & 7] >> (vk & 31)) & 1);
}

// cPeripheral::update's dead zone: below 0.24 on both axes the stick reads 0, else -127..127.
void read_stick(const kAnlgInfo& in, kAnlg& out) {
    if (std::fabs(in.x) >= 0.24f || std::fabs(in.y) >= 0.24f) {
        out.x = (short)(int)(in.x * 127.0f);
        out.y = (short)(int)(in.y * 127.0f);
    } else {
        out.x = 0;
        out.y = 0;
    }
}

// cPeripheral::update's stick keys: keys[] = the player's mActBtnKB (0 = unbound), extra = sKeyboard on[3] bits (numpad)
// or 0 for the right stick.
void stick_keys(const sKeyboard* kb, kAnlg& a, uint32_t right, uint32_t left, uint32_t up, uint32_t down,
                uint32_t extra_right, uint32_t extra_left, uint32_t extra_up, uint32_t extra_down) {
    const uint32_t numpad = kb->mState.on[3];
    if (std::abs((int)a.x) < 0x40) {
        if (key_down(kb, right) || (numpad & extra_right)) { a.x = 127; }
        if (key_down(kb, left) || (numpad & extra_left)) { a.x = (a.x >= 64) ? 0 : -127; }
    }
    if (std::abs((int)a.y) < 0x40) {
        if (key_down(kb, up) || (numpad & extra_up)) { a.y = 127; }
        if (key_down(kb, down) || (numpad & extra_down)) { a.y = (a.y >= 64) ? 0 : -127; }
    }
}

void update_anlg_info(kAnlg* anlg) {
    __asm {
        pushad
        mov esi, anlg
        call dword ptr [update_anlg_info_fn]
        popad
    }
}

} // namespace

namespace coop_pads {

bool init() {
    const char* names[] = {"xinput1_3.dll", "xinput9_1_0.dll", "xinput1_4.dll"};
    HMODULE dll = nullptr;
    for (const char* name : names) {
        if ((dll = GetModuleHandleA(name))) { break; }
    }
    for (const char* name : names) {
        if (dll) { break; }
        dll = LoadLibraryA(name);
    }
    if (!dll) {
        return false;
    }
    xinput_get = reinterpret_cast<XInputGetStateFn>(GetProcAddress(dll, "XInputGetState"));
    xinput_set = reinterpret_cast<XInputSetStateFn>(GetProcAddress(dll, "XInputSetState"));
    return xinput_get != nullptr;
}

sDevil4Pad* pad_for(int control) {
    if (control == 0) {
        return game_pad();
    }
    if (control < 0 || control >= coop::kMaxPlayers) {
        return nullptr;
    }
    return stand_in(control);
}

bool connected(int control) {
    if (control == 0) {
        sDevil4Pad* real = game_pad();
        return real && real->mPad[0].Info.Kind != 0;
    }
    return control > 0 && control < coop::kMaxPlayers && stand_in_connected[control];
}

void set_keyboard_owner(int control) {
    keyboard_owner = control;
    kb_owner_byte = (control >= 0 && control < coop::kMaxPlayers) ? (uint8_t)control : 0xFF;
}

void stop_all() {
    for (int c = 1; c < coop::kMaxPlayers; c++) {
        clear_rumble(c);
    }
}

void __stdcall game_tick() {
    LARGE_INTEGER now, freq;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&freq);
    float dt = last_tick.QuadPart ? (float)((now.QuadPart - last_tick.QuadPart) * 1000.0 / freq.QuadPart) : 0.0f;
    last_tick = now;
    dt = (std::min)(dt, 100.0f);

    for (int c = 1; c < coop::kMaxPlayers; c++) {
        sDevil4Pad* pad = stand_in(c);
        XINPUT_STATE st{};
        const int index = coop::players[c].xinput;
        // XInputGetState on an empty slot is slow (it looks for the device), so a missing pad is retried once a
        // second, as sPad::open does for the game's pad.
        const bool due = stand_in_connected[c] || now.QuadPart >= next_retry[c];
        bool ok = false;
        if (c < (int)Coop::player_num && due && xinput_get && index >= 0 && index < XUSER_MAX_COUNT) {
            ok = xinput_get((DWORD)index, &st) == ERROR_SUCCESS;
            if (!ok) {
                next_retry[c] = now.QuadPart + freq.QuadPart;
            }
        }
        if (ok != stand_in_connected[c]) {
            rumbles[c].dirty = true;
        }
        stand_in_connected[c] = ok;
        fill_stand_in(c, pad, ok ? &st.Gamepad : nullptr);

        if (!ok) {
            continue;
        }
        const float high = tick_envelopes(rumbles[c].high, dt);
        const float low = tick_envelopes(rumbles[c].low, dt);
        auto speed = [](float level) { return (WORD)((std::min)((std::max)(level, 0.0f), 255.0f) * 257.0f); };
        send_rumble(c, rumble ? speed(high) : 0, rumble ? speed(low) : 0);
    }
}

void reset_remap(int control) {
    for (int i = 0; i < kButtons; i++) {
        remap[control][i] = i;
    }
}

void draw_bindings(int control) {
    const bool open = ImGui::TreeNode(_("Buttons"));
    ImGui::SameLine();
    Mod::help_marker(_("This player's own button layout. The game's controller settings for the character still apply "
                       "on top: this only moves which of their buttons counts as which pad button"));
    if (!open) {
        return;
    }
    for (int i = 0; i < kButtons; i++) {
        ImGui::PushID(i);
        ImGui::TextUnformatted(kButtonNames[i]);
        ImGui::SameLine(Mod::sameLineItemWidth);
        const bool waiting = capture_player == control && capture_button == i;
        const int src = remap[control][i];
        const char* label = waiting ? _("Press a button...") : (src >= 0 ? kButtonNames[src] : _("none"));
        if (ImGui::Button(label, ImVec2(Mod::sameLineItemWidth, 0.0f))) {
            if (waiting) {
                capture_player = capture_button = -1;
            } else {
                capture_player = control;
                capture_button = i;
            }
        }
        ImGui::SameLine();
        Mod::help_marker(_("Click, then press the button this player wants for this pad button (click again to cancel). "
                           "The pad button that had it gets this one's old button"));
        ImGui::PopID();
    }
    if (ImGui::Button(_("Default layout"))) {
        reset_remap(control);
    }
    ImGui::SameLine();
    Mod::help_marker(_("Every button back where the pad has it"));
    ImGui::TreePop();
}

bool __stdcall update_peripheral(cPeripheral* peri) {
    if (!Coop::mod_enabled) {
        return false;
    }
    const uintptr_t player = (uintptr_t)peri - kPeripheral;
    const int control = *reinterpret_cast<uint8_t*>((uintptr_t)peri + 0x95);
    sDevil4Pad* pad = (control < (int)Coop::player_num) ? pad_for(control) : nullptr;

    if (pad) {
        const cPadInfo& pi = pad->mPadInfo[0];
        peri->mPadBtnOn = pi.mBtn.on;
        peri->mPadBtnTrg = pi.mBtn.trg;
        peri->mPadBtnRel = pi.mBtn.rel;
        std::memcpy(peri->mPadBtnPress, &pi.mPress, sizeof(peri->mPadBtnPress));
        read_stick(pi.mAnlg[0], peri->mAnlgL);
        read_stick(pi.mAnlg[1], peri->mAnlgR);
    } else {
        peri->mPadBtnOn = peri->mPadBtnTrg = peri->mPadBtnRel = 0;
        std::memset(peri->mPadBtnPress, 0, sizeof(peri->mPadBtnPress));
        peri->mAnlgL.x = peri->mAnlgL.y = 0;
        peri->mAnlgR.x = peri->mAnlgR.y = 0;
    }

    const sKeyboard* kb = *reinterpret_cast<sKeyboard**>(kKeyboard);
    const uintptr_t save = *reinterpret_cast<uintptr_t*>(kSave);
    if (kb && save && control == keyboard_owner) {
        const uint32_t id = at<uint32_t>(player, kPlayerID);
        auto key = [&](int i) -> uint32_t {
            if (id == 0) { return at<uint32_t>(save, kKeysDante + i * 4); }
            if (id == 1) { return at<uint32_t>(save, kKeysNero + i * 4); }
            return 0;
        };
        stick_keys(kb, peri->mAnlgL, key(18), key(17), key(15), key(16), 0x40, 0x10, 0x100, 0x20);
        stick_keys(kb, peri->mAnlgR, key(22), key(21), key(19), key(20), 0, 0, 0, 0);
    }
    update_anlg_info(&peri->mAnlgL);
    update_anlg_info(&peri->mAnlgR);
    return true;
}

// setInterface 0x7A94A1 (esi = player): setInterfaceInfoKB only for the keyboard's owner.
naked_extern void detour_kb_merge() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne merge
        mov al, byte ptr [esi+0x1405]
        cmp al, byte ptr [kb_owner_byte]
        jne skip
    merge:
        push esi
        call dword ptr [set_interface_info_kb]
    skip:
        jmp dword ptr [ret_kb_merge]
    }
}

// setInterface 0x7A9440 (esi = player, ecx = the key, xmm0 / xmm1 / xmm4 live): the Exceed key only for the owner.
naked_extern void detour_kb_exceed() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        push eax
        mov al, byte ptr [esi+0x1405]
        cmp al, byte ptr [kb_owner_byte]
        pop eax
        jne skip
    originalcode:
        mov edx, 0x00E559C0
        mov edx, [edx]
        jmp dword ptr [ret_kb_exceed]
    skip:
        jmp dword ptr [skip_kb_exceed]
    }
}

// setPadVibration 0x7AC130 (edx = player, eax = index, plain retn): players 2-4 get their own rumble.
naked_extern void detour_vib() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        cmp byte ptr [edx+0x1405], 0
        je originalcode
        pushad
        push eax
        push edx
        call rumble_start
        popad
        ret
    originalcode:
        sub esp, 0x1C
        cmp dword ptr [edx+0xEA4], 1
        jmp dword ptr [ret_vib]
    }
}

// updateContactInfoSub 0x7AB817 (esi = player): stop the rumble of players 2-4 on their own pad.
naked_extern void detour_vib_stop() {
    __asm {
        cmp byte ptr [Coop::mod_enabled], 1
        jne originalcode
        cmp byte ptr [esi+0x1405], 0
        je originalcode
        pushad
        push esi
        call rumble_stop
        popad
        jmp dword ptr [skip_vib_stop]
    originalcode:
        mov ecx, 0x00E559C4
        mov ecx, [ecx]
        jmp dword ptr [ret_vib_stop]
    }
}

} // namespace coop_pads

#undef naked_extern
#pragma pop_macro("naked")
