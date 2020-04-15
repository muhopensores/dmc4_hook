#include "hooks.h"
#include "main.h"
#include "mods/modWorkRate.hpp"
bool*       g_enableBackgroundInput = false;
bool        g_drawGUI   = false;
const char* windowName  = "DEVIL MAY CRY 4";
WNDPROC     oWndProc    = NULL;
bool        resetCalled = false;
HWND        hWindow = NULL;

hl::Hooker  d3d_hook;
int32_t width;
int32_t height;


bool IsCursorVisibleWINAPI() {
	CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
	if (!GetCursorInfo(&info))
	{
		throw std::exception("GetCursorInfo");
	}

	return (info.flags & CURSOR_SHOWING) != 0;
}

void hookSetEnableBackgroundInput(bool* address) {
	g_enableBackgroundInput = address;
}

static void resetCallDetour(hl::CpuContext* ctx) {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	resetCalled = true;
}
static void presentCallDetour(hl::CpuContext* ctx) {
	IDirect3DDevice9* device = (IDirect3DDevice9*)ctx->EAX;
	auto d3dObj = *(D3D9obj*)ctx->ESI;

	width = d3dObj.resX_00;
	height = d3dObj.resY_00;

	if (resetCalled) 
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		resetCalled = false;
	}
	/*auto main = GetMain();
	if (main->m_modsInitialized) {
		main->m_workRate->onFrame();
	}*/
	if (g_drawGUI) 
	{
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

void ToggleBorderless(bool enable)
{
	if (!hWindow) 
	{
		return;
	}
	static LONG init = GetWindowLongA(hWindow, GWL_STYLE);
	
	LONG style = init;
	if (enable) 
	{
		style = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_VISIBLE;
	}
	SetWindowLongA(hWindow, GWL_STYLE, style);

	POINT point = {};
	RECT rect = {};
	if (!GetWindowRect(hWindow, &rect)) 
	{
		return;
	}

	point.x = rect.left;
	point.y = rect.top;

	rect.right = width;
	rect.bottom = height;
	
	AdjustWindowRect(&rect, style, 0);
	MoveWindow(hWindow, point.x, point.y, (rect.right - rect.left), (rect.bottom - rect.top), 0);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (*g_enableBackgroundInput)
	{
		switch (uMsg)
		{
			case WM_KILLFOCUS:
			{
				if (hWnd == hWindow) {
					return DefWindowProc(hWnd, uMsg, wParam, lParam);
				}
				break;
			}
			case WM_MOUSEACTIVATE:
			{
				if ((HWND)wParam == hWindow) {
					return MA_ACTIVATEANDEAT;
				}
				else {
					return MA_ACTIVATE;
				}
			    break;
			}
			case WM_ACTIVATEAPP:
			case WM_ACTIVATE:
			case WM_NCACTIVATE:
			{
				return 1;
				break;
			}
			case WM_SHOWWINDOW:
			{
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			}

		}

	}
    if (true && ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
        return DefWindowProc(hWnd, uMsg, wParam, lParam);

    return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}