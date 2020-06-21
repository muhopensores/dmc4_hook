#pragma once

#include "../mod.hpp"

class InfAllHealth : public Mod {
public:

	InfAllHealth() = default;

	static bool modEnabled;

	void toggle(bool enable);

	std::string getModName() override { return "InfAllHealth"; };
	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;

private:
	hl::Patch patchAllHealth;

};