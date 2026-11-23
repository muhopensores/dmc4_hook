#pragma once

#include "../../mod.hpp"
#include "../../sdk/Selection.hpp"
#include "../postfx/PostFX.hpp"
#include <cstdint>
#include <vector>

// ObjectPicker: Ctrl + click in the game view selects what's under the cursor (sdk/Selection, shared with PostFX's
// outline, the debug server's pick / selection tools and UnitPacks' lists), and a window shows what it is: class, move
// line, position, the .sdl track that made it and the tracks that key it (sdk/Schedulers), the UnitPacks spawn it came
// from; a gizmo moves it. Plan: Vibed\PostFX\README.md "Object picker", Vibed\StageEditor\README.md (the editor's
// selection). The pick itself: PostFX reads the frame's depth (world point + object ID) and resolve() turns that into a
// unit on the game thread.
namespace picker {

struct Candidate {
    uintptr_t unit = 0;
    uintptr_t dti = 0;
    int       line = 0;
    int       joint = -1;      // -1: scored by its position
    float     dist = 0.0f;
    float     at[3] = {};      // the joint's (or the unit's) world position
};
// every model unit on the move lines, nearest first by its joint closest to p (game cm). Game thread.
std::vector<Candidate> score_units(const float p[3]);

struct Result {
    uintptr_t              unit = 0;
    selection::By          by = selection::By::None;
    std::vector<Candidate> candidates;   // nearest first, at most 5
};
// What a depth sample picked: the unit that drew the pixel (its object ID), else the model whose nearest joint is within
// max_dist cm. Game thread.
Result resolve(const post_fx::DepthSample& s, float max_dist);

int line_of(uintptr_t unit);   // the move line holding the unit, -1 if none. Game thread.
// on a move line, or a model that drew lately (parts other units own and draw: Nero's coat, weapons). Game thread.
bool alive(uintptr_t unit);

} // namespace picker

class ObjectPicker : public Mod {
public:
    ObjectPicker() = default;
    std::string get_mod_name() override { return "ObjectPicker"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
