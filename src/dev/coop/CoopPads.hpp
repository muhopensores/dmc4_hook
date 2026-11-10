#pragma once

#include "Coop.hpp"
#include "../../sdk/Pad.hpp"
#include <cstdint>

// CoopPads: input for every coop player, read on the game thread.
//
// Player 1 keeps the game's sDevil4Pad (sDevil4Pad::mpInstance 0xE559C4, mPadInfo[0]) unless he picks an XInput pad,
// which gives him a stand-in like the others (his camera's stick then reads it through CoopContext). Players 2-4 each get a stand-in
// sDevil4Pad that is never constructed by the game (sPad::sPad 0x8E3D60 would overwrite the singleton and open
// DirectInput): only mPadInfo[0] and mPad[0].Data are filled, once a frame from XInputGetState, with the game's own
// mapping (sub_8E4950 for the bits, sDevil4Pad::cPadInfo::update 0x485F20 for sticks and press values). The camera hooks
// read these stand-ins like the real pad.
//
// uPlayer::cPeripheral::update 0x7AFD10 is replaced while coop is on (Macro's pad fallback): the same algorithm, reading
// the pad of the player's control number, with the stick keys only for the player who owns the keyboard (the binding set
// by that player's mPlayerID +0x1494). The other two keyboard reads in uPlayer::setInterface 0x7A91E0 are gated the same
// way: the setInterfaceInfoKB call 0x7A94A1 and the Exceed key 0x7A9440.
//
// Rumble: uPlayer::setPadVibration 0x7AC130 (edx = player, eax = vibration index) and the stop at 0x7AB817
// (updateContactInfoSub, esi = player) always drive the game's pad. For players 2-4 they are redirected to an envelope
// kept here (the game's PAD_VIB_LIST rules: start / end level, length in frames) sent with XInputSetState.
//
// Button layout per player (players 2-4): game button i (bit 1 << i of the raw pad bits, the layout sub_8E4950 gives an
// XInput pad) is pressed while the player's own button remap[i] is, so each player can move buttons around before the
// game's own controller config (per character, uPlayer::getInterfaceBtnInfo 0x7A96D0) turns them into actions. The
// stick directions (bits 16-23) and the analog sticks are not remapped; a trigger keeps its analog value wherever it
// goes.

namespace coop_pads {

extern int  keyboard_owner;      // control number that uses the keyboard, -1 = nobody
extern bool rumble;              // rumble for players 2-4

constexpr int kButtons = 16;
extern int remap[coop::kMaxPlayers][kButtons];   // remap[player][game button] = the player's button, -1 = none

bool init();                                 // resolves XInput; false = no XInput DLL
bool uses_stand_in(int control);             // players 2-4, and player 1 with an XInput pad picked
sDevil4Pad* pad_for(int control);            // the stand-in, or the game's pad for player 1 on it
bool connected(int control);
void set_keyboard_owner(int control);
void stop_all();
void reset_remap(int control);
void draw_bindings(int control);                 // the Buttons tree of one player in the Coop panel

bool __stdcall update_peripheral(cPeripheral* peripheral);   // Macro pad fallback

// Hook sites (absolute) and their detours, installed by Coop::on_initialize.
constexpr uintptr_t kTickSite       = 0x8AEFA1;  // sDevil4Main move loop, before the pad system's move
constexpr uintptr_t kKbMergeSite    = 0x7A94A1;  // uPlayer::setInterface: push esi; call setInterfaceInfoKB
constexpr uintptr_t kKbExceedSite   = 0x7A9440;  // uPlayer::setInterface: the Exceed key's sKeyboard read
constexpr uintptr_t kVibSite        = 0x7AC130;  // uPlayer::setPadVibration entry
constexpr uintptr_t kVibStopSite    = 0x7AB817;  // uPlayer::updateContactInfoSub: stopVibration on the game's pad

extern uintptr_t ret_kb_merge, ret_kb_exceed, ret_vib, ret_vib_stop;
void detour_kb_merge();
void detour_kb_exceed();
void detour_vib();
void detour_vib_stop();
void __stdcall game_tick();

} // namespace coop_pads
