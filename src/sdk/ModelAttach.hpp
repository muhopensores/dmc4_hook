#pragma once

#include <cstdint>

// Extra models attached to another model at runtime (hats, coats, accessories): RE note model_attachments.md (DX9). A
// plain uDevil4Model unit built by hand (its DTI's newInstance returns 0), given its .mod with uModel::setModel, with a
// copy of its vtable whose move does nothing and whose moveAfter (the serial pass after every unit's move and moveEx,
// where the players have finished their matrices) keeps it attached and then runs the model's own update, so it never
// lags its owner. Two modes:
//   - RIGID: the model follows one joint of the owner (uCoord parent = the owner, parent number = that joint NUMBER), with
//     an offset in that joint's frame: what weapons do;
//   - SKIN: every joint of the model whose number the owner also has copies the owner joint's world matrix each frame,
//     through a cCnsMatrix per joint (what Nero's face, hair and coat do, initModel 0x7E3760). Joints the owner lacks stay
//     plain FK from their parent, so chains can drive them (coat tails); a joint a chain holds is left to the chain. The
//     shared joints need the owner's bind pose, or the mesh comes out offset.
// The model follows the owner's visibility (the owner hidden, or a player's mbActorTrans off: cutscenes) and dies with it
// (an owner that isn't moving any more). Default line 29: after the players (13) and enemies (15+); a room change kills
// it, so attachments are spawned again per room.
namespace model_attach {

enum Mode : int { RIGID = 0, SKIN = 1 };
constexpr int kDefaultLine = 29;

struct Offset {                      // RIGID: the model's place in the owner joint's frame (cm, quaternion x y z w)
    float pos[3]   = { 0.0f, 0.0f, 0.0f };
    float quat[4]  = { 0.0f, 0.0f, 0.0f, 1.0f };
    float scale[3] = { 1.0f, 1.0f, 1.0f };
};

// The attached model (a uDevil4Model unit), 0 when the .mod doesn't load. `mod_path`: game path without extension.
// `joint` (RIGID): the owner's joint number (-1 = its root).
uintptr_t spawn(uintptr_t owner, const char* mod_path, Mode mode, int joint = -1, const Offset* offset = nullptr,
                int line = kDefaultLine);

// Lets go of the owner (its constraints off the model's joints) and kills the unit (the game frees it two passes later).
void despawn(uintptr_t model);

bool      is_attachment(uintptr_t model);   // one of ours (a tag in the block after the model)
uintptr_t owner_of(uintptr_t model);        // 0 once let go

} // namespace model_attach
