#pragma once

#include "../mod.hpp"

class HeightRestrictionNero : public Mod {
public:

	HeightRestrictionNero() = default;

	static bool modEnabled;

	void toggle(bool enable);

	std::string getModName() override { return "HeightRestrictionNero"; };
	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;

private:
	hl::Patch patchBuster;
	hl::Patch patchSplit;
	hl::Patch patchCalibur;
	hl::Patch patchExCalibur;
	hl::Patch patchSnatch;
	hl::Patch patchRave;
	hl::Patch patchDoubleDown;
	hl::Patch patchRev;

};