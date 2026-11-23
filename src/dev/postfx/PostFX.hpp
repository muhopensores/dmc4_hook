#pragma once

#include "../../mod.hpp"
#include <cstdint>
#include <string>

// PostFX: screen passes the DX9 game doesn't have (SE's engine has them as filters: Vibed\RE\filters_se.md). Plan
// Vibed\PostFX\README.md. Step 1, ambient occlusion: drawn with the device directly, from a hook at the head of
// sRender::dispatchCommands' tag loop (0x8F4460), before the first main-scene tag of pass 6 (TRANSPARENT) or later, i.e.
// after the opaque scene and before effects and transparent meshes. It reads the frame's depth texture (sRender +0x1700C,
// Vibed\RE\depth_texture.md), writes occlusion into a target of its own, blurs it and multiplies it onto the scene's
// render target. The device's state is put back with a state block (render target and depth surface by hand: a state
// block doesn't hold them), so the game's own state cache stays right.
namespace post_fx {

struct Settings {
    bool  ao = false;
    float ao_strength = 4.0f;     // how dark a fully occluded corner gets
    float ao_radius = 120.0f;     // cm: how far occluders count
    int   ao_view = 0;            // debug: 0 the picture, 1 the occlusion alone, 2 the depth (distance)
};
extern Settings settings;

struct Status {
    bool        hooked = false;
    bool        compiled = false;
    bool        from_file = false;
    std::string error;
    int         frames = 0;       // frames AO was drawn in (since start)
    int         width = 0, height = 0;
};
Status status();

} // namespace post_fx

class PostFX : public Mod {
public:
    PostFX() = default;
    std::string get_mod_name() override { return "PostFX"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
    void on_reset() override;

    std::unique_ptr<FunctionHook> hook_tag;
};
