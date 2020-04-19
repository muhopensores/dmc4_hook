#pragma once

#include "mod.hpp"
//#include <unordered_map>

class Mods {
public:
	Mods();
	virtual ~Mods() {};

	std::optional<std::string> onInitialize() const;

	void onFrame();
	void onDrawUI(uint32_t hash);
// @TODO: leave those like that until complete rewrite
#if 0
	void onConfigSave();
#else
	void onConfigLoad(utils::Config& cfg);
	void onConfigSave(utils::Config& cfg);
#endif
	void onGamePause(bool toggle);
	const auto& getMods() const {
		return m_mods;
	}

private:
	std::unordered_map<std::uint32_t, std::shared_ptr<Mod>> m_mods;
	//std::vector<std::shared_ptr<Mod>> m_mods;
};