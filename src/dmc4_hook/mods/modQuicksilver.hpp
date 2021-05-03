#pragma once

#include "../mod.hpp"

// Define ModName class that is a Mod
class Quicksilver : public Mod {
public:
	// using default constructor
	Quicksilver() = default;

	// override getModName() method with your mod name for logging.
	std::string getModName() override { return "Quicksilver"; };

	// override onInitialize() method with routines required to initialize the mod.
	// return Mod::onInitialize() on success or error string on failure.
	std::optional<std::string> onInitialize() override;

	void onFrame(fmilliseconds& dt) override;
	void onGUIframe() override;
	void onTwitchCommand(std::size_t hash) override;

private:
	utils::Timer* m_timer;
	std::size_t m_command;
};