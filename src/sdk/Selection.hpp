#pragma once

#include <cstdint>

// One selection for every tool that points at a unit in the game, so they work on the same thing:
// - the object picker (dev/picker: Ctrl + click in the game view; the debug server's `pick`) selects what's under the
//   cursor and remembers the world point it hit;
// - lists select from their rows (UnitPacks' spawns, later the stage editor's track tree);
// - PostFX outlines the selected unit (its object ID in the depth texture);
// - the picker's window shows what the selected unit is, which .sdl track made it (sdk/Schedulers) and moves it.
// Any thread. The unit is a plain address: readers check it is still alive (picker::line_of) before touching it.
namespace selection {

enum class By : uint8_t {
    None = 0,
    Id,      // the object ID the unit drew into the depth texture at the picked pixel (exact)
    Joint,   // the unit whose nearest joint is closest to the picked point (no ID there)
    List,    // a list's Select button
    Tool,    // the debug server's `selection` tool
};
const char* by_name(By by);

void select(uintptr_t unit, By by);   // 0 = clear
void clear();
uintptr_t unit();
By by();
uint32_t revision();                  // changes with every select / clear / point

// The last picked world point (game cm, Y up), whether or not a unit was there: where UnitPacks can spawn things.
void set_point(const float p[3]);
bool point(float out[3]);             // false: nothing picked yet
void forget_point();

} // namespace selection
