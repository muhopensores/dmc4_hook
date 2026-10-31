#pragma once

#include "../mod.hpp"
#include "WallRun.hpp"     // wallrun::at / wallrun::pl offsets; shares no state with WallRun

// NeroStyle: styles for Nero, switched like Dante's. A style is a value in uPlayer::mStyle (+0x156C), the field the
// move table's row filter compares with a row's style condition (word 6: checkAtckStyle 0x7AC990, checkAtckSub, -1 =
// any). Every vanilla Nero row says -1 and nothing of Nero's writes the field (the base constructor sets -1), so a
// value here gates only the rows that ask for it: Nero's own Devil Bringer rows (Snatch / Buster, see gate_bringer),
// Gerb's (MoveTable, see gate_gerb) and MoveDef rows with `style=`.
//
// Input: the four style buttons. uPlayer::getInterfaceBtnInfo 0x7A96D0 maps them into mInterface.mBtnTrg for both
// characters, always from Dante's bindings (Trickster 0x1000, Royal Guard 0x2000, Gunslinger 0x4000, Swordmaster
// 0x8000; d-pad left / down / right / up by default), and only uPlayerDante::checkStyleChange reads them, so on Nero they
// are free. Each button is a slot with a name and an on / off; slot n is style value BASE + n (5-8: Dante's own values
// 0-4 are also what GuardTimer and PinProperties test, which don't check the character). The user's order: slot 0 (up)
// is the **Devil Bringer** (the vanilla Snatch / Buster rows, attr 4 / 5 of Nero's table, carry its value while the
// mod gates them, so the grab belongs to that style), slot 1 (right) is **Gerbera** (Gerb's rows). The selection starts
// as the Devil Bringer, so the vanilla moves work until a switch.
//
// The switch follows checkStyleChange: a press goes into a pending style at once (kept through the cooldown), the
// field changes once the 12-frame cooldown is over. The selection is the mod's (a new player object starts at -1 again:
// the pre-check tick writes it back) and is saved in the config. No pose, effect or sound yet (the user: the switch
// first, "stance switch poses and efx later").
//
// Other mods read the style with nero_style::current(player); Gerb's rows get gerb_condition() in their style word.

namespace nero_style {

constexpr int kSlots = 4;
constexpr int BASE   = 5;            // mStyle of slot n = BASE + n
constexpr int NONE   = -1;           // STYLE_KIND_NONE, the constructor's value
enum Slot : int { SLOT_SM = 0 /* Swordmaster button, up */, SLOT_GS = 1 /* Gunslinger, right */, SLOT_TS = 2 /* Trickster, left */, SLOT_RG = 3 /* Royal Guard, down */ };

struct StyleSlot {
    bool enabled;
    char name[32];
};

extern bool      mod_enabled;
extern StyleSlot slots[kSlots];
extern float     cooldown_frames;    // frames between a press and the change (Dante: 12)
extern bool      press_again_clears; // the current style's button again -> no style
extern bool      gate_bringer;       // the vanilla Snatch / Buster rows carry the Devil Bringer slot's value as their style condition
extern int       bringer_slot;       // the slot that is the Devil Bringer
extern bool      gate_gerb;          // Gerb's rows carry the Gerbera slot's value as their style condition
extern int       gerb_slot;          // the slot that is Gerbera

int      value_of(int slot);         // BASE + slot
int      selected();                 // the chosen style value (NONE = none)
int      current(uintptr_t player);  // the style of this player: its mStyle for a Nero with the mod on, else NONE
uint32_t gerb_condition();           // Gerb's rows' style word: the Gerbera value with the mod on and gate_gerb, else -1 (any)
uint32_t bringer_condition();        // the Devil Bringer rows' style word, likewise

} // namespace nero_style

class NeroStyle : public Mod {
public:
    NeroStyle() = default;
    std::string get_mod_name() override { return "NeroStyle"; };

    std::optional<std::string> on_initialize() override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_gui_frame(int display) override;

    std::unique_ptr<FunctionHook> hook_cmd;
};
