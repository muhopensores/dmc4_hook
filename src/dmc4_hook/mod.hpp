#pragma once
#include "hacklib/PatternScanner.h"
#include "hacklib/Logging.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "imgui/imgui.h"
#include "utils/Config.hpp"
#include "main.h"

class Mod {
public:

	virtual std::optional<std::string> onInitialize() { return std::nullopt; }

	virtual void onFrame() {}
	
	virtual void onGUIframe() {}

	virtual void onConfigLoad(const utils::Config& cfg) {}
	virtual void onConfigSave(utils::Config& cfg) {}

	bool install_hook_offset(ptrdiff_t offset, hl::Hooker& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
		uintptr_t base = GetMain()->modBase;
		uintptr_t location = base + offset;
		HL_LOG_RAW("Installing hook at %p\n", location);
		auto hkRes = hook.hookJMP(location, next_instruction_offset, detour);
		if (!hkRes) {
			return false;
		}
		if (next_instruction_offset) {
			*ret = location + next_instruction_offset;
		}
		return true;
	}
	bool install_hook_absolute(uintptr_t location, hl::Hooker& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
		HL_LOG_RAW("Installing hook at %p\n", location);
		auto hkRes = hook.hookJMP(location, next_instruction_offset, detour);
		if (!hkRes) {
			return false;
		}
		if (next_instruction_offset) {
			*ret = location + next_instruction_offset;
		}
		return true;
	}
};