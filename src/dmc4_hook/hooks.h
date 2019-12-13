#pragma once
#include <d3d9.h>
#include <d3dx9.h>

#include "imgui_dmc4.h"
#include "hacklib/Hooker.h"

#include <Windows.h>

class D3D9obj
{
public:
	virtual void Function0() {}; //
	virtual void Function1() {}; //

	char pad_0x0004[0x40]; //0x0004
	__int32 resX_00; //0x0044 
	__int32 resY_00; //0x0048 
	char pad_0x004C[0x4]; //0x004C
	__int32 resX_01; //0x0050 
	__int32 resY_01; //0x0054 
	char pad_0x0058[0x7E8]; //0x0058

}; //Size=0x0840

extern bool g_drawGUI;

// def winproc
extern const char* windowName;
extern WNDPROC oWndProc;

void hookD3D9(uintptr_t modBase);

// winproc
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ToggleBorderless(bool enable);