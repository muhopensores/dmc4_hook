#include "../mods.h"

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
		return true;
	};
	/* call from the imgui drawing routine to draw gui elements if you require */
	void onGUIframe() {
		// from main.cpp
		// line 1352 -> if (ImGui::Checkbox("No Helm Breaker Knockback", &checkNoHelmBreakerKnockback))

		// since it's a simple detour we don't need to do anything in event of user clicking imgui checkbox
		// and can just do this
		ImGui::Checkbox("No Helm Breaker Knockback", &modEnabled);
	}

	/* call this in config.cpp/updateMods(); to load values from the config and toggle upon start */
	void loadConfig(CONFIG& config) {
		// from main.cpp
		// line 749 -> checkNoHelmBreakerKnockback = reader.GetBoolean("player", "no_helmbreaker_knockback", true);
		modEnabled = config.player.options["no_helmbreaker_knockback"];
	}
	/* call this in config.cpp/updateConfig(); to save values into CONFIG structure 
	   to be later saved into an ini file.
	*/
	void saveConfig(CONFIG& config) {
		config.player.options["no_helmbreaker_knockback"] = modEnabled;
	}
};