#include "Menus.hpp"
#include "Devil4.hpp"
#include "MtMemoryStream.hpp"
#include <cstdio>
#include <cstring>
#include <string>

namespace menus {
namespace {

uintptr_t message() { return *reinterpret_cast<const uintptr_t*>(kMessage); }

} // namespace

bool slot_loaded(int slot) {
    const uintptr_t m = message();
    if (!m || slot < 0 || slot >= kMessageSlots) return false;
    return *reinterpret_cast<const uint8_t*>(m + kSlotStride * slot + kSlotEnabled) == 1;
}

bool print(int slot, int x, int y, unsigned w, unsigned h, MtColor color, int prio, const char* text) {
    if (!text || !slot_loaded(slot)) return false;
    MtPoint pos{x, y};
    MtSize size{w, h};
    using Fn = void(__cdecl*)(uintptr_t, uint8_t, const MtPoint*, const MtSize*, MtColor, int, const char*, ...);
    reinterpret_cast<Fn>(kMessagePrintf)(message(), static_cast<uint8_t>(slot), &pos, &size, color, prio, "%s", text);
    return true;
}

// ---- the menus' font for print (moji00) ----

namespace {

constexpr uintptr_t kMessageDti = 0xE57738;         // rMessage::DTI (newInstance = its vtable slot 1, thiscall)
constexpr uintptr_t kSlotData = 0x40;               // slot data = sMessage + 0x40 + 0x340 * slot: +0x10 texture, +0x20 message
const uintptr_t kQueueReset = 0x496F80;             // usercall: al = slot, ecx = sMessage
const uintptr_t kSlotClear = 0x49B010;              // stdcall(slot data)
const uintptr_t kSlotSetMessage = 0x49B0B0;         // eax = slot data, stdcall(rMessage*)
const uintptr_t kSlotSetTexture = 0x49B070;         // esi = slot data, stdcall(rTexture*)

// Vibed\Menus\tools\make_ascii_moji00.py: message 0 = chars 0x20..0x7A as moji00 glyphs (# * < > backslash ^ = spaces)
// and the end entry (until 2026-11-19 it had none, and 'z' was drawn as nothing);
// the header as id\localize\USA\pc\msg\mission_menu (512 x 512 sheet, font 16 x 32, 8 colour bits)
const uint8_t kAsciiMoji00[] = {
    0x4D, 0x53, 0x47, 0x32, 0x40, 0x00, 0x00, 0x00, 0xB0, 0x01, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00,
    0x00, 0x02, 0x00, 0x02, 0x10, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x1C, 0x00, 0x28, 0x00, 0x14, 0x00, 0x17, 0x00, 0x28, 0x00, 0x28, 0x00, 0x1C, 0x00, 0x28, 0x00,
    0x36, 0x00, 0x38, 0x00, 0x16, 0x01, 0x51, 0x00, 0x1E, 0x00, 0x5D, 0x00, 0x2A, 0x00, 0x17, 0x00,
    0x18, 0x00, 0x25, 0x00, 0x1A, 0x00, 0x25, 0x00, 0x1C, 0x00, 0x28, 0x00, 0x30, 0x00, 0x44, 0x00,
    0x24, 0x00, 0x16, 0x00, 0x2E, 0x00, 0x20, 0x00, 0x26, 0x00, 0x17, 0x00, 0x32, 0x00, 0x49, 0x00,
    0x00, 0x00, 0x4D, 0x00, 0x02, 0x00, 0x2E, 0x00, 0x04, 0x00, 0x44, 0x00, 0x06, 0x00, 0x44, 0x00,
    0x08, 0x00, 0x4B, 0x00, 0x0A, 0x00, 0x46, 0x00, 0x0C, 0x00, 0x48, 0x00, 0x0E, 0x00, 0x42, 0x00,
    0x10, 0x00, 0x48, 0x00, 0x12, 0x00, 0x49, 0x00, 0x20, 0x00, 0x18, 0x00, 0x22, 0x00, 0x18, 0x00,
    0x1C, 0x00, 0x28, 0x00, 0x0C, 0x01, 0x3C, 0x00, 0x1C, 0x00, 0x28, 0x00, 0x16, 0x00, 0x3C, 0x00,
    0x34, 0x00, 0x65, 0x00, 0x38, 0x00, 0x61, 0x00, 0x3A, 0x00, 0x53, 0x00, 0x3C, 0x00, 0x52, 0x00,
    0x3E, 0x00, 0x5B, 0x00, 0x40, 0x00, 0x51, 0x00, 0x42, 0x00, 0x4D, 0x00, 0x44, 0x00, 0x5B, 0x00,
    0x46, 0x00, 0x4F, 0x00, 0x48, 0x00, 0x1E, 0x00, 0x4A, 0x00, 0x44, 0x00, 0x4C, 0x00, 0x55, 0x00,
    0x4E, 0x00, 0x4B, 0x00, 0x50, 0x00, 0x6F, 0x00, 0x52, 0x00, 0x5A, 0x00, 0x54, 0x00, 0x63, 0x00,
    0x56, 0x00, 0x48, 0x00, 0x58, 0x00, 0x6D, 0x00, 0x5A, 0x00, 0x51, 0x00, 0x5C, 0x00, 0x4D, 0x00,
    0x5E, 0x00, 0x51, 0x00, 0x60, 0x00, 0x51, 0x00, 0x62, 0x00, 0x54, 0x00, 0x64, 0x00, 0x7D, 0x00,
    0x66, 0x00, 0x57, 0x00, 0x68, 0x00, 0x51, 0x00, 0x6A, 0x00, 0x49, 0x00, 0xA0, 0x00, 0x1C, 0x00,
    0x1C, 0x00, 0x28, 0x00, 0xA2, 0x00, 0x1C, 0x00, 0x1C, 0x00, 0x28, 0x00, 0x1A, 0x01, 0x55, 0x00,
    0x2A, 0x00, 0x17, 0x00, 0x6C, 0x00, 0x3D, 0x00, 0x6E, 0x00, 0x47, 0x00, 0x70, 0x00, 0x40, 0x00,
    0x72, 0x00, 0x45, 0x00, 0x74, 0x00, 0x41, 0x00, 0x76, 0x00, 0x2B, 0x00, 0x78, 0x00, 0x43, 0x00,
    0x7A, 0x00, 0x45, 0x00, 0x7C, 0x00, 0x1D, 0x00, 0x7E, 0x00, 0x18, 0x00, 0x80, 0x00, 0x46, 0x00,
    0x82, 0x00, 0x1C, 0x00, 0x84, 0x00, 0x6D, 0x00, 0x86, 0x00, 0x44, 0x00, 0x88, 0x00, 0x45, 0x00,
    0x8A, 0x00, 0x46, 0x00, 0x8C, 0x00, 0x44, 0x00, 0x8E, 0x00, 0x2C, 0x00, 0x90, 0x00, 0x3B, 0x00,
    0x92, 0x00, 0x28, 0x00, 0x94, 0x00, 0x46, 0x00, 0x96, 0x00, 0x41, 0x00, 0x98, 0x00, 0x5F, 0x00,
    0x9A, 0x00, 0x41, 0x00, 0x9C, 0x00, 0x43, 0x00, 0x9E, 0x00, 0x37, 0x00, 0x00, 0x00, 0x01, 0x04,
};

uintptr_t g_font = 0;   // our rMessage (one reference of ours kept for good)

uintptr_t slot_data(uintptr_t m, int slot) { return m + kSlotData + kSlotStride * slot; }

uintptr_t make_font() {
    if (g_font) return g_font;
    const uintptr_t vt = *reinterpret_cast<const uintptr_t*>(kMessageDti);
    using NewInstance = void*(__thiscall*)(void*);
    const NewInstance new_instance = *reinterpret_cast<const NewInstance*>(vt + 4);
    void* r = new_instance(reinterpret_cast<void*>(kMessageDti));   // refcount 1
    if (!r) return 0;
    const uintptr_t l = reinterpret_cast<uintptr_t>(r);
    std::snprintf(reinterpret_cast<char*>(l + 0x04), 64, "%s", "dmc4_hook ascii_moji00");   // a name for debuggers only
    *reinterpret_cast<uint64_t*>(l + 0x58) = 0;                                              // no sResource table entry
    MtMemoryStream s(kAsciiMoji00, sizeof kAsciiMoji00);
    if (!load_resource_from(r, s)) {                                                         // rMessage::load 0x46D5F0
        devil4_sdk::release_resource(reinterpret_cast<cResource*>(r));
        return 0;
    }
    *reinterpret_cast<uint32_t*>(l + 0x50) = sizeof kAsciiMoji00;                            // mSize, loaded
    *reinterpret_cast<uint32_t*>(l + 0x4C) |= 1;
    return g_font = l;
}

uintptr_t moji00_texture(uintptr_t m) {   // the sheet, from whichever slot holds it
    for (int k = 0; k < kMessageSlots; ++k) {
        const uintptr_t tex = *reinterpret_cast<const uintptr_t*>(slot_data(m, k) + 0x10);
        if (tex && std::strstr(reinterpret_cast<const char*>(tex + 0x04), "moji00")) return tex;
    }
    return 0;
}

// what sMessage::createMessage 0x497290 does once it has the rMessage
void fill_slot(uintptr_t m, int slot, uintptr_t msg, uintptr_t tex) {
    const uintptr_t data = slot_data(m, slot);
    const uint8_t s = static_cast<uint8_t>(slot);
    __asm {
        pushad
        mov al, s
        mov ecx, m
        call dword ptr [kQueueReset]
        mov al, s
        mov ecx, m
        call dword ptr [kQueueReset]
        push data
        call dword ptr [kSlotClear]
        push msg
        mov eax, data
        call dword ptr [kSlotSetMessage]
        push tex
        mov esi, data
        call dword ptr [kSlotSetTexture]
        popad
    }
}

} // namespace

// ---- label fonts ----

namespace {

constexpr uintptr_t kTextureDti = 0xEAD680;         // rTexture::DTI

struct LabelFont {
    std::string path;
    uintptr_t msg = 0, tex = 0;   // our references, kept while the path is in use
    bool failed = false;          // not there (tried once per path)
};
LabelFont g_label;

bool loaded(uintptr_t r) { return r && (*reinterpret_cast<const uint32_t*>(r + 0x4C) & 1); }

void drop(LabelFont& f) {
    if (f.msg) devil4_sdk::release_resource(reinterpret_cast<cResource*>(f.msg));
    if (f.tex) devil4_sdk::release_resource(reinterpret_cast<cResource*>(f.tex));
    f.msg = f.tex = 0;
}

uintptr_t slot_message(int slot) {
    const uintptr_t m = message();
    return m && slot >= 0 && slot < kMessageSlots ? *reinterpret_cast<const uintptr_t*>(slot_data(m, slot) + 0x20) : 0;
}

// message k's entries (u16 code, u8 advance, u8 flag) of an rMessage: +0x60 the message index, +0x64 the header
const uint8_t* message_entries(uintptr_t msg, int k) {
    const uintptr_t index = *reinterpret_cast<const uintptr_t*>(msg + 0x60);
    const uintptr_t head = *reinterpret_cast<const uintptr_t*>(msg + 0x64);
    if (!index || !head || k >= *reinterpret_cast<const uint16_t*>(head + 0x1C)) return nullptr;
    return *reinterpret_cast<const uint8_t* const*>(index + 4 * k);
}

// a label font in `slot` (ours): its metrics (message 1) and cell size (2 x font width, font height)
struct Metrics {
    const uint8_t* m1 = nullptr;
    unsigned w = 0, h = 0;
};
Metrics metrics(int slot) {
    Metrics r;
    const uintptr_t msg = slot_message(slot);
    if (!msg || msg != g_label.msg || !slot_loaded(slot)) return r;
    const uintptr_t head = *reinterpret_cast<const uintptr_t*>(msg + 0x64);
    r.m1 = message_entries(msg, 1);
    r.w = 2u * *reinterpret_cast<const uint16_t*>(head + 0x14);
    r.h = *reinterpret_cast<const uint16_t*>(head + 0x16);
    return r;
}

} // namespace

bool load_label_font(const char* path, int slot) {
    if (!path || !*path) return false;
    const uintptr_t m = message();
    if (!m || slot < 0 || slot >= kMessageSlots) return false;
    if (g_label.path != path) {
        const uintptr_t held = slot_message(slot);
        if (held && held == g_label.msg) return false;   // another font of ours still in the slot: not swapped under it
        drop(g_label);
        g_label = LabelFont{path};
    }
    if (g_label.failed) return false;
    if (!g_label.msg) {
        const std::string p = g_label.path;
        g_label.msg = reinterpret_cast<uintptr_t>(devil4_sdk::get_stuff_from_files(reinterpret_cast<MtDTI*>(kMessageDti), const_cast<char*>(p.c_str()), 1));
        g_label.tex = reinterpret_cast<uintptr_t>(devil4_sdk::get_stuff_from_files(reinterpret_cast<MtDTI*>(kTextureDti), const_cast<char*>(p.c_str()), 1));
        if (!loaded(g_label.msg) || !loaded(g_label.tex) || !message_entries(g_label.msg, 1)) {
            drop(g_label);
            g_label.failed = true;
            return false;
        }
    }
    const uintptr_t held = slot_message(slot);
    if (held == g_label.msg && slot_loaded(slot)) return true;
    if (held && held != g_label.msg) return false;   // the game's
    fill_slot(m, slot, g_label.msg, g_label.tex);
    return slot_message(slot) == g_label.msg && slot_loaded(slot);
}

int label_width(const char* text, int slot) {
    const Metrics f = metrics(slot);
    if (!f.m1 || !text) return 0;
    int w = 0;
    for (const char* c = text; *c; ++c)
        if (*c >= 0x20 && *c < 0x7F) w += *reinterpret_cast<const uint16_t*>(f.m1 + 4 * (*c - 0x20));
    return w;
}

bool print_label(int x, int y, MtColor color, int prio, const char* text, int slot) {
    const Metrics f = metrics(slot);
    if (!f.m1 || !text) return false;
    char one[2] = {0, 0};
    for (const char* c = text; *c; ++c) {
        if (*c < 0x20 || *c >= 0x7F) continue;
        const uint8_t* e = f.m1 + 4 * (*c - 0x20);
        if (*c != ' ') {
            one[0] = *c;
            print(slot, x - e[2], y, f.w, f.h, color, prio, one);
        }
        x += *reinterpret_cast<const uint16_t*>(e);
    }
    return true;
}

bool menu_font_ready(int slot) {
    const uintptr_t m = message();
    if (!m || !g_font || slot < 0 || slot >= kMessageSlots) return false;
    return *reinterpret_cast<const uintptr_t*>(slot_data(m, slot) + 0x20) == g_font && slot_loaded(slot);
}

bool load_menu_font(int slot) {
    if (menu_font_ready(slot)) return true;
    const uintptr_t m = message();
    if (!m || slot < 0 || slot >= kMessageSlots) return false;
    const uintptr_t held = *reinterpret_cast<const uintptr_t*>(slot_data(m, slot) + 0x20);
    if (held && held != g_font) return false;   // the game's: never taken over
    const uintptr_t tex = moji00_texture(m);
    const uintptr_t font = tex ? make_font() : 0;
    if (!font) return false;
    fill_slot(m, slot, font, tex);
    return menu_font_ready(slot);
}

} // namespace menus
