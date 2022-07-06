#pragma once

#include <spdlog/spdlog.h>

class Mods;
namespace utility {
    class  Input;
    struct Hotkey;
}

#include "D3D9Hook.hpp"
#include "WindowsMessageHook.hpp"
#include "utility/ChronoAlias.hpp"
#include "imgui.h"

// Global facilitator
class ModFramework {
public:
    ModFramework();
    virtual ~ModFramework();

    bool is_valid() const {
        return m_valid;
    }

    const auto& get_mods() const {
        return m_mods;
    }

	const auto& get_d3d9_device() const {
		return m_d3d9_hook->get_device();
	}

    Address get_module() const {
        return GetModuleHandle(0);
        //return m_game_module;
    }

    bool is_ready() const {
        return m_game_data_initialized;
    }

	const bool get_window_focus() const {
		return m_window_focused;
	}

    const auto get_window_handle() const {
        return m_wnd;
    }

    const auto& get_input_struct() const {
        return m_input;
    }

    const auto& get_menu_key_struct() const {
        return m_menu_key;
    }

    void on_frame();
    void on_reset();
    bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);

private:
    void draw_ui();

    bool initialize();
    void create_render_target();
    void cleanup_render_target();

    bool m_first_frame{ true };
    bool m_valid{ false };
    bool m_initialized{ false };
    bool m_draw_ui{ false };
	bool m_draw_cursor{ true };
	bool m_window_focused{ true };
    std::atomic<bool> m_game_data_initialized{ false };
    
    HWND m_wnd{ 0 };
    HMODULE m_game_module{ 0 };

    std::chrono::high_resolution_clock::time_point m_prev_time;

    std::unique_ptr<D3D9Hook> m_d3d9_hook{};
    std::unique_ptr<WindowsMessageHook> m_windows_message_hook;
    std::shared_ptr<spdlog::logger> m_logger;
    std::string m_error{ "" };

    std::unique_ptr<utility::Input> m_input;
    std::unique_ptr<utility::Hotkey> m_menu_key;

    // Game-specific stuff
    std::unique_ptr<Mods> m_mods;
};

extern std::unique_ptr<ModFramework> g_framework;