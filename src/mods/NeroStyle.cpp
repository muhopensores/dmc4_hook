#include "NeroStyle.hpp"
#include "Gerb.hpp"
#include "NeroGunslinger.hpp"
#include "MoveTable.hpp"
#include "../sdk/Devil4.hpp"
#include "../sdk/InputCommand.hpp"
#include <algorithm>
#include <cstdio>
#include <string>

using namespace wallrun;

namespace nero_style {

bool      mod_enabled = false;
StyleSlot slots[kSlots] = {
    { true,  "Devil Bringer" },   // up: Snatch / Buster
    { true,  "Gerbera" },         // right: Gerb
    { false, "" },
    { true,  "Gunslinger" },      // down: the Gunslinger mod
};

namespace {

constexpr float kCooldownFrames = 12.0f;     // frames between a change and the next one (Dante's)
// The styles the gated moves belong to: the vanilla Snatch / Buster rows, Gerb's rows, the Gunslinger mod's rows.
constexpr int kBringerSlot    = SLOT_SM;     // up
constexpr int kGerbSlot       = SLOT_GS;     // right
constexpr int kGunslingerSlot = SLOT_RG;     // down

constexpr uintptr_t kStyle = 0x156C;                                      // uPlayer::mStyle.kind
constexpr uint32_t  kSlotBit[kSlots] = { 0x8000, 0x4000, 0x1000, 0x2000 };   // mBtnTrg: Swordmaster, Gunslinger, Trickster, Royal Guard
const char* const kSlotButton[kSlots] = { "Swordmaster button (d-pad up)", "Gunslinger button (d-pad right)",
                                          "Trickster button (d-pad left)", "Royal Guard button (d-pad down)" };

// The chosen style value per player, by uPlayer::padID (+0x1405: 0 = player 1, co-op's players 2-4 = 1-3); player 1's is
// the one saved. The Devil Bringer at first.
constexpr uintptr_t kPadID = 0x1405;
constexpr int       kPads  = 4;
int g_selected[kPads] = { BASE + SLOT_SM, BASE + SLOT_SM, BASE + SLOT_SM, BASE + SLOT_SM };

int& selected_for(uintptr_t pl) {
    const uint8_t id = at<uint8_t>(pl, kPadID);
    return g_selected[id < kPads ? id : 0];
}
uint32_t g_presses = 0, g_switches = 0;   // readout

// The cooldown per Nero player object (co-op or a new object after a load each get their own).
struct State { uintptr_t player = 0; int pending = NONE; float timer = 0.0f; uint32_t last_seen = 0;
               uint32_t switches = 0; int switched_to = NONE; };
State g_states[8];

State& state_for(uintptr_t pl) {
    static uint32_t tick = 0;
    ++tick;
    for (State& s : g_states) if (s.player == pl) { s.last_seen = tick; return s; }
    State* victim = &g_states[0];
    for (State& s : g_states) {
        if (s.player == 0) { victim = &s; break; }
        if (s.last_seen < victim->last_seen) victim = &s;
    }
    *victim = State{};
    victim->player = pl;
    victim->pending = selected_for(pl);
    victim->last_seen = tick;
    return *victim;
}

bool is_nero(uintptr_t pl) { return pl && at<int>(pl, pl::mPlayerID) == 1; }

const char* name_of(int value) {
    const int slot = value - BASE;
    if (slot < 0 || slot >= kSlots) return value == NONE ? "none" : "?";
    return slots[slot].name[0] ? slots[slot].name : kSlotButton[slot];
}

void apply_bringer_rows(int selected);   // below

// The pre-check tick (after executeCommandCheck): the press, the cooldown, the field.
void tick(uintptr_t pl) {
    if (!mod_enabled || !is_nero(pl)) return;
    State& s = state_for(pl);
    int& sel = selected_for(pl);
    if (s.timer > 0.0f) s.timer -= at<float>(pl, pl::m_delta_time);
    const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
    for (int i = 0; i < kSlots; ++i) {
        if (!(trg & kSlotBit[i]) || !slots[i].enabled) continue;
        s.pending = value_of(i);     // the current style's button again changes nothing, as for Dante
        ++g_presses;
        break;
    }
    if (s.timer <= 0.0f && s.pending != sel) {
        sel = s.pending;
        s.timer = kCooldownFrames;
        ++g_switches;
        ++s.switches;
        s.switched_to = sel;
    }
    int& st = at<int>(pl, kStyle);
    if (st != sel) st = sel;     // also the write-back on a new player object (the constructor's -1)
    apply_bringer_rows(sel);     // the table is shared: set for this Nero, whose attack check comes next in his update
}

// Nero's own Devil Bringer rows: attr 4 / 5 with ids 1 Snatch, 2 air Snatch, 5 Buster, 6 air Buster (rows 27-34 of the
// vanilla table). Their style word isn't read: the base uPlayer::checkAtckSub 0x7AC710 handles attr 0 / 1 / 2 only and
// uPlayerNero::checkAtckSub 0x7E5B00 runs attr 4 / 5 itself (lock-on + the grab press, the etc check), without the style
// compare. So the gate switches the attr: 4 / 5 while the Devil Bringer is picked, 6 / 7 otherwise, which neither function
// handles (checkAtck 0x803B70 only compares attr with 3, the terminator, and 2). Walked in MoveTable's copy (the added rows
// in front); ids past 0x33 are added rows and stay as their mods wrote them. Run every pre-check tick: the switch is live.
constexpr uint32_t kAttrOff = 2;   // 4 -> 6, 5 -> 7
bool rows_on_for(int selected) { return !mod_enabled || selected == value_of(kBringerSlot); }
void apply_bringer_rows(int selected) {
    if (!HookNeroKADTbl) return;
    const bool on = rows_on_for(selected);
    kAtckDefTbl* row = (kAtckDefTbl*)HookNeroKADTbl;
    for (int i = 0; i < 256 && row->atckAttr != 3; ++i, ++row) {
        if (row->atckId > 0x33) continue;
        if (!on && (row->atckAttr == 4 || row->atckAttr == 5)) row->atckAttr += kAttrOff;
        else if (on && (row->atckAttr == 4 + kAttrOff || row->atckAttr == 5 + kAttrOff)) row->atckAttr -= kAttrOff;
    }
}

// The style words of the gated rows follow the mod (and the field goes back to -1 when the mod is turned off).
void apply(bool reset_field) {
    apply_bringer_rows(g_selected[0]);
    gerb::apply_row();
    gunslinger::apply_row();
    if (reset_field)
        if (uPlayer_old* lp = devil4_sdk::get_local_player(); lp && is_nero((uintptr_t)lp)) at<int>((uintptr_t)lp, kStyle) = NONE;
}

} // namespace

int value_of(int slot) { return (slot < 0 || slot >= kSlots) ? NONE : BASE + slot; }
int selected() { return mod_enabled ? g_selected[0] : NONE; }
int current(uintptr_t player) { return (mod_enabled && is_nero(player)) ? at<int>(player, kStyle) : NONE; }
uint32_t switches(uintptr_t player, int* style) {
    for (const State& s : g_states) {
        if (!player || s.player != player) continue;
        if (style) *style = s.switched_to;
        return s.switches;
    }
    if (style) *style = NONE;
    return 0;
}
uint32_t gerb_condition() { return mod_enabled ? (uint32_t)value_of(kGerbSlot) : 0xFFFFFFFFu; }
uint32_t gunslinger_condition() { return mod_enabled ? (uint32_t)value_of(kGunslingerSlot) : 0xFFFFFFFFu; }
bool bringer_rows_on() { return rows_on_for(g_selected[0]); }

} // namespace nero_style

