#pragma once

#include "mod.hpp"
//#include <unordered_map>

class Mods {
public:
	Mods();
	virtual ~Mods() {};

	std::optional<std::string> onInitialize() const;
	std::optional<std::string> onSlowInitialize() const;

	void onFrame(fmilliseconds& dt);
	void onDrawUI(uint32_t hash);
	void onDrawSlowUI(uint32_t hash);
// @TODO: leave those like that until complete rewrite
#if 1
	void onConfigSave();
#else
	void onConfigLoad(utils::Config& cfg);
	void onConfigSave(utils::Config& cfg);
#endif
    void onUpdateInput(hl::Input& input);
	void onGamePause(bool toggle);
	bool onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam);
	void onChatCommand(const std::string& message);
	const auto& getMods() const {
		return m_mods;
	}

private:
	std::unordered_map<std::uint32_t, std::shared_ptr<Mod>> m_mods;
	std::unordered_map<std::uint32_t, std::shared_ptr<Mod>> m_slowMods;
	//std::vector<std::shared_ptr<Mod>> m_mods;
};