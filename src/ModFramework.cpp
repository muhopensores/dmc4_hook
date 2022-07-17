
// TODO(low priority): merge spdlog sinks or something fun
#ifndef NDEBUG
#include <spdlog/sinks/stdout_color_sinks.h>
#else
#include <spdlog/sinks/basic_file_sink.h>
#endif
#include <imgui/imgui.h>

#include <Windows.h>

// ours with XInput removed
#include "fw-imgui/imgui_impl_win32.h"
#include "fw-imgui/imgui_impl_dx9.h"

#include "utility/Module.hpp"

#include "Mods.hpp"

#include "LicenseStrings.hpp"
#include "ModFramework.hpp"

#include "Config.hpp"

#include "utility/Thread.hpp"
#include "GuiFunctions.hpp"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

std::unique_ptr<ModFramework> g_framework{};

ModFramework::ModFramework()
    : m_game_module{ GetModuleHandle(0) },
#ifndef NDEBUG
    m_logger{ spdlog::stdout_color_mt("ModFramework") }
#else
    m_logger{ spdlog::basic_logger_mt("ModFramework", LOG_FILENAME, true) }
#endif
{
    spdlog::set_default_logger(m_logger);
    spdlog::flush_on(spdlog::level::info);
    spdlog::info(LOG_ENTRY);

#ifndef NDEBUG
    spdlog::set_level(spdlog::level::debug);
#endif

    // SteamStub shit
    // wait until steam drm unpacks itself
    uintptr_t code_ptr = 0x00B84120;
    int data = *(int*)(code_ptr);
    while (data != 0xF6A005C7) {
        data = *(int*)(code_ptr);
        //Sleep(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
    }

    std::queue<DWORD> tr = utility::suspend_all_other_threads();

    m_mods = std::make_unique<Mods>();
    
    auto e = m_mods->on_initialize(Mod::ModType::REGULAR);

    utility::resume_threads(tr);

    if (e) {
        if (e->empty()) {
            m_error = "An unknown error has occurred during slow mods initialization.";
            spdlog::error(m_error);
        }
        else {
            m_error = *e;
            spdlog::error(m_error);
        }
    }

    m_d3d9_hook = std::make_unique<D3D9Hook>();

	m_d3d9_hook->on_reset    ([this](D3D9Hook& hook) { on_reset(); });
	m_d3d9_hook->on_present  ([this](D3D9Hook& hook) { on_frame(); });

    m_valid = m_d3d9_hook->hook();

    if (m_valid) {
        spdlog::info("Hooked D3D9");
    }
}

ModFramework::~ModFramework() {
    // TODO(): clean dll unload
}

void set_visible_cursor_winapi(bool visible) {

	CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
	if (!GetCursorInfo(&info))
	{
        return;
	}

	bool isvisible = (info.flags & CURSOR_SHOWING) != 0;
	if (isvisible != visible)
	{
		ShowCursor(visible);
	}
}

static bool is_cursor_visible_winapi() {
	CURSORINFO info = { sizeof(CURSORINFO), 0, nullptr, {} };
	if (!GetCursorInfo(&info))
	{
        return false;
	}

	return (info.flags & CURSOR_SHOWING) != 0;
}

void ModFramework::on_frame() {
    //spdlog::debug("on_frame");

    std::chrono::high_resolution_clock::time_point now_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> delta = now_time - m_prev_time;

    if (!m_initialized) {
        if (!initialize()) {
            spdlog::error("Failed to initialize ModFramework");
            return;
        }

        spdlog::info("ModFramework initialized");
        m_initialized = true;
        return;
    }

    m_input->update();
    m_mods->on_update_input(*m_input);
    if (m_input->went_down(VK_DELETE) || m_menu_key->check(*m_input)) {
        m_draw_ui = !m_draw_ui;
        m_mods->on_game_pause(m_draw_ui);
        set_visible_cursor_winapi(m_draw_ui);
    }

	ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (m_error.empty() && m_game_data_initialized) {
        m_mods->on_frame(delta);
        m_mods->on_draw_custom_imgui_window();
    }

    draw_ui();

    ImGui::EndFrame();
    ImGui::Render();

	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
    m_prev_time = now_time;
}

void ModFramework::on_reset() {
    spdlog::info("Reset!");
	if (!m_initialized) { return; }
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui_ImplDX9_InvalidateDeviceObjects();
    // Crashes if we don't release it at this point.
    //cleanup_render_target();
    m_initialized = false;
}

bool ModFramework::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    
    if (!m_initialized) {
        return true;
    }

    if (!m_mods->on_message(wnd, message, w_param, l_param)) { return false; }

    if (m_draw_ui && ImGui_ImplWin32_WndProcHandler(wnd, message, w_param, l_param) != 0) {
        // If the user is interacting with the UI we block the message from going to the game.
        auto& io = ImGui::GetIO();

        if (io.WantCaptureMouse || io.WantCaptureKeyboard || io.WantTextInput) {
            return false;
        }
    }

    return true;
}

