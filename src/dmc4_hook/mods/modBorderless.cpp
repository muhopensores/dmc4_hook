#include "modBorderless.hpp"
#include "../utils/String.hpp"
#include "../hooks.h"
#if 1

using namespace utils;

BOOL CALLBACK onMonitor(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM data) {
	UNREFERENCED_PARAMETER(hdc);
	UNREFERENCED_PARAMETER(rect);

	auto me = (Borderless*)data;

	MONITORINFOEX monitorInfo{};

	monitorInfo.cbSize = sizeof(MONITORINFOEX);

	if (GetMonitorInfo(monitor, &monitorInfo) == FALSE) {
		return TRUE;
	}

	Borderless::Monitor bwMonitor{};

	bwMonitor.name = monitorInfo.szDevice;
	bwMonitor.rect = monitorInfo.rcMonitor;
	bwMonitor.workRect = monitorInfo.rcWork;

	me->getMonitors().push_back(bwMonitor);

	return TRUE;
}

void Borderless::apply() {
	HL_LOG_RAW("[BorderlessWindow] Applying Borderless Window settings...\n");
	auto wnd = getMainWindow();
	if (!wnd) {
		HL_LOG_RAW("[BorderlessWindow] ERROR: DMC4 window was not available!\n");
	}
	if (m_defaultStyle == 0) {
		m_defaultStyle = GetWindowLong(wnd, GWL_STYLE);
	}
	m_style = m_defaultStyle;
	// If we were unable to enumerate any monitors then just fail I guess.
	if (m_monitors.empty()) {
		HL_LOG_RAW("[BorderlessWindow] ERROR: No monitors available!\n");
		return;
	}
	int m_monitorChoise = 0;
	const auto& r = m_monitors[m_monitorChoise].rect;

	m_x = r.left;
	m_y = r.top;
	m_w = r.right - r.left;
	m_h = r.bottom - r.top;
	m_style = WS_OVERLAPPED | WS_MINIMIZEBOX | WS_CLIPSIBLINGS | WS_VISIBLE;
	//m_style &= ~(WS_BORDER | WS_CAPTION | WS_THICKFRAME | WS_OVERLAPPED | WS_CLIPSIBLINGS | WS_VISIBLE);

	m_changeStyle = true;
	m_changePos = true;

	if (m_changeStyle) {
		HL_LOG_RAW("[BorderlessWindow] Trying to set style: %ld\n", m_style);

		const auto isStyleSet = SetWindowLong(wnd, GWL_STYLE, m_style) == m_style;
		const auto isStylePosSet = SetWindowPos(wnd, HWND_TOP, m_x, m_y, m_w, m_h, SWP_FRAMECHANGED) != FALSE;

		if (!isStyleSet || !isStylePosSet) {
			return;
		}

		HL_LOG_RAW("[BorderlessWindow] Style set!\n");

		m_changeStyle = false;
	}
	if (m_changePos) { 
		HL_LOG_RAW("[BorderlessWindow] Trying to move to x:%d y:%d w:%d h:%d\n", m_x, m_y, m_w, m_h);

		const auto wasWindowMoved = MoveWindow(wnd, m_x, m_y, m_w, m_h, TRUE) != FALSE;

		if (!wasWindowMoved) {
			return;
		}

		HL_LOG_RAW("[BorderlessWindow] Window moved!\n");

		m_changePos = false;
	}
	m_isChoiceFulfilled = true;
	HL_LOG_RAW("[BorderlessWindow] Choices applied.\n");

}

bool Borderless::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_WINDOWPOSCHANGED:
	case WM_SIZE:
	case WM_MOVE:
		if (m_isChoiceFulfilled) {
			m_changeStyle = true;
			m_changePos = true;
			m_isChoiceFulfilled = false;
		}
	default:
		return true;
	}
}

void Borderless::onFrame(fmilliseconds& dt) {
	if (m_enabled && !m_isChoiceFulfilled) {
		apply();
	}
}

void Borderless::onConfigLoad(const utils::Config & cfg) {
	m_enabled = cfg.get<bool>("borderless_window").value_or(false);
}

void Borderless::onConfigSave(utils::Config & cfg) {
	cfg.set<bool>("borderless_window", m_enabled);
}

std::optional<std::string> Borderless::onInitialize() {
	EnumDisplayMonitors(nullptr, nullptr, onMonitor, (LPARAM)this);
	return Mod::onInitialize();
}

void Borderless::onGUIframe() {
	if (ImGui::Checkbox("Borderless Window", &m_enabled)) {
		apply();
	}
}
#endif