#include "../mods.h"
#include "modLimitAdjust.hpp"

#if 1

float LimitAdjust::limit{ 0.0f };

LimitAdjust::LimitAdjust() {
	onInitialize();
}

naked void limitadjust_patch(void) {
	_asm {
		fld dword ptr[LimitAdjust::limit]
	}
}


std::optional<std::string> LimitAdjust::onInitialize() {
	// c++ 17 feature i think
	constexpr std::array aobs = {
		"D9 80 DC 00 00 00 8D ?? ?? 1D 00 00 D9 9E ?? 1E 00 00 C6 86 ?? 1E 00 00 00 E8", //swords
		"D9 81 DC 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? CC", //guns
		"D9 81 3C 01 00 00 8B 07 D9 9F ?? ?? 00 00 8B 97 ?? 1E 00 00 D9 82 40 01 00 00 8B 90 94 01 00 00", //jc
		"0F 82 ?? ?? 00 00 8B 86 ?? ?? 01 00 3B C7 0F 84 ?? ?? 00 00 39 9E A4 0E 00 00 F3 0F 10", //styleswitch
		"75 ?? 0F 57 C0 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 ?? 88 ?? ?? 30 00 00 75" //targetchange
	};

	/*constexpr auto swordSwitch_aob        = "D9 80 DC 00 00 00 8D ?? ?? 1D 00 00 D9 9E ?? 1E 00 00 C6 86 ?? 1E 00 00 00 E8";
	constexpr auto gunSwitch_aob          = "D9 81 DC 00 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 00 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? CC";
	constexpr auto jcCooldown_aob         = "D9 81 3C 01 00 00 8B 07 D9 9F ?? ?? 00 00 8B 97 ?? 1E 00 00 D9 82 40 01 00 00 8B 90 94 01 00 00";
	constexpr auto styleSwitch_aob        = "0F 82 ?? ?? 00 00 8B 86 ?? ?? 01 00 3B C7 0F 84 ?? ?? 00 00 39 9E A4 0E 00 00 F3 0F 10";
	constexpr auto movingTargetChange_aob = "75 ?? 0F 57 C0 ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? ?? 00 ?? 88 ?? ?? 30 00 00 75";
	*/
	/*m_limit[SWORDS].m_location        = hl::FindPattern(swordSwitch_aob);
	m_limit[GUNS].m_location          = hl::FindPattern(gunSwitch_aob);
	m_limit[JC].m_location            = hl::FindPattern(jcCooldown_aob);
	m_limit[STYLE_SWITCH].m_location  = hl::FindPattern(styleSwitch_aob);
	m_limit[TARGET_CHANGE].m_location = hl::FindPattern(movingTargetChange_aob);*/
	int index = 0;
	for (auto& limit : m_limit) {
		limit.m_location = hl::FindPattern(aobs[index]);
		if (!limit.m_location) {
			return "Failed to init Limit Adjust mod.";
		}
		index++;
	}

	return Mod::onInitialize();
}

void LimitAdjust::toggle(int index) {
	if (m_limit[index].m_enabled) {
		m_limit[index].m_patch.apply(m_limit[index].m_location, (char*)limitadjust_patch);
	}
	else {
		m_limit[index].m_patch.revert();
	}
};

void LimitAdjust::onConfigLoad(const utils::Config& cfg) {

	int index;
	for (auto& limit : m_limit) {
		limit.m_enabled = cfg.get<bool>(cfgStrings[index]).value_or(false);
		toggle(index);
		++index;
	}
	/*m_limit[SWORDS].m_enabled        = cfg.get<bool>("sword_gun_switch_limits_removed").value_or(false);
	m_limit[GUNS].m_enabled          = cfg.get<bool>("sword_gun_switch_limits_removed").value_or(false);
	m_limit[JC].m_enabled            = cfg.get<bool>("jc_limits_removed").value_or(false);
	m_limit[STYLE_SWITCH].m_enabled  = cfg.get<bool>("style_switch_limits_removed").value_or(false);
	m_limit[TARGET_CHANGE].m_enabled = cfg.get<bool>("target_change_limit_removed").value_or(false);*/
}
void LimitAdjust::onConfigSave(utils::Config& cfg) {
	int index;
	for (auto& limit : m_limit) {
		cfg.set<bool>(cfgStrings[index], limit.m_enabled);
		index++;
	}
	/*cfg.set<bool>("sword_gun_switch_limits_removed", m_limit[SWORDS].m_enabled);
	cfg.set<bool>("sword_gun_switch_limits_removed", m_limit[GUNS].m_enabled);
	cfg.set<bool>("jc_limits_removed",               m_limit[JC].m_enabled);
	cfg.set<bool>("style_switch_limits_removed",     m_limit[STYLE_SWITCH].m_enabled);
	cfg.set<bool>("target_change_limit_removed",     m_limit[TARGET_CHANGE].m_enabled);*/
}

void LimitAdjust::onGUIframe() {
	if (ImGui::Checkbox("Remove Sword & Gun Switch Limit", &m_limit[SWORDS].m_enabled)) {
		m_limit[GUNS].m_enabled = m_limit[SWORDS].m_enabled;
		toggle(SWORDS);
		toggle(GUNS);
	}
	if (ImGui::Checkbox("Remove JC Limit", &m_limit[JC].m_enabled)) {
		toggle(JC);
	}
	if (ImGui::Checkbox("Remove Style Switch Limit", &m_limit[STYLE_SWITCH].m_enabled)) {
		toggle(STYLE_SWITCH);
	}
	if (ImGui::Checkbox("Remove Target Switch Limit", &m_limit[TARGET_CHANGE].m_enabled)) {
		toggle(TARGET_CHANGE);
	}
}

#else
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
		/*loadCallbackRegister(loadConfig);
		saveCallbackRegister(saveConfig);*/

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
	void onConfigLoad(const utils::Config& cfg) {
		enabled[SWORD_GUNS]   = cfg.get<bool>("sword_gun_switch_limits_removed").value_or(false);
		enabled[JC]           = cfg.get<bool>("jc_limits_removed").value_or(false);
		enabled[STYLE_SWITCH] = cfg.get<bool>("style_switch_limits_removed").value_or(false);
		enabled[SWORD_GUNS]   = cfg.get<bool>("target_change_limit_removed").value_or(false);
		for (int i = 0; i < 4; i++) {
			toggle(enabled[i], (LIMIT_TYPE)i);
		}
	};

	// will be called during config.cpp/updateConfig() if you've set your callbacks
	// to write values into config file
	void onConfigSave(utils::Config& cfg) {
		cfg.set<bool>("sword_gun_switch_limits_removed", enabled[SWORD_GUNS]);
		cfg.set<bool>("jc_limits_removed", enabled[JC]);
		cfg.set<bool>("style_switch_limits_removed", enabled[STYLE_SWITCH]);
		cfg.set<bool>("target_change_limit_removed", enabled[TARGET_CHANGE]);
	};
};
#endif