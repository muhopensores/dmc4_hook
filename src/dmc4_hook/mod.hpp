#pragma once
#include "hacklib/PatternScanner.h"
#include "hacklib/Logging.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "imgui/imgui.h"
#include "utils/Config.hpp"
#include "sdk/ReClass_Internal.hpp"
#include "main.h"

#define naked static __declspec(naked)

#include <chrono>

class Mod {
public:

	void HelpMarker(const char* desc) {
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

	bool install_hook_offset(ptrdiff_t offset, hl::Hooker& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
		uintptr_t base = GetMain()->modBase;
		uintptr_t location = base + offset;
		HL_LOG_RAW("[%s]: Installing hook at %p.\n", getModName().c_str(), location);
		auto hkRes = hook.hookJMP(location, next_instruction_offset, detour);
		if (!hkRes) {
			return false;
		}
		if (next_instruction_offset) {
			if (ret) {
				*ret = location + next_instruction_offset;
			}
		}
		return true;
	}
	bool install_hook_absolute(uintptr_t location, hl::Hooker& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
		HL_LOG_RAW("[%s]: Installing hook at %p.\n", getModName().c_str(), location);
		auto hkRes = hook.hookJMP(location, next_instruction_offset, detour);
		if (!hkRes) {
			return false;
		}
		if (next_instruction_offset) {
			if (ret) {
				*ret = location + next_instruction_offset;
			}
		}
		return true;
	}

	virtual std::string getModName() { return "UnknownMod"; };

	virtual std::optional<std::string> onInitialize() { return std::nullopt; }
	// should be called from d3d hook
	virtual void onFrame() {}
	// called only when imgui window displays
	virtual void onGUIframe() {}

	virtual void onConfigLoad(const utils::Config& cfg) {}
	virtual void onConfigSave(utils::Config& cfg) {}

	//game specific callbacks
	virtual void onGamePause(bool toggle) {};
	virtual bool onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) { return true; };
};