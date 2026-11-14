#pragma once

#include <d3d9.h>

// Pixel shader bind listeners, for mods whose replacement shaders read registers of their own. Constants written once a
// frame don't reach the game's draws (PbrMaterials, 2026-11-14), so they're set where sRender::dispatchCommands binds a
// pixel shader: PbrMaterials hooks that (0x8F4E34, reached only when the shader changes) and calls these there, on
// sRender's thread. A listener checks whether `ps` is one of its own and sets its registers if so.
namespace ps_bind {

using Listener = void(__stdcall*)(IDirect3DDevice9* device, IDirect3DPixelShader9* ps);

constexpr int kMax = 4;
inline Listener listeners[kMax] = {};

inline void add(Listener fn) {
    for (Listener& l : listeners) {
        if (l == fn) return;
        if (!l) { l = fn; return; }
    }
}

inline void call(IDirect3DDevice9* device, IDirect3DPixelShader9* ps) {
    for (Listener l : listeners) {
        if (!l) return;
        l(device, ps);
    }
}

} // namespace ps_bind
