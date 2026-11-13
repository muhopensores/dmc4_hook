#pragma once
#include <cstdint>

// Think trace (EnemyAI; a checkbox in the Debug tab opens its own window, dev builds): what an enemy's AI decided and why. Each cThinkMgr::GetActionNo
// 0x45C240 call of the traced enemy (the one player 1 is locked on to) is a decision: its inputs (distance, angle,
// height, motion state, level, the mbCheck flags, the bit field), every row thinkCheck 0x45C5E0 looked at (table name
// and index from the SE PDB's names matched to DX9 by content, EnemyAIThinkTables.inc), what each row returned, and for
// a turned-down row the first gate that failed, worked out again from the row and the manager the way thinkCheck tests
// them (Vibed\RE\think_mgr.md). The hooks are EnemyAI's (EnemyAI.cpp); this file records and draws.
namespace think_trace {

extern bool enabled;

// The state thinkCheck's gates read that it changes itself (the random block), taken before the call, and where the
// row is in its table.
struct Pre {
    uintptr_t row = 0;          // the row's address in its table (0 = not found)
    uint8_t next = 0, rnd_start = 0;
    float sub = 0.0f;
    int act_pass = -1;
};

bool begin(uintptr_t mgr, uint32_t mot, float leng, float deg, float height, int lv);   // GetActionNo starts: true = traced
bool recording(uintptr_t mgr);                                                         // a row of the traced decision
Pre before(uintptr_t mgr, const void* row);     // before thinkCheck (the row as the caller passed it)
void after(uintptr_t mgr, const void* row, const Pre& pre, int result);   // after it (the row as it was tested)
void end(int result);                           // GetActionNo returned
void draw_toggle();                             // the Debug tab's checkbox
void draw_window();                             // its window (on_frame), like MoveTable's

} // namespace think_trace
