#pragma once

#include "../mod.hpp"

// ViewportWindow: shows one of sCamera's viewports in a second OS window (2026-10-26). The viewport renders into a
// render-target texture instead of a region of the screen, and every frame the texture is copied to a swap chain of
// our own on that window. Whatever camera Coop / DebugCam put on the viewport is shown (viewport 0's when it has none).
// How (DX9 IDB):
//   - sRender::buildDisplayList 0x8F6480 draws a viewport that is mActive and has mAttr bit 0 (on screen) or an
//     mpRenderTarget (+0x9C); without bit 0, Viewport::setup 0x8E6830 renders into the target's surface;
//   - the setter sub_8E5E50 (thiscall, ecx = viewport, arg = rRenderTargetTexture) add-refs it and makes a matching
//     "SceneZBuffer" depth target (+0xA0); a null argument releases both;
//   - the target is an rRenderTargetTexture (DTI 0xEAE268, uMirror's mpRTTex too) loaded from a 40-byte .rtex header
//     the mod writes under nativePC; +0xCC is its cTransTexture, whose +0x10 is the IDirect3DTexture9;
//   - units draw in a viewport only when their mTransView (unit +7) has its bit; the default is 15, so viewports 1-3;
//   - sCamera::reset 0x8EBD10 clears every viewport's mpCamera on a stage load, and Coop / DebugCam rewrite mAttr, so
//     the viewport is set up again every frame while the mod is on.
// All state lives in the .cpp, so the class layout never changes (Mods.cpp / GuiFunctions.cpp construct and draw it).

namespace vpwin {

extern bool mod_enabled;
extern int  viewport_index; // 1-3: the viewport shown in the window
extern int  width, height;  // the render target's size (the window scales it)
extern bool full_main;      // keep viewport 0 full screen while the other one is in the window (Coop lays them out)

} // namespace vpwin

class ViewportWindow : public Mod {
public:
    ViewportWindow() = default;
    std::string get_mod_name() override { return "ViewportWindow"; };

    std::optional<std::string> on_initialize() override;
    void on_frame(fmilliseconds& dt) override;
    void on_reset() override;
    void after_reset() override;
    void on_gui_frame(int display) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;
};
