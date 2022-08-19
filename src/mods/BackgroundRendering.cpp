#include "MinHook.h"

#include "BackgroundRendering.hpp"
// expose functions/variables you need to call from outside in mods.h

typedef HWND (WINAPI *GETFOCUS)(VOID);
typedef HWND (WINAPI *GETFOREGROUNDWINDOW)(VOID);

GETFOCUS            fp_get_focus            = NULL;
GETFOREGROUNDWINDOW fp_get_foreground_window = NULL;

HWND modGameWindow{ 0 };
bool mod_enabled{ false };

HWND WINAPI detour_get_foreground_window() {
	if (mod_enabled) {
		return modGameWindow;
	}
	
		return fp_get_foreground_window();
}

HWND WINAPI detour_get_focus() {
	if (mod_enabled) {
		return modGameWindow;
	}
	
		return fp_get_focus();
}



std::optional<std::string> BackgroundRendering::on_initialize() {
    modGameWindow = g_framework->get_window_handle();//FindWindowA(NULL, "DEVIL MAY CRY 4");
	if (!modGameWindow) {
		throw std::runtime_error("[BackgroundRendering] WINDOW NOT FOUND\n");
	}

	if (MH_CreateHookApi(L"user32", "GetForegroundWindow", &detour_get_foreground_window, (LPVOID*)&fp_get_foreground_window) == MH_OK) {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) returned MH_OK\n");
	}
	else {
		spdlog::error("[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetForegroundWindow) failed";
	}
	if (MH_EnableHook(&GetForegroundWindow) == MH_OK) {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetForegroundWindow) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetForegroundWindow) failed";
	}

	if (MH_CreateHookApi(L"user32", "GetFocus", &detour_get_focus, (LPVOID*)&fp_get_focus) == MH_OK) {
		spdlog::info("[BackgroundRendering]: CreateHookApi(user32, GetFocus) returned MH_OK\n");
	}
	else {
		spdlog::error("[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed\n");
		return "[BackgroundRendering]: CreateHookApi(user32, GetFocus) failed";
	}
	if (MH_EnableHook(&GetFocus) == MH_OK) {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetFocus) returned MH_OK\n");
	}
	else {
		spdlog::info("[BackgroundRendering]: EnableHook(&GetFocus) failed\n");
		return "[BackgroundRendering]: EnableHook(&GetFocus) failed";
	}
	return Mod::on_initialize();
}

void BackgroundRendering::on_config_load(const utility::Config& cfg) {
	mod_enabled = cfg.get<bool>("enable_focus_patch").value_or(false);
}

void BackgroundRendering::on_config_save(utility::Config& cfg) {
	cfg.set<bool>("enable_focus_patch", mod_enabled);
}

void BackgroundRendering::on_gui_frame() {
	ImGui::Checkbox("Background Input", &mod_enabled);
}

namespace background_rendering {
    LRESULT WINAPI on_message_wnd_proc_return(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (!mod_enabled) { return 0xDEADBEEF; }
        
            switch (message)
            {
            case WM_KILLFOCUS:
            {
                if (modGameWindow == wnd) {
                    return DefWindowProc(wnd, message, wParam, lParam);
                }
                break;
            }
            case WM_MOUSEACTIVATE:
            {
                if ((HWND)wParam == wnd) {
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
                return DefWindowProc(wnd, message, wParam, lParam);
            }
            default:
                return 0xDEADBEEF;
            }
       
        return DefWindowProc(wnd, message, wParam, lParam);
    }
}
