#pragma once

#include "../mod.hpp"

class NoHbKnockback : public Mod {
public:

	NoHbKnockback();

	static bool      modEnabled;
	static uintptr_t _noHelmBreakerKnockbackContinue;// = 0x0051C389;
	static uintptr_t _noHelmBreakerKnockbackJE;// = 0x0051C367;

	std::string getModName() override { return "SelectiveCancels"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

private:
	hl::Hooker hook;
};