#pragma once

#include "../mod.hpp"

class InfAllHealth : public Mod {
public:

	InfAllHealth() = default;

	static bool modEnabled;
	static int hotkey;
	void toggle(bool enable);

	std::string getModName() override { return "InfAllHealth"; };
	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;
	//void onFrame(fmilliseconds& dt) override;
    void onUpdateInput(hl::Input& input) override;

private:
	hl::Patch patchAllHealth;

};