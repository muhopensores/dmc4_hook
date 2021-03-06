#pragma once

#include "../mod.hpp"

class WorkRate : public Mod {
public:
	WorkRate();

	static uintptr_t  jmp_return;
	static sWorkRate* sWorkRatePtr;

	std::string getModName() override { return "WorkRate"; };

	std::optional<std::string> onInitialize() override;

	void onFrame() override;
	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;
	void onGamePause(bool toggle) override;
private:
	hl::Hooker hook;
	float m_globalSpeed = 1.0f;
	bool checkWorkRatePtr();
};