// ---------------------------------------------------------------------------------------------------------------
// The Mod: framework plumbing only.
// ---------------------------------------------------------------------------------------------------------------
using namespace nero_style;

std::optional<std::string> NeroStyle::on_initialize() {
    input_commands::add_tick(&tick);
    if (!input_commands::hook_installed) {     // one installer: whichever command mod initialises first
        if (!install_hook_absolute(0x7A83B5, hook_cmd, &input_commands::detour, &input_commands::jmp_ret, 8)) {
            spdlog::error("NeroStyle: failed to hook uPlayer::main after executeCommandCheck (0x7A83B5)");
            return "Failed to init NeroStyle (command hook)";
        }
        input_commands::hook_installed = true;
    }
    return Mod::on_initialize();
}

void NeroStyle::on_gui_frame(int display) {
    if (display != DISPLAY_SYSTEM_A) return;
    if (ImGui::Checkbox(_("Nero styles"), &mod_enabled)) apply(!mod_enabled);
    ImGui::SameLine();
    help_marker(_("Styles for Nero, switched with the style buttons like Dante's (the d-pad with the default bindings; the "
                  "buttons do nothing on Nero otherwise). A style is a number in the player's style field: the move table "
                  "runs a row only when the row's style condition is -1 (any) or equal to it, so moves made for a style "
                  "(the vanilla Snatch / Buster, Gerb's rows, MoveDef rows with style=) are only available while it is picked. "
                  "Slot values: 5 (up, the Devil Bringer), 6 (right, Gerbera), 7 (left), 8 (down, Gunslinger). The Devil Bringer is picked "
                  "at the start, so the vanilla moves work until a switch. A switch takes 12 frames, as Dante's; the current "
                  "style's button again changes nothing. The stand can play a pose on a switch (Guardian Devil, per style); "
                  "Nero has no pose, flash or sound yet."));
    if (!mod_enabled) return;
    ImGui::Indent(lineIndent);
    ImGui::PushID("NeroStyle");

    ImGui::TextUnformatted(_("Styles"));
    for (int i = 0; i < kSlots; ++i) {
        ImGui::PushID(i);
        ImGui::Checkbox("##on", &slots[i].enabled);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        ImGui::InputText(kSlotButton[i], slots[i].name, sizeof(slots[i].name));
        ImGui::SameLine();
        char help[160];
        snprintf(help, sizeof(help), "Style value %d. On: this button picks it. The name is a label (the readout, Guardian Devil's sets).", value_of(i));
        help_marker(help);
        ImGui::PopID();
    }
    if (uPlayer_old* lp = devil4_sdk::get_local_player()) {    // live readout
        const uintptr_t P = (uintptr_t)lp;
        if (is_nero(P)) {
            const State& s = state_for(P);
            const int sel = selected_for(P);
            ImGui::Text("style %d (%s) | field %d | pending %d | cooldown %.0f | presses %u switches %u | Bringer rows %s | Gerb rows style %d | Gunslinger rows style %d",
                        sel, name_of(sel), at<int>(P, kStyle), s.pending, std::max(s.timer, 0.0f), g_presses, g_switches,
                        bringer_rows_on() ? "on" : "off", (int)gerb_condition(), (int)gunslinger_condition());
        } else {
            ImGui::TextUnformatted(_("(the local player isn't Nero)"));
        }
    }
    ImGui::PopID();
    ImGui::Unindent(lineIndent);
}

