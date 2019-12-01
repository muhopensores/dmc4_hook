#pragma once
#include <d3d9.h>
#include <d3dx9.h>

#include "imgui_dmc4.h"
#include "hacklib/Hooker.h"


extern bool g_drawGUI;

// def winproc
extern const char* windowName;
extern WNDPROC oWndProc;

void hookD3D9(uintptr_t modBase);

// winproc
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
