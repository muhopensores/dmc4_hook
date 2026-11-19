#pragma once

#include <functional>
#include <string>

// A row of our own in the pause menu, opening a list menu (rows of "label  < value >"), DX9 (read in IDA 2026-11-19 with
// SE's names; Vibed\RE\pause_menu.md). The practice mode plugin's Practice submenu goes through it (plugin API v5
// api->menus).
//
// uPauseMenu (0xFC, vtable 0xBE2270) has 5 rows (+0x3C), cursor +0x40, mMenuEnable[5] +0xF4: 0 Items, 1 Skill List,
// 2 Map, 3 System, 4 Return. A decided row fades to black, creates a child unit (+0xF0) on unit line 26, waits for its
// isEnd (vtable +0x50, polled in sync), reads its getDecValue (+0x54: 0 = back to the pause menu) and kills it (+0x30).
// The row table holds 5 rows, so our row takes the Map row's place (a map means nothing in a custom room): its label
// sprites (13 normal, 18 highlighted) and description (font 2) are hidden and ours drawn there, and the Map child
// (uMapMgr, built by `call 0x774450` at 0x7A01A1) gets our vtable: a uMapMgr whose move / transMain / isEnd /
// getDecValue are the list menu's. The list menu fades in, runs, fades out, then reports its end, as the game's own
// pause children do.
namespace pause_menu {

struct ListMenu {
    std::string title;
    std::function<int()> rows;                                               // how many rows
    std::function<void(int row, std::string& label, std::string& value)> text;   // value "" = an action row (A runs it)
    std::function<void(int row, int step)> change;                          // left / right: -1 / +1
    std::function<void(int row)> decide;                                    // A on a row
};

struct Row {
    int room = -1;               // only in this room (aRoom room number); -1 = every room
    std::string label;           // drawn in place of "Map"
    std::string description;     // the menu's description line while the row is selected
    ListMenu menu;
};

bool install();                  // once (idempotent); false if the game's code isn't the expected bytes
void set_row(const Row& row);    // the row from now on (one at a time: the last one set)
void clear_row();
bool menu_open();                // the list menu is running now

} // namespace pause_menu