void NeroStyle::on_config_load(const utility::Config& cfg) {
    mod_enabled        = cfg.get<bool>("nero_style").value_or(false);
    for (int i = 0; i < kSlots; ++i) {
        const std::string k = "nero_style_slot_" + std::to_string(i);
        // a slot saved without a name while it now has a default one was saved by a build without that style: it keeps
        // the default name and on / off, so a style added later isn't switched off by an old config
        const auto n = cfg.get(k + "_name");
        if ((!n || n->empty()) && slots[i].name[0]) continue;
        slots[i].enabled = cfg.get<bool>(k + "_on").value_or(slots[i].enabled);
        if (n) strcpy_s(slots[i].name, n->c_str());
    }
    int saved = cfg.get<int>("nero_style_selected").value_or(g_selected[0]);
    if (saved != NONE && (saved < BASE || saved >= BASE + kSlots)) saved = value_of(kBringerSlot);
    for (int& v : g_selected) v = saved;
    apply(false);
}

void NeroStyle::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_style", mod_enabled);
    for (int i = 0; i < kSlots; ++i) {
        const std::string k = "nero_style_slot_" + std::to_string(i);
        cfg.set<bool>(k + "_on", slots[i].enabled);
        cfg.set(k + "_name", std::string(slots[i].name));
    }
    cfg.set<int>("nero_style_selected", g_selected[0]);
}
