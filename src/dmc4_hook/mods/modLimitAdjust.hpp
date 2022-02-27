#pragma once

#include "../mod.hpp"

#if 1
class LimitAdjust : public Mod
{
public:
    LimitAdjust() = default;

	static bool gunSword;
    static bool enemyStep;
	static bool style;
	static bool target;

    static float limit;

    void SwordAndGun(bool enable);
    void EnemyStep(bool enable);
    void Style(bool enable);
    void Target(bool enable);

    std::string getModName() override { return "LimitAdjust"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patchsword;
    hl::Patch patchgun;
    hl::Patch patchenemystep;
    hl::Patch patchstyle;
    hl::Patch patchtarget;
};

#else
struct limitStruct {
	bool      m_enabled{ false };
	uintptr_t m_location{ NULL };
	hl::Patch m_patch;
};

class LimitAdjust : public Mod {
public:

	enum LIMIT_TYPE {
		SWORDS,
		GUNS,
		JC,
		STYLE_SWITCH,
		TARGET_CHANGE
	};

	static float limit;

	LimitAdjust();

	std::string getModName() override { return "LimitAdjust"; };

	std::optional<std::string> onInitialize() override;

	void onConfigLoad(const utils::Config& cfg) override;
	void onConfigSave(utils::Config& cfg) override;

	void onGUIframe() override;

private:
	std::array<limitStruct, 5> m_limit;
	const std::array<const char*, 5> cfgStrings = {
		"sword_switch_limits_removed",
		"gun_switch_limits_removed",
		"jc_limits_removed",
		"style_switch_limits_removed",
		"target_change_limit_removed"
	};
	/*uintptr_t locationSwords;
	uintptr_t locationGuns;
	uintptr_t locationJCcooldown;
	uintptr_t locationStyleSwitch;
	uintptr_t locationMovingTargetChange;*/
	/*
	hl::Patch patchSwords;
	hl::Patch patchGuns;
	hl::Patch patchJCcooldown;
	hl::Patch patchStyleSwitch;
	hl::Patch patchMovingTargetChange;*/

	void toggle(int index);
};
#endif