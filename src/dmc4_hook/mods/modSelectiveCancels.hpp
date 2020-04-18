#pragma once

#include "../mod.hpp"

// using powers of two here to avoid making alot of bool variables
enum CANCEL_MOVES {       // bits table
	ECSTASY         = 1,  // 0000 0001
	ARGUMENT        = 2,  // 0000 0010
	KICK13          = 4,  // 0000 0100
	SLASH_DIMENSION = 8,  // 0000 1000
	PROP            = 16, // 0001 0000
	SHOCK           = 32, // 0010 0000
	OMEN            = 64  // 0100 0000
};

class SelectiveCancels : public Mod {
public:
	SelectiveCancels();

	static bool      selectiveCancelsEnable;// = false;
	static uint32_t  cancels;// = 0;
	static uintptr_t _selectiveCancelsContinue; //= 0x0080332F;

	std::string getModName() override { return "SelectiveCancels"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

	inline void drawCheckboxSimple(const char* name, CANCEL_MOVES move);
private:
	hl::Hooker detour;
};