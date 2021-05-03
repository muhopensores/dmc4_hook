#include "Mods.hpp"
#include "utils/Hash.hpp"
// include mod headers here
#include "mods/modFastStart.hpp"
#include "mods/modAreaJump.hpp"
#include "mods/modLimitAdjust.hpp"
#include "mods/modMoveIDs.hpp"
#include "mods/modNoHBknockback.hpp"
#include "mods/modEasyJc.hpp"
#include "mods/modSelectiveCancels.hpp"
#include "mods/modWorkRate.hpp"
#include "mods/modBackgroundRendering.hpp"
#include "mods/modBorderless.hpp"
#include "mods/modInfDreadnought.hpp"
#include "mods/modNoClip.hpp"
#include "mods/modSkipPandora.hpp"
#include "mods/modRestoreMaxHp.hpp"
#include "mods/modInputStates.hpp"
#include "mods/modFpsLimit.hpp"
#include "mods/modMoveIDsNero.hpp"
#include "mods/modInfSkyStars.hpp"
#include "mods/modForceLucifer.hpp"
#include "mods/modLdkWithDmd.hpp"
#include "mods/modDmdBloodyPalace.hpp"
#include "mods/modDmdLevelAi.hpp"
#include "mods/modOneHitKill.hpp"
#include "mods/modManualTwosomeTime.hpp"
#include "mods/modHeightRestrictionDante.hpp"
#include "mods/modInfFaustCloak.hpp"
#include "mods/modPassiveEnemies.hpp"
#include "mods/modHeightRestrictionNero.hpp"
#include "mods/modInfiniteTime.hpp"
#include "mods/modInfAllHealth.hpp"
#include "mods/modCameraSettings.hpp"
#include "mods/modFreezeEnemies.hpp"
#include "mods/modFreeJc.hpp"
#include "mods/modNoDeath.hpp"
#include "mods/modEnemySpawn.hpp"
#include "mods/modKnockbackEdits.hpp"
#include "mods/modNoDtCooldown.hpp"
#include "mods/modDtEnemiesDontStun.hpp"
#include "mods/modEnemySlotting.hpp"
#include "mods/modPlayerTracker.hpp"
#include "mods/modTrackingFullHouse.hpp"
#include "mods/modInfTrickRange.hpp"
#include "mods/modTimerMem.hpp"
#include "mods/modTrickDown.hpp"
#include "mods/modDamageMultiplier.hpp"
#include "mods/modHpInOrbsDisplay.hpp"
#include "mods/modInfDT.hpp"
#include "mods/modInfPlayerHealth.hpp"
#include "mods/modBerialDaze.hpp"
#include "mods/modRoseRemovesPins.hpp"
#include "mods/modStunAnything.hpp"
#include "mods/modDisableCameraEvents.hpp"
#include "mods/modHideHud.hpp"
#include "mods/modEnemyDT.hpp"
#include "mods/modBpPortal.hpp"
#include "mods/modFastPandora.hpp"
#include "mods/modInfAirHikes.hpp"
#include "mods/modBpBossRush.hpp"
#include "mods/modInfRevive.hpp"
#include "mods/modCharacterSwap.hpp"
#include "mods/modDisableDarkslayer.hpp"
#include "mods/modRemoveLaunchArmour.hpp"
#include "mods/modCutsceneSkip.hpp"
#include "mods/modFastSprint.hpp"
#include "mods/modInfTableHopper.hpp"
#include "mods/modEnemyAttackOffScreen.hpp"
#include "mods/modSlowWalk.hpp"
#include "mods/modDifficultySelect.hpp"
#include "mods/modEnemyReplace.hpp"
#include "mods/modBpJumpHook.hpp"
#include "mods/modNoAutomaticCharacters.hpp"
#include "mods/modDisableChimeraBlades.hpp"
#include "mods/modTwitch.hpp"
#include "mods/modQuicksilver.hpp"


//#include "mods/modSample.hpp"
// mods constructor
Mods::Mods() {
	    m_slowMods["BackgroundRendering"_hash] = std::make_unique<BackgroundRendering>();
		m_slowMods["Borderless"_hash] = std::make_unique<Borderless>();
		//add mods here
		//m_mods["ModName"_hash] = std::make_unique<ModName>();
		m_mods["FastStart"_hash] = std::make_unique<FastStart>();
		m_mods["AreaJump"_hash] = std::make_unique<AreaJump>();
		m_mods["LimitAdjust"_hash] = std::make_unique<LimitAdjust>();
		m_mods["MoveIds"_hash] = std::make_unique<MoveIds>();
		m_mods["NoHbKnockback"_hash] = std::make_unique<NoHbKnockback>();
		m_mods["EasyJc"_hash] = std::make_unique<EasyJc>();
		m_mods["SelectiveCancels"_hash] = std::make_unique<SelectiveCancels>();
		m_mods["WorkRate"_hash] = std::make_unique<WorkRate>();
		m_mods["InfDreadnought"_hash] = std::make_unique<InfDreadnought>();
		m_mods["NoClip"_hash] = std::make_unique<NoClip>();
		m_mods["SkipPandora"_hash] = std::make_unique<SkipPandora>();
		m_mods["RestoreMaxHp"_hash] = std::make_unique<RestoreMaxHp>();
		m_mods["InputStates"_hash] = std::make_unique<InputStates>();
		m_mods["FpsLimit"_hash] = std::make_unique<FpsLimit>();
		m_mods["MoveIdsNero"_hash] = std::make_unique<MoveIdsNero>();
		m_mods["InfSkyStars"_hash] = std::make_unique<InfSkyStars>();
		m_mods["ForceLucifer"_hash] = std::make_unique<ForceLucifer>();
		m_mods["LdkWithDmd"_hash] = std::make_unique<LdkWithDmd>();
		m_mods["DmdBloodyPalace"_hash] = std::make_unique<DmdBloodyPalace>();
		m_mods["DmdLevelAi"_hash] = std::make_unique<DmdLevelAi>();
		m_mods["OneHitKill"_hash] = std::make_unique<OneHitKill>();
		m_mods["ManualTwosomeTime"_hash] = std::make_unique<ManualTwosomeTime>();
		m_mods["HeightRestrictionDante"_hash] = std::make_unique<HeightRestrictionDante>();
		m_mods["InfFaustCloak"_hash] = std::make_unique<InfFaustCloak>();
		m_mods["PassiveEnemies"_hash] = std::make_unique<PassiveEnemies>();
		m_mods["HeightRestrictionNero"_hash] = std::make_unique<HeightRestrictionNero>();
		m_mods["InfiniteTime"_hash] = std::make_unique<InfiniteTime>();
		m_mods["InfAllHealth"_hash] = std::make_unique<InfAllHealth>();
		m_mods["CameraSettings"_hash] = std::make_unique<CameraSettings>();
		m_mods["FreezeEnemies"_hash] = std::make_unique<FreezeEnemies>();
		m_mods["FreeJc"_hash] = std::make_unique<FreeJc>();
		m_mods["NoDeath"_hash] = std::make_unique<NoDeath>();
		m_mods["EnemySpawn"_hash] = std::make_unique<EnemySpawn>();
		m_mods["KnockbackEdits"_hash] = std::make_unique<KnockbackEdits>();
		m_mods["NoDtCooldown"_hash] = std::make_unique<NoDtCooldown>();
		m_mods["DtEnemiesDontStun"_hash] = std::make_unique<DtEnemiesDontStun>();
		m_mods["EnemySlotting"_hash] = std::make_unique<EnemySlotting>();
		m_mods["PlayerTracker"_hash] = std::make_unique<PlayerTracker>();
		m_mods["TrackingFullHouse"_hash] = std::make_unique<TrackingFullHouse>();
		m_mods["InfTrickRange"_hash] = std::make_unique<InfTrickRange>();
        m_mods["TimerMem"_hash] = std::make_unique<TimerMem>();
		m_mods["TrickDown"_hash] = std::make_unique<TrickDown>();
        m_mods["DamageMultiplier"_hash] = std::make_unique<DamageMultiplier>();
		m_mods["HpInOrbsDisplay"_hash] = std::make_unique<HpInOrbsDisplay>();
        m_mods["InfDT"_hash] = std::make_unique<InfDT>();
		m_mods["InfPlayerHealth"_hash] = std::make_unique<InfPlayerHealth>();
        m_mods["BerialDaze"_hash] = std::make_unique<BerialDaze>();
		m_mods["RoseRemovesPins"_hash] = std::make_unique<RoseRemovesPins>();
        m_mods["StunAnything"_hash] = std::make_unique<StunAnything>();
		m_mods["DisableCameraEvents"_hash] = std::make_unique<DisableCameraEvents>();
        m_mods["HideHud"_hash] = std::make_unique<HideHud>();
		m_mods["EnemyDT"_hash] = std::make_unique<EnemyDT>();
        m_mods["BpPortal"_hash] = std::make_unique<BpPortal>();
		m_mods["FastPandora"_hash] = std::make_unique<FastPandora>();
        m_mods["InfAirHikes"_hash] = std::make_unique<InfAirHikes>();
		m_mods["BpBossRush"_hash] = std::make_unique<BpBossRush>();
        m_mods["InfRevive"_hash] = std::make_unique<InfRevive>();
		m_mods["CharacterSwap"_hash] = std::make_unique<CharacterSwap>();
        m_mods["DisableDarkslayer"_hash] = std::make_unique<DisableDarkslayer>();
		m_mods["RemoveLaunchArmour"_hash] = std::make_unique<RemoveLaunchArmour>();
        m_mods["CutsceneSkip"_hash] = std::make_unique<CutsceneSkip>();
		m_mods["FastSprint"_hash] = std::make_unique<FastSprint>();
        m_mods["InfTableHopper"_hash] = std::make_unique<InfTableHopper>();
		m_mods["EnemyAttackOffScreen"_hash] = std::make_unique<EnemyAttackOffScreen>();
        m_mods["SlowWalk"_hash] = std::make_unique<SlowWalk>();
		m_mods["DifficultySelect"_hash] = std::make_unique<DifficultySelect>();
        m_mods["EnemyReplace"_hash] = std::make_unique<EnemyReplace>();
		m_mods["BpJumpHook"_hash] = std::make_unique<BpJumpHook>();
        m_mods["NoAutomaticCharacters"_hash] = std::make_unique<NoAutomaticCharacters>();
		m_mods["DisableChimeraBlades"_hash] = std::make_unique<DisableChimeraBlades>();
		m_mods["TwitchClient"_hash] = std::make_unique<TwitchClient>();
		m_mods["Quicksilver"_hash] = std::make_unique<Quicksilver>();
}

std::optional<std::string> Mods::onSlowInitialize() const {
	for (auto& umod : m_slowMods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onSlowInitialize()\n", mod->getModName().data());

		if (auto e = mod->onInitialize(); e != std::nullopt) {
			HL_LOG_RAW("%s::onSlowInitialize() has failed: %s\n", mod->getModName().data(), *e);
			return e;
		}
	}

	utils::Config cfg{ "dmc4_hook.cfg" };

	for (auto& umod : m_slowMods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigLoad()\n", mod->getModName().data());
		mod->onConfigLoad(cfg);
	}
	return std::nullopt;
}

// Initializes mods, checks for errors
std::optional<std::string> Mods::onInitialize() const {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onInitialize()\n", mod->getModName().data());

		if (auto e = mod->onInitialize(); e != std::nullopt) {
			HL_LOG_RAW("%s::onInitialize() has failed: %s\n", mod->getModName().data(), *e);
			return e;
		}
	}


	utils::Config cfg{ "dmc4_hook.cfg" };

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigLoad()\n", mod->getModName().data());
		mod->onConfigLoad(cfg);
	}

	return std::nullopt;
}
// @TODO: use this version once all the mods are converted to muh c++
#if 1
void Mods::onConfigSave() {
	HL_LOG_RAW("Saving config to dmc4_hook.cfg\n");

	utils::Config cfg{};

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigSave()\n", mod->getModName().data());
		mod->onConfigSave(cfg);
	}
	if (!cfg.save("dmc4_hook.cfg")) {
		HL_LOG_RAW("Failed to save the config\n");
	}
}
#else
// Walks over the unordered_map and calls saveConfig
void Mods::onConfigSave(utils::Config& cfg) {
	//HL_LOG_RAW("Saving config to dmc4_hook.cfg\n");

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigSave()\n", mod->getModName().data());
		mod->onConfigSave(cfg);
	}
}
// Walks over unordered_map and calls loadConfig
void Mods::onConfigLoad(utils::Config& cfg) {
	HL_LOG_RAW("Loading config from dmc4_hook.cfg\n");

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigLoad()\n", mod->getModName().data());
		mod->onConfigLoad(cfg);
	}
}
#endif
// Called when showing ui, currently only WorkRate uses this to freeze time.
void Mods::onGamePause(bool toggle) {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onGamePause(toggle);
	}
}
bool Mods::onMessage(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		if (!mod->onMessage(wnd, message, wParam, lParam)) {
			return false;
		}
	}
	return true;
}
// Called on every frame for mods that override this method
void Mods::onFrame(fmilliseconds& dt) {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onFrame(dt);
	}
}
// Called when drawing the gui
void Mods::onDrawUI(uint32_t hash) {
	m_mods[hash]->onGUIframe();
}

// Called when drawing the gui
void Mods::onDrawSlowUI(uint32_t hash) {
	m_slowMods[hash]->onGUIframe();
}

// this is terrible atm
void Mods::onChatCommand(const std::string& message) {
	std::size_t hash = std::hash<std::string>{}(message);
	HL_LOG_RAW("[TwitchCommand] hash:%d message:%s", hash, message.c_str());
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onTwitchCommand(hash);
	}
}
