#pragma once

#include "../mod.hpp"

class AreaJump : public Mod {
public:
	AreaJump() = default;

	static uintptr_t jmp_return;
	static cAreaJump* cAreaJumpPtr;

	std::string getModName() override { return "AreaJump"; };

	std::optional<std::string> onInitialize() override;

	void onGUIframe() override;

private:
	void jumpToStage(int stage);
	hl::Hooker hook;
};