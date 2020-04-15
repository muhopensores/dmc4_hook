#pragma once

#include "../mod.hpp"

class AreaJump : public Mod {
public:
	AreaJump();

	static uintptr_t jmp_return;
	static cAreaJump* cAreaJumpPtr;

	virtual std::optional<std::string> onInitialize() override;

	void onGUIframe() override;

private:
	void jumpToStage(int stage);
	hl::Hooker hook;
};