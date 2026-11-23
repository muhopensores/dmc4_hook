#pragma once

#include <cstdint>
#include <string>

// Object IDs in the frame's depth texture (the tagged object picker, Vibed\PostFX\README.md "Object picker").
//   - uModel::draw (0x9F6CC0) is wrapped: a lit draw (render mode 1) records the address range of the command records
//     it emitted (cTrans +0x2AB0, the command allocator, grows downward) in one ring, with its unit's ID (1-255, kept
//     while the unit is drawn, recycled after it isn't for a while).
//   - At replay (PostFX's tag hook, render thread), each depth-pass tag's command is looked up in those ranges and c220.x
//     set to its model's ID / 255 when it changes (0 for anything else).
//   - tXfMaterialZPass's two pixel shaders are swapped for tag.inc's, which write the depth exactly as the stock ones and
//     c220.x into blue.
namespace tags {

extern bool enabled;   // the depth shaders in and the IDs set (Graphics tab; on by default)

struct Status {
    bool        hooked = false;
    bool        shaders = false;        // the tagged depth shaders are in
    int         last_frame_models = 0;  // IDs in use
    std::string error;
};
Status status();

void init();                                   // the uModel::draw hook; once
void update();                                 // every frame (Present thread): compile / install the depth shaders
void on_reset();

// render thread, PostFX's tag hook: for a depth-pass tag's command, sets c220 to its model's ID
void on_depth_tag(void* device, uintptr_t command);
void on_dispatch_start();                      // render thread, the first tag of a dispatch

// the unit an ID belongs to, and a unit's ID (0 = none); any thread
uintptr_t unit_of(uint8_t id);
uint8_t id_of(uintptr_t unit);

} // namespace tags
