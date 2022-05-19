#include "hooks.h"
#include "main.h"
#include "mods/modWorkRate.hpp"
#include "mods/modBackgroundRendering.hpp"
#include "Mods.hpp"

bool*       g_enableBackgroundInput = false;
bool        g_drawGUI   = false;
const char* windowName  = "DEVIL MAY CRY 4";
WNDPROC     oWndProc    = NULL;
bool        resetCalled = false;
HWND        hWindow = NULL;
static bool once = false;
hl::Hooker  d3d_hook;
static hl::Patch d3d_hook_fixup;
int32_t width;
int32_t height;

HWND getMainWindow() {
	if (!hWindow) {
		HL_LOG_ERR("hWindow is null!\n");
		//throw std::runtime_error("hWindow is null!");
	}
	return hWindow;
}

void setWindowHook(HWND window) {
	HL_LOG_RAW("Setting window hook, param = %x hWindow = %x\n",window, hWindow);
	hWindow = window;
	oWndProc = (WNDPROC)SetWindowLongPtr(window, GWL_WNDPROC, (LONG_PTR)WndProc);
}

bool IsCursorVisibleWINAPI() {
	CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
	if (!GetCursorInfo(&info))
	{
		throw std::exception("GetCursorInfo");
	}

	return (info.flags & CURSOR_SHOWING) != 0;
}

static void resetCallDetour(hl::CpuContext* ctx) {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	resetCalled = true;
}

std::chrono::high_resolution_clock::time_point prevTime;

static void presentCallDetour(hl::CpuContext* ctx) {
	IDirect3DDevice9* device = (IDirect3DDevice9*)ctx->EAX;
	auto d3dObj = *(D3D9obj*)ctx->ESI;

	width = d3dObj.resX_00;
	height = d3dObj.resY_00;

	if (!once) {
		D3DDEVICE_CREATION_PARAMETERS devParams{ 0 };
		auto hr = device->GetCreationParameters(&devParams);
		if (SUCCEEDED(hr)) {
			if (devParams.hFocusWindow) {
				HL_LOG_RAW("[D3D Device present] D3DDEVICE_CREATION_PARAMETERS hFocusWindow=%x\n", devParams.hFocusWindow);
				setWindowHook(devParams.hFocusWindow);
			}
			else {
				HL_LOG_ERR("[D3D Device present] D3DDEVICE_CREATION_PARAMETERS hFocusWindow= is NULL\n");
			}
		}
		else {
			HL_LOG_RAW("[D3D Device present] device->GetCreationParameters() FAILED\n");
			goto bail;
		}
		g_enableBackgroundInput = BackgroundRendering::getModEnabledPtr();
		once = true;
		prevTime = std::chrono::high_resolution_clock::now();
	}
bail:
	if (resetCalled) 
	{
		ImGui_ImplDX9_CreateDeviceObjects();
		resetCalled = false;
	}
	std::chrono::high_resolution_clock::time_point nowTime = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli> delta = nowTime - prevTime;
	GetMain()->getMods()->onFrame(delta);
	/*auto main = GetMain();
	if (main->m_modsInitialized) {
		main->m_workRate->onFrame();
	}*/

	RenderImgui(device,g_drawGUI);
	/*if (g_drawGUI) 
	{
		RenderImgui(device,g_drawGUI);
	}*/
	prevTime = nowTime;
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
    uintptr_t fixupLC = 0x08F1383;
    char fixupData[] = "\xEB\x0A";
    d3d_hook_fixup.apply(fixupLC, fixupData, 2);
	d3d_hook.hookDetour(resetCall2, 7, &resetCallDetour);
}

LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (*g_enableBackgroundInput)
	{
		if (!GetMain()->getMods()->onMessage(hWnd, uMsg, wParam, lParam)) {
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
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