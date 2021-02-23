#pragma once

#include "../mod.hpp"

class FastStart : public Mod
{
public:
	FastStart() = default;

	static bool modEnabled;

	void toggle(bool enable);

	std::string getModName() override { return "FastStart"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

private:
	hl::Hooker hook_dti;
	hl::Patch  patch_menu;
	hl::Patch  patch_mt01; // menu transitions
	hl::Patch  patch_mt02;
};