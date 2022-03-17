#pragma once

#include "../mod.hpp"

class InfAllHealth : public Mod {
public:

	InfAllHealth() = default;

	static uintptr_t jmp_return;
	static bool modEnabled;
	static int hotkey;

	std::string getModName() override { return "InfAllHealth"; };
	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;
	void onGUIframe() override;
	//void onFrame(fmilliseconds& dt) override;
    void onUpdateInput(hl::Input& input) override;

private:
    hl::Hooker hook;
};