#pragma once

#include "../mod.hpp"

// Define ModName class that is a Mod
class BigHeadMode : public Mod {
public:
	// using default constructor
	BigHeadMode() = default;

	// override getModName() method with your mod name for logging.
	std::string getModName() override { return "BigHeadMode"; };

	// override onInitialize() method with routines required to initialize the mod.
	// return Mod::onInitialize() on success or error string on failure.
	std::optional<std::string> onInitialize() override;

	/*void onFrame(fmilliseconds& dt) override;*/
	void onGUIframe() override;
	void onTwitchCommand(std::size_t hash) override;

private:

	std::size_t m_command;
	hl::Hooker hook1;
	hl::Hooker hook2;
};