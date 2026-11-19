#pragma once

// From the Mission Menu into a room (DX9, read in IDA 2026-11-18; Vibed\RE\menus.md "uMissionMenu").
//   uMissionMenu ends when its state +0x14 is 4 (isEnd), and aMissionSelect then starts mission getOptionalValue
//   (+0x70 + 4 * row, the mission the menu holds for that difficulty) on difficulty getDecValue (the cursor; sub_408E80).
//   A difficulty row's decide path (result, step +0x17 = 2) fades out, saves the cursor (setDanteStyle) and reaches
//   state 4 by itself. So aGame's room_id / init_jump are set (AreaJump), the menu is put on a difficulty row at that
//   step, and the mission start takes the jump: the room loads instead of the mission's first room. Done in the menu's
//   own frame (a thunk on uMissionMenu::main, vtable 0xBDFD88 slot 17), while it's selecting.
//   Checked: Mission Menu -> room 900 through Mission 20's mission-start screen (needs a press) and its first room.
namespace mission_menu {

bool install();             // the main thunk, once (idempotent); false if the vtable slot isn't the game's
bool in_mission_menu();     // the Mission Menu ran in the last ~0.2 s

// Into room `room`: from a room, AreaJump's jump; from the Mission Menu, the menu's difficulty (the selected row, or the
// last played one) and its mission, starting in that room. False when neither applies (title, loading, another screen)
// or the room doesn't exist. Any thread.
bool enter_room(int room);

} // namespace mission_menu
