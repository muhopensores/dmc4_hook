#include "hooks.h"

bool        g_drawGUI   = false;
const char* windowName  = "DEVIL MAY CRY 4";
WNDPROC     oWndProc    = NULL;
bool        resetCalled = false;

hl::Hooker  d3d_hook;

static void resetCallDetour(hl::CpuContext* ctx) {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	resetCalled = true;
}
static void presentCallDetour(hl::CpuContext* ctx) {
	IDirect3DDevice9* device = (IDirect3DDevice9*)ctx->EAX;
	if (resetCalled) {
		ImGui_ImplDX9_CreateDeviceObjects();
		resetCalled = false;
	}
	if (g_drawGUI) {
		RenderImgui(device);
	}
}
void hookD3D9(uintptr_t modBase) {
	// present call
	// DevilMayCry4_DX9.exe+4F33EC - 6A 00                 - push 00 { 0 }
	uintptr_t presentCall = modBase + 0x4F33EC;
	d3d_hook.hookDetour(presentCall, 6, &presentCallDetour);
	// reset call
	// DevilMayCry4_DX9.exe+4F138A - FF D0                 - call eax
	uintptr_t resetCall1 = modBase + 0x4F138A;
	d3d_hook.hookDetour(resetCall1, 7, &resetCallDetour);
	// another reset call
	// DevilMayCry4_DX9.exe+4F135D - FF D0                 - call eax
	uintptr_t resetCall2 = modBase + 0x4F135D;
	d3d_hook.hookDetour(resetCall2, 7, &resetCallDetour);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return true;

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}