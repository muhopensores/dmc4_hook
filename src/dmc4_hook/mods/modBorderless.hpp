#pragma once

#include <Windows.h>

#include "../mod.hpp"

class Borderless : public Mod {
public:
	struct Monitor {
		std::string name;
		RECT rect;
		RECT workRect;
	};

	Borderless() = default;

	std::string getModName() override { return "Borderless"; };

	std::optional<std::string> onInitialize() override;

	void onFrame() override;
	void onGUIframe() override;
	bool onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	auto& getMonitors() {
		return m_monitors;
	}

private:
	std::vector<Monitor> m_monitors;
	LONG m_defaultStyle;
	LONG m_style;
	int m_x, m_y, m_w, m_h;
	bool m_changeStyle;
	bool m_changePos;

	int m_styleChoice;
	bool m_isChoiceFulfilled;
	bool m_enabled;
	void apply();
	void undo();
};