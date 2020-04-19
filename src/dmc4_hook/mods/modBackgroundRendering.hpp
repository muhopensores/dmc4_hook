#pragma once

#include "../mod.hpp"

class BackgroundRendering : public Mod {
public:
	BackgroundRendering() = default;

	std::string getModName() override { return "BackgroundRendering"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

	static bool* getModEnabledPtr();
private:
};