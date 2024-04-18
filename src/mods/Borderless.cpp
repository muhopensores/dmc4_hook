#include "Borderless.hpp"
#include "utility/String.hpp"

#if 1

using namespace utility;

BOOL CALLBACK on_monitor(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data) {
    UNREFERENCED_PARAMETER(hdc);
    UNREFERENCED_PARAMETER(rect);

    auto me = (Borderless*)data;

    MONITORINFOEX monitor_info{};

    monitor_info.cbSize = sizeof(MONITORINFOEX);

    if (GetMonitorInfo(monitor, &monitor_info) == FALSE) {
        return TRUE;
    }

    Borderless::Monitor bw_monitor{};

    bw_monitor.name      = monitor_info.szDevice;
    bw_monitor.rect      = monitor_info.rcMonitor;
    bw_monitor.work_rect = monitor_info.rcWork;

    me->get_monitors().push_back(bw_monitor);

    return TRUE;
}

void Borderless::apply() {
    spdlog::info("[BorderlessWindow] Applying Borderless Window settings...\n");
    auto wnd = g_framework->get_window_handle();
    if (!wnd) {
        spdlog::info("[BorderlessWindow] ERROR: DMC4 window was not available!\n");
    }
    if (m_default_style == 0) {
        m_default_style = GetWindowLong(wnd, GWL_STYLE);
    }
    m_style = m_default_style;
    // If we were unable to enumerate any monitors then just fail I guess.
    if (m_monitors.empty()) {
        spdlog::info("[BorderlessWindow] ERROR: No monitors available!\n");
        return;
    }
    int m_monitor_choise = 0;
    const auto& r        = m_monitors[m_monitor_choise].rect;

    m_x     = r.left;
    m_y     = r.top;
    m_w     = r.right - r.left;
    m_h     = r.bottom - r.top;
    m_style = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_VISIBLE;
    // m_style &= ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPED | WS_CLIPSIBLINGS | WS_VISIBLE);

    m_change_style = true;
    m_change_pos   = true;

    spdlog::info("[BorderlessWindow] Trying to set style: {}\n", m_style);

    const auto is_style_set     = SetWindowLong(wnd, GWL_STYLE, m_style) == m_style;
    const auto is_style_pos_set = SetWindowPos(wnd, HWND_TOP, m_x, m_y, m_w, m_h, SWP_FRAMECHANGED) != FALSE;

    if (!is_style_set || !is_style_pos_set) {
        return;
    }

    spdlog::info("[BorderlessWindow] Style set!\n");

    m_change_style = false;

    if (m_change_pos) {
        spdlog::info("[BorderlessWindow] Trying to move to x:{} y:{} w:{} h:{}\n", m_x, m_y, m_w, m_h);

        const auto was_window_moved = MoveWindow(wnd, m_x, m_y, m_w, m_h, TRUE) != FALSE;

        if (!was_window_moved) {
            return;
        }

        spdlog::info("[BorderlessWindow] Window moved!\n");

        m_change_pos = false;
    }
    m_is_choice_fulfilled = true;
    spdlog::info("[BorderlessWindow] Choices applied.\n");
}

bool Borderless::on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) {
    switch (message) {
    case WM_WINDOWPOSCHANGED:
    case WM_SIZE:
    case WM_MOVE:
        if (m_is_choice_fulfilled) {
            m_change_style        = true;
            m_change_pos          = true;
            m_is_choice_fulfilled = false;
        }
    default:
        return true;
    }
}

void Borderless::on_frame(fmilliseconds& dt) {
    if (m_enabled && !m_is_choice_fulfilled) {
        apply();
    }
}

void Borderless::on_config_load(const utility::Config& cfg) {
    m_enabled = cfg.get<bool>("borderless_window").value_or(false);
    if (m_enabled) {
        apply();
    }
}

void Borderless::on_config_save(utility::Config& cfg) {
    cfg.set<bool>("borderless_window", m_enabled);
}

std::optional<std::string> Borderless::on_initialize() {
    EnumDisplayMonitors(nullptr, nullptr, on_monitor, (LPARAM)this);
    return Mod::on_initialize();
}

void Borderless::on_gui_frame() {
    if (ImGui::Checkbox("Borderless Window", &m_enabled)) {
        apply();
    }
    ImGui::SameLine();
    help_marker("Only applies if already in fullscreen. Hit Alt+Enter to toggle fullscreen");
}
#endif
