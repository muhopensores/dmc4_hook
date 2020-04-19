#include "../mods.h"
#include "modNoHBknockback.hpp"
#include "modMoveIDs.hpp"

#if 1
bool      NoHbKnockback::modEnabled{ false };
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackContinue{ 0x0051C389 };
uintptr_t NoHbKnockback::_noHelmBreakerKnockbackJE{ 0x0051C367 };

NoHbKnockback::NoHbKnockback() {
	//onInitialize();
}

naked void noHelmBreakerKnockback_proc(void)
{
	_asm {
		cmp byte ptr [NoHbKnockback::modEnabled],0
		je originalcode

		cmp [MoveIds::moveID],0x20A	// 522 // Low
		je newcode
		cmp [MoveIds::moveID],0x213	// 531 // Mid
		je newcode
		cmp [MoveIds::moveID],0x214	// 532 // High
		je newcode
		jmp originalcode

		newcode:
		cmp ecx,0x05
			je nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]

			nohelmbreakerknockbackje:
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackJE]

			originalcode:
			cmp ecx,0x05
			jl nohelmbreakerknockbackje
			jmp dword ptr [NoHbKnockback::_noHelmBreakerKnockbackContinue]
	}
}

std::optional<std::string> NoHbKnockback::onInitialize() {

	if (!install_hook_offset(0x11C384, hook, &noHelmBreakerKnockback_proc, 0, 6)) {
		HL_LOG_ERR("Failed to init SelectiveCancels mod\n");
		return "Failed to init SelectiveCancels mod";
	}

	return Mod::onInitialize();
}

void NoHbKnockback::onGUIframe() {
	// from main.cpp
	// line 1352 -> if (ImGui::Checkbox("No Helm Breaker Knockback", &checkNoHelmBreakerKnockback))
	ImGui::Checkbox("No Helm Breaker Knockback", &modEnabled);
}

void NoHbKnockback::onConfigLoad(const utils::Config& cfg) {
	modEnabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
};

void NoHbKnockback::onConfigSave(utils::Config& cfg) {
	cfg.set<bool>("no_helmbreaker_knockback", modEnabled);
};

#else

// using external variable from modMoveIDs.cpp
extern "C" int moveID;

// expose functions/variables you need to call from outside in mods.h
namespace modNoHBknockback {

	// mod variables
	static bool modEnabled;
	static uintptr_t noHelmBreakerKnockback;
    //static hl::Patch patch;
	static hl::Hooker hook;

	// using detours should be the same as before.
	extern "C"
	{
		// from naked.h
		// line 43 -> uintptr_t _noHelmBreakerKnockbackContinue = 0x0051C389;
		// line 44 -> uintptr_t _noHelmBreakerKnockbackJE       = 0x0051C367;
		uintptr_t _noHelmBreakerKnockbackContinue = 0x0051C389;
		uintptr_t _noHelmBreakerKnockbackJE = 0x0051C367;
	}
	// from naked.h
	// line 530 -> _declspec(naked) void noHelmBreakerKnockback_proc(void)
	_declspec(naked) void noHelmBreakerKnockback_proc(void)
	{
		_asm {
			cmp byte ptr [modEnabled],0
			je originalcode

			cmp [moveID],0x20A	// 522 // Low
			je newcode
			cmp [moveID],0x213	// 531 // Mid
			je newcode
			cmp [moveID],0x214	// 532 // High
			je newcode
			jmp originalcode

			newcode:
			cmp ecx,0x05
				je nohelmbreakerknockbackje
				jmp dword ptr [_noHelmBreakerKnockbackContinue]

				nohelmbreakerknockbackje:
				jmp dword ptr [_noHelmBreakerKnockbackJE]

				originalcode:
				cmp ecx,0x05
				jl nohelmbreakerknockbackje
				jmp dword ptr [_noHelmBreakerKnockbackContinue]
		}
	}

	/* call this in hlMain::init() to initialize the mod, check the return value in case it fails
	you can optionally pass the data for the function for example module base.
	*/
	bool init(uintptr_t modBase) {
		// from main.cpp
		// line 985 -> noHelmBreakerKnockback = modBase + 0x11C384;
		noHelmBreakerKnockback = modBase + 0x11C384;
		// from main.cpp
		// line 1159 -> auto noHelmBreakerKnockback_hk = m_hook.hookJMP(noHelmBreakerKnockback, 5, &noHelmBreakerKnockback_proc);
		auto noHelmBreakerKnockback_hk = hook.hookJMP(noHelmBreakerKnockback, 5, &noHelmBreakerKnockback_proc);
		if (!noHelmBreakerKnockback_hk) {
			return false;
		}
		// if everything went well register our ini callbacks.
		// those will be called at ini load/save.
		/*loadCallbackRegister(loadConfig);
		saveCallbackRegister(saveConfig);*/
		return true;
	};
	// call from the imgui drawing routine to draw gui elements if you require
	void onGUIframe() {
		// from main.cpp
		// line 1352 -> if (ImGui::Checkbox("No Helm Breaker Knockback", &checkNoHelmBreakerKnockback))

		if (ImGui::Checkbox("No Helm Breaker Knockback", &modEnabled)) {
			modMoveIDs::toggle(modEnabled);
		}
	}

	// will be called during config.cpp/updateConfig() if you've set your callbacks
	// to load values from the config and toggle upon start
	void onConfigLoad(const utils::Config& cfg) {
		modEnabled = cfg.get<bool>("no_helmbreaker_knockback").value_or(false);
		modMoveIDs::toggle(modEnabled);
	};

	// will be called during config.cpp/updateConfig() if you've set your callbacks
	// to write values into config file
	void onConfigSave(utils::Config& cfg) {
		cfg.set<bool>("no_helmbreaker_knockback", modEnabled);
	};
};
#endif