#pragma once

#include <spdlog/spdlog.h>

class Mods;
class ImGuiConsole;
namespace utility {
    class  Input;
    struct Hotkey;
}

#include "D3D9Hook.hpp"
#include "XInputHook.hpp"
#include "WindowsMessageHook.hpp"
#include "utility/ChronoAlias.hpp"
#include "imgui.h"


// Global facilitator
class ModFramework {
public:
    explicit ModFramework();
    ModFramework(const ModFramework&)            = delete;
    ModFramework(ModFramework&&)                 = delete;
    ModFramework& operator=(const ModFramework&) = delete;
    ModFramework& operator=(ModFramework&&)      = delete;

    virtual ~ModFramework();

    bool is_valid() const {
        return m_valid;
    }

    const auto& get_mods() const {
        return m_mods;
    }

	auto get_d3d9_device() const {
		return m_d3d9_hook->get_device();
	}

    static Address get_module()  {
        return GetModuleHandle(0);
        //return m_game_module;
    }

    static int sys_ms();

    bool is_ready() const {
        return m_game_data_initialized;
    }

    auto get_window_handle() const {
        return m_wnd;
    }

    const auto& get_input_struct() const {
        return m_input;
    }

    const auto& get_menu_key_struct() const {
        return m_menu_key;
    }

    const auto& get_menu_button_struct() const {
        return m_menu_xinput_buttons;
    }

    // const auto& get_custom_imgui_font() const {
    //     return m_custom_font;
    // }
    bool m_draw_console = false;
    bool m_draw_ui = false;

    // NOTE(): locale stuff
    bool m_imfont_queue_reload_flag {false};
    ImFont* m_imfont_main {};
    char m_glob_locale[3] = "en"; // two chars and null terminator

    void on_frame();
    void on_reset();
    void on_after_reset();
    // NOTE(): need to delay updating font atlas till after 
    // frame is done through a flag at the end of on_frame();
    void on_locale_update(const char* contry_code);
    bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);

private:
    void draw_ui();

    bool initialize();
    void create_render_target();
    void cleanup_render_target();

    bool m_first_frame = true;
    bool m_valid = false;
    bool m_initialized = false;
    
    std::atomic<bool> m_game_data_initialized = false;
    
    HWND m_wnd{ nullptr };
    HMODULE m_game_module{ nullptr };

    std::chrono::high_resolution_clock::time_point m_prev_time;

    std::unique_ptr<D3D9Hook> m_d3d9_hook{};
    std::unique_ptr<XInputHook> m_xinput_hook{};
    std::unique_ptr<WindowsMessageHook> m_windows_message_hook;
    std::shared_ptr<spdlog::logger> m_logger;
    std::string m_error;

    std::unique_ptr<utility::Input>  m_input;
    std::unique_ptr<utility::Hotkey> m_menu_key;
    std::unique_ptr<utility::Hotkey> m_menu_xinput_buttons;

    // Game-specific stuff
    std::unique_ptr<Mods> m_mods;
    // ImFont* m_custom_font{};
};

extern std::unique_ptr<ModFramework> g_framework;
