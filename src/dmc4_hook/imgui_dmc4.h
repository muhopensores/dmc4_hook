#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_internal.h"

// imgui draw function prototype
void RenderImgui(IDirect3DDevice9* pDevice, bool draw);
void RenderBackgroundWindow();


