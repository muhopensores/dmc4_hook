#pragma once

#include "../mod.hpp"

class FastStart : public Mod
{
public:
	FastStart() = default;

	std::string getModName() override { return "FastStart"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

private:
	hl::Hooker hook_dti;
	hl::Hooker hook_uFade;
};