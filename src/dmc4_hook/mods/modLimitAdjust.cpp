#include "../mods.h"

namespace modLimitAdjust {

    constexpr auto swordSwitch_aob        = "D9 80 DC 00 00 00 8D ?? ?? 1D 00 00 D9 9E ?? 1E 00 00 C6 86 ?? 1E 00 00 00 E8";
	constexpr auto gunSwitch_aob          = "D9 81 DC 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? CC";
	constexpr auto jcCooldown_aob         = "D9 81 3C 01 00 00 8B 07 D9 9F ?? ?? 00 00 8B 97 ?? 1E 00 00 D9 82 40 01 00 00 8B 90 94 01 00 00";
	constexpr auto styleSwitch_aob        = "0F 82 ?? ?? 00 00 8B 86 ?? ?? 01 00 3B C7 0F 84 ?? ?? 00 00 39 9E A4 0E 00 00 F3 0F 10";
	constexpr auto movingTargetChange_aob = "75 ?? 0F 57 C0 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 ?? 88 ?? ?? 30 00 00 75";

	bool   enabled[4];

	static uintptr_t locationSwords;
    static uintptr_t locationGuns;
	static uintptr_t locationJCcooldown;
	static uintptr_t locationStyleSwitch;
	static uintptr_t locationMovingTargetChange;
    static hl::Patch patchSwords;
    static hl::Patch patchGuns;
	static hl::Patch patchJCcooldown;
	static hl::Patch patchStyleSwitch;
	static hl::Patch patchMovingTargetChange;

    bool init() {
		locationSwords = hl::FindPattern(swordSwitch_aob);
		locationGuns = hl::FindPattern(gunSwitch_aob);
		locationJCcooldown = hl::FindPattern(jcCooldown_aob);
		locationStyleSwitch = hl::FindPattern(styleSwitch_aob);
		locationMovingTargetChange = hl::FindPattern(movingTargetChange_aob);
		if (!locationSwords || !locationGuns || !locationJCcooldown ||
			!locationStyleSwitch || !locationMovingTargetChange) {
			return false;
		}

		// if everything went well register our ini callbacks.
		// those will be called at ini load/save.
		loadCallbackRegister(loadConfig);
		saveCallbackRegister(saveConfig);

		return true;
    };

	void toggle(bool value, LIMIT_TYPE limit) {
		switch (limit) {
		case SWORD_GUNS: {
			if (value) {
				patchSwords.apply(locationSwords, (char*)&modShared::limitadjust_patch, 6);
				patchGuns.apply(locationGuns, (char*)&modShared::limitadjust_patch, 6);
			}
			else {
				patchSwords.revert();
				patchGuns.revert();
			}
			break;
		}
		case JC: {
			if (value)
				patchJCcooldown.apply(locationJCcooldown, (char*)&modShared::limitadjust_patch, 6);
			else
				patchJCcooldown.revert();
			break;
		}
		case STYLE_SWITCH: {
			if (value)
				patchStyleSwitch.apply(locationStyleSwitch, "\x90\x90\x90\x90\x90\x90", 6);
			else
				patchStyleSwitch.revert();
			break;
		}
		case TARGET_CHANGE: {
			if (value)
				patchMovingTargetChange.apply(locationMovingTargetChange, "\x90\x90", 2);
			else
				patchMovingTargetChange.revert();
			break;
		}
		default:
			break;
		}
	};

	void onGUIframe() {
		if (ImGui::Checkbox("Remove Sword & Gun Switch Limit", &enabled[SWORD_GUNS])) {
			toggle(enabled[SWORD_GUNS], SWORD_GUNS);
		}
		if (ImGui::Checkbox("Remove JC Limit", &enabled[JC])) {
			toggle(enabled[JC], JC);
		}
		if (ImGui::Checkbox("Remove Style Switch Limit", &enabled[STYLE_SWITCH])) {
			toggle(enabled[STYLE_SWITCH], STYLE_SWITCH);
		}
		if (ImGui::Checkbox("Remove Target Switch Limit", &enabled[TARGET_CHANGE])) {
			toggle(enabled[TARGET_CHANGE], TARGET_CHANGE);
		}
	}

	void loadConfig(CONFIG& config) {
		enabled[SWORD_GUNS]    = config.player.options["sword_gun_switch_limits_removed"];
		enabled[JC]            = config.player.options["jc_limits_removed"];
		enabled[STYLE_SWITCH]  = config.player.options["style_switch_limits_removed"];
		enabled[TARGET_CHANGE] = config.player.options["target_change_limit_removed"];
		// iterate over the list and toggle values according to config data.
		for (int i = 0; i < 4; i++) {
			toggle(enabled[i], (LIMIT_TYPE)i);
		}
	};

	void saveConfig(CONFIG& config) {
		config.player.options["sword_gun_switch_limits_removed"] = enabled[SWORD_GUNS];
		config.player.options["jc_limits_removed"]               = enabled[JC];
		config.player.options["style_switch_limits_removed"]     = enabled[STYLE_SWITCH];
		config.player.options["target_change_limit_removed"]     = enabled[TARGET_CHANGE];
	};
};