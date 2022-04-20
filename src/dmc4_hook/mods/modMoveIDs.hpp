#pragma once

#include "../mod.hpp"

class MoveIds : public Mod {
public:
	MoveIds() = default;

	static uintptr_t jmp_return;
	static uint32_t  moveID;

	std::string getModName() override { return "MoveIds"; };

	std::optional<std::string> onInitialize() override;

private:
	hl::Hooker hook;
};