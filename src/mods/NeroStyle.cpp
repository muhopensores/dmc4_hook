#include "NeroStyle.hpp"
#include "Gerb.hpp"
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
    { false, "" },
};
float cooldown_frames    = 12.0f;
bool  press_again_clears = false;
bool  gate_bringer       = true;
int   bringer_slot       = SLOT_SM;
bool  gate_gerb          = true;
int   gerb_slot          = SLOT_GS;

namespace {

constexpr uintptr_t kStyle = 0x156C;                                      // uPlayer::mStyle.kind
constexpr uint32_t  kSlotBit[kSlots] = { 0x8000, 0x4000, 0x1000, 0x2000 };   // mBtnTrg: Swordmaster, Gunslinger, Trickster, Royal Guard
const char* const kSlotButton[kSlots] = { "Swordmaster button (d-pad up)", "Gunslinger button (d-pad right)",
                                          "Trickster button (d-pad left)", "Royal Guard button (d-pad down)" };

int g_selected = BASE + SLOT_SM;   // the chosen style value, one for the game (as Dante's save slot holds one); the Devil Bringer at first
uint32_t g_presses = 0, g_switches = 0;   // readout

// The cooldown per Nero player object (co-op or a new object after a load each get their own).
struct State { uintptr_t player = 0; int pending = NONE; float timer = 0.0f; uint32_t last_seen = 0; };
State g_states[4];

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
    victim->pending = g_selected;
    victim->last_seen = tick;
    return *victim;
}

bool is_nero(uintptr_t pl) { return pl && at<int>(pl, pl::mPlayerID) == 1; }

const char* name_of(int value) {
    const int slot = value - BASE;
    if (slot < 0 || slot >= kSlots) return value == NONE ? "none" : "?";
    return slots[slot].name[0] ? slots[slot].name : kSlotButton[slot];
}

void apply_bringer_rows();   // below

// The pre-check tick (after executeCommandCheck): the press, the cooldown, the field.
void tick(uintptr_t pl) {
    if (!mod_enabled || !is_nero(pl)) return;
    State& s = state_for(pl);
    if (s.timer > 0.0f) s.timer -= at<float>(pl, pl::m_delta_time);
    const uint32_t trg = at<uint32_t>(pl, pl::mBtnTrg);
    for (int i = 0; i < kSlots; ++i) {
        if (!(trg & kSlotBit[i]) || !slots[i].enabled) continue;
        const int v = value_of(i);
        s.pending = (press_again_clears && v == g_selected) ? NONE : v;
        ++g_presses;
        break;
    }
    if (s.timer <= 0.0f && s.pending != g_selected) {
        g_selected = s.pending;
        s.timer = cooldown_frames;
        ++g_switches;
    }
    int& st = at<int>(pl, kStyle);
    if (st != g_selected) st = g_selected;     // also the write-back on a new player object (the constructor's -1)
    apply_bringer_rows();
}

// Nero's own Devil Bringer rows: attr 4 / 5 with ids 1 Snatch, 2 air Snatch, 5 Buster, 6 air Buster (rows 27-34 of the
// vanilla table). Their style word isn't read: the base uPlayer::checkAtckSub 0x7AC710 handles attr 0 / 1 / 2 only and
// uPlayerNero::checkAtckSub 0x7E5B00 runs attr 4 / 5 itself (lock-on + the grab press, the etc check), without the style
// compare. So the gate switches the attr: 4 / 5 while the Devil Bringer is picked, 6 / 7 otherwise, which neither function
// handles (checkAtck 0x803B70 only compares attr with 3, the terminator, and 2). Walked in MoveTable's copy (the added rows
// in front); ids past 0x33 are added rows and stay as their mods wrote them. Run every pre-check tick: the switch is live.
constexpr uint32_t kAttrOff = 2;   // 4 -> 6, 5 -> 7
void apply_bringer_rows() {
    if (!HookNeroKADTbl) return;
    const bool on = bringer_rows_on();
    kAtckDefTbl* row = (kAtckDefTbl*)HookNeroKADTbl;
    for (int i = 0; i < 256 && row->atckAttr != 3; ++i, ++row) {
        if (row->atckId > 0x33) continue;
        if (!on && (row->atckAttr == 4 || row->atckAttr == 5)) row->atckAttr += kAttrOff;
        else if (on && (row->atckAttr == 4 + kAttrOff || row->atckAttr == 5 + kAttrOff)) row->atckAttr -= kAttrOff;
    }
}

// The style words of the gated rows follow the mod (and the field goes back to -1 when the mod is turned off).
void apply(bool reset_field) {
    apply_bringer_rows();
    gerb::apply_row();
    if (reset_field)
        if (uPlayer* lp = devil4_sdk::get_local_player(); lp && is_nero((uintptr_t)lp)) at<int>((uintptr_t)lp, kStyle) = NONE;
}

} // namespace

int value_of(int slot) { return (slot < 0 || slot >= kSlots) ? NONE : BASE + slot; }
int selected() { return mod_enabled ? g_selected : NONE; }
int current(uintptr_t player) { return (mod_enabled && is_nero(player)) ? at<int>(player, kStyle) : NONE; }
uint32_t gerb_condition() { return (mod_enabled && gate_gerb) ? (uint32_t)value_of(gerb_slot) : 0xFFFFFFFFu; }
bool bringer_rows_on() { return !(mod_enabled && gate_bringer) || g_selected == value_of(bringer_slot); }

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
                  "Slot values: 5 (up, the Devil Bringer), 6 (right, Gerbera), 7 (left), 8 (down). The Devil Bringer is picked "
                  "at the start, so the vanilla moves work until a switch. No stance pose, flash or sound yet."));
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
        snprintf(help, sizeof(help), "Style value %d. On: this button picks it. The name is for the readout only.", value_of(i));
        help_marker(help);
        ImGui::PopID();
    }
    ImGui::SliderFloat(_("Switch cooldown"), &cooldown_frames, 0.0f, 60.0f, "%.0f frames");
    ImGui::SameLine();
    help_marker(_("Frames after a change before the next one takes (Dante's is 12). A press during the cooldown is kept and applies when it ends."));
    ImGui::Checkbox(_("Pressing the current style again clears it"), &press_again_clears);
    ImGui::SameLine();
    help_marker(_("On: the picked style's button goes back to no style (-1). Off: nothing happens, as for Dante without Dark Slayer."));

    ImGui::TextUnformatted(_("Moves"));
    const char* names[kSlots];
    for (int i = 0; i < kSlots; ++i) names[i] = slots[i].name[0] ? slots[i].name : kSlotButton[i];
    ImGui::Checkbox(_("Snatch and Buster need their style"), &gate_bringer);
    ImGui::SameLine();
    help_marker(_("On: the game's eight Devil Bringer rows (Snatch, air Snatch, Buster, air Buster, and their second set) are "
                  "switched off while another style is picked, so the grab only works in the Devil Bringer. Off: they work in "
                  "every style, as in the vanilla game."));
    if (gate_bringer) {
        ImGui::SetNextItemWidth(200.0f);
        ImGui::Combo(_("Devil Bringer slot"), &bringer_slot, names, kSlots);
        ImGui::SameLine();
        help_marker(_("Which style is the Devil Bringer (Snatch / Buster)."));
    }
    ImGui::Checkbox(_("Gerb needs its style"), &gate_gerb);
    ImGui::SameLine();
    help_marker(_("On: Gerb's two rows (0x40 / 0x41) carry the Gerbera slot's value as their style condition, so the jump only works "
                  "while that style is picked. Off: Gerb works in every style, as without this mod."));
    if (gate_gerb) {
        ImGui::SetNextItemWidth(200.0f);
        ImGui::Combo(_("Gerbera slot"), &gerb_slot, names, kSlots);
        ImGui::SameLine();
        help_marker(_("Which style is Gerbera (the Gerb jump). MoveDef rows use the value directly: style=\"6\" for the second slot."));
    }
    apply_bringer_rows();  // every frame while the panel is open: cheap, and the rows follow the controls above
    gerb::apply_row();

    if (uPlayer* lp = devil4_sdk::get_local_player()) {    // live readout
        const uintptr_t P = (uintptr_t)lp;
        if (is_nero(P)) {
            const State& s = state_for(P);
            ImGui::Text("style %d (%s) | field %d | pending %d | cooldown %.0f | presses %u switches %u | Bringer rows %s | Gerb rows style %d",
                        g_selected, name_of(g_selected), at<int>(P, kStyle), s.pending, std::max(s.timer, 0.0f), g_presses, g_switches,
                        bringer_rows_on() ? "on" : "off", (int)gerb_condition());
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
        slots[i].enabled = cfg.get<bool>(k + "_on").value_or(slots[i].enabled);
        if (auto n = cfg.get(k + "_name")) strcpy_s(slots[i].name, n->c_str());
    }
    cooldown_frames    = cfg.get<float>("nero_style_cooldown").value_or(cooldown_frames);
    press_again_clears = cfg.get<bool>("nero_style_press_again_clears").value_or(press_again_clears);
    gate_bringer       = cfg.get<bool>("nero_style_gate_bringer").value_or(gate_bringer);
    bringer_slot       = std::clamp(cfg.get<int>("nero_style_bringer_slot").value_or(bringer_slot), 0, kSlots - 1);
    gate_gerb          = cfg.get<bool>("nero_style_gate_gerb").value_or(gate_gerb);
    gerb_slot          = std::clamp(cfg.get<int>("nero_style_gerb_slot").value_or(gerb_slot), 0, kSlots - 1);
    g_selected         = cfg.get<int>("nero_style_selected").value_or(g_selected);
    if (g_selected != NONE && (g_selected < BASE || g_selected >= BASE + kSlots)) g_selected = value_of(bringer_slot);
    apply(false);
}

void NeroStyle::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("nero_style", mod_enabled);
    for (int i = 0; i < kSlots; ++i) {
        const std::string k = "nero_style_slot_" + std::to_string(i);
        cfg.set<bool>(k + "_on", slots[i].enabled);
        cfg.set(k + "_name", std::string(slots[i].name));
    }
    cfg.set<float>("nero_style_cooldown", cooldown_frames);
    cfg.set<bool>("nero_style_press_again_clears", press_again_clears);
    cfg.set<bool>("nero_style_gate_bringer", gate_bringer);
    cfg.set<int>("nero_style_bringer_slot", bringer_slot);
    cfg.set<bool>("nero_style_gate_gerb", gate_gerb);
    cfg.set<int>("nero_style_gerb_slot", gerb_slot);
    cfg.set<int>("nero_style_selected", g_selected);
}