void ModFramework::draw_ui() {
    //std::lock_guard _{ m_input_mutex };
    if (!m_draw_ui) {
        //m_dinput_hook->acknowledge_input();
        //ImGui::GetIO().MouseDrawCursor = false;
        return;
    }
    if (m_game_data_initialized) {
        gui::set_window_props();
        gui::im_gui_main_window_proc(m_mods.get());
    }
    else if (!m_game_data_initialized) {
        ImGui::Begin("initializing");
        ImGui::TextWrapped("ModFramework is currently initializing...");
        ImGui::End();
    }
    else if(!m_error.empty()) {
        char buffer[MAX_PATH];
        sprintf_s(buffer, sizeof(buffer), "ModFramework error: %s", m_error.c_str());
        MessageBoxA(m_wnd, buffer, "DMC4 mod error", MB_ICONERROR);
        std::exit(ERROR_ARENA_TRASHED); // funne errno name
    }
#if 0
    auto& io = ImGui::GetIO();


    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_::ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(410, 500), ImGuiCond_::ImGuiCond_Once);

    ImGui::Begin("ModFramework", &m_draw_ui);

#ifdef GIT_HASH
	ImGui::Text("Version: %s", GIT_HASH);
	ImGui::Text("Date: %s", GIT_DATE);
#endif
    ImGui::Text("Menu Key: Insert");
	
	if (ImGui::Button("Save config")) {
		save_config();
	}
    draw_about();

    if (m_error.empty() && m_game_data_initialized) {
        m_mods->on_draw_ui();
    }
    else if (!m_game_data_initialized) {
        ImGui::TextWrapped("ModFramework is currently initializing...");
    }
    else if(!m_error.empty()) {
        ImGui::TextWrapped("ModFramework error: %s", m_error.c_str());
    }

    ImGui::End();
#endif
}

bool ModFramework::initialize() {
    if (m_initialized) {
        return true;
    }

    spdlog::info("Attempting to initialize");

    auto device = m_d3d9_hook->get_device();

    // Wait.
    if (device == nullptr) {
        spdlog::info("Device is null.");
        return false;
    }

	D3DDEVICE_CREATION_PARAMETERS dev_params{ 0 };
	auto hr = device->GetCreationParameters(&dev_params);
	if (SUCCEEDED(hr)) {
		if (dev_params.hFocusWindow) {
			spdlog::info("[D3D Device init] D3DDEVICE_CREATION_PARAMETERS hFocusWindow={0:x}\n", (void*)dev_params.hFocusWindow);
			m_wnd = dev_params.hFocusWindow;
		}
		else {
			spdlog::info("[D3D Device present] D3DDEVICE_CREATION_PARAMETERS hFocusWindow= is NULL\n");
			return false;
		}
	}

    // Explicitly call destructor first
    m_windows_message_hook.reset();
    m_windows_message_hook = std::make_unique<WindowsMessageHook>(m_wnd);
    m_windows_message_hook->on_message = [this](auto wnd, auto msg, auto w_param, auto l_param) {
        return on_message(wnd, msg, w_param, l_param);
    };

    m_menu_key = std::make_unique<utility::Hotkey>(VK_DELETE, "Menu Key", "menu_key");

    spdlog::info("Window Handle: 0x{0:x}", (uintptr_t)m_wnd);
    spdlog::info("Initializing ImGui");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    spdlog::info("Initializing ImGui Win32");

    if (!ImGui_ImplWin32_Init(m_wnd)) {
        spdlog::error("Failed to initialize ImGui.");
        return false;
    }

    spdlog::info("Initializing ImGui D3D9");

    if (!ImGui_ImplDX9_Init(device)) {
        spdlog::error("Failed to initialize ImGui DX9.");
        return false;
    }

    spdlog::info("Initializing Input system");
    m_input = std::make_unique<utility::Input>();

    gui::dark_theme();

    if (m_first_frame) {
        m_first_frame = false;

        spdlog::info("Starting game data initialization thread");

        // Game specific initialization stuff
        std::thread init_thread([this]() {

            auto e = m_mods->on_initialize(Mod::ModType::SLOW);

            if (e) {
                if (e->empty()) {
                    m_error = "An unknown error has occurred during slow mods initialization.";
                    spdlog::error(m_error);
                }
                else {
                    m_error = *e;
                    spdlog::error(m_error);
                }
            }

            m_game_data_initialized = true;
        });

        init_thread.detach();
    }

    return true;
}

void ModFramework::create_render_target() {
    /*cleanup_render_target();

    ID3D11Texture2D* back_buffer{ nullptr };
    if (m_d3d11_hook->get_swap_chain()->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&back_buffer) == S_OK) {
        m_d3d11_hook->get_device()->CreateRenderTargetView(back_buffer, NULL, &m_main_render_target_view);
        back_buffer->Release();
    }*/
}

void ModFramework::cleanup_render_target() {
    /*if (m_main_render_target_view != nullptr) {
        m_main_render_target_view->Release();
        m_main_render_target_view = nullptr;
    }*/
}

