#include "Mods.hpp"
#include "utils/Hash.hpp"
// include mod headers here
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
#include "mods/modFreeCam.hpp"
#include "mods/modForceLucifer.hpp"
#include "mods/modDisableLastEnemyZoom.hpp"
#include "mods/modLdkWithDmd.hpp"
#include "mods/modDmdBloodyPalace.hpp"
#include "mods/modDmdLevelAi.hpp"
#include "mods/modOneHitKill.hpp"
#include "mods/modHideTimer.hpp"
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
#include "mods/modCameraLookdown.hpp"
#include "mods/modNoDeath.hpp"
#include "mods/modEnemySpawn.hpp"
#include "mods/modCameraReset.hpp"
#include "mods/modKnockbackEdits.hpp"
#include "mods/modNoDtCooldown.hpp"
#include "mods/modDtEnemiesDontStun.hpp"
#include "mods/modEnemySlotting.hpp"

//#include "mods/modSample.hpp"
// mods constructor
Mods::Mods() {
	//add mods here
	//m_mods["ModName"_hash] = std::make_unique<ModName>();
	m_mods["AreaJump"_hash] = std::make_unique<AreaJump>();
	m_mods["LimitAdjust"_hash] = std::make_unique<LimitAdjust>();
	m_mods["MoveIds"_hash] = std::make_unique<MoveIds>();
	m_mods["NoHbKnockback"_hash] = std::make_unique<NoHbKnockback>();
	m_mods["EasyJc"_hash] = std::make_unique<EasyJc>();
	m_mods["SelectiveCancels"_hash] = std::make_unique<SelectiveCancels>();
	m_mods["WorkRate"_hash] = std::make_unique<WorkRate>();
	m_mods["BackgroundRendering"_hash] = std::make_unique<BackgroundRendering>();
	m_mods["Borderless"_hash] = std::make_unique<Borderless>();
    m_mods["InfDreadnought"_hash] = std::make_unique<InfDreadnought>();
	m_mods["NoClip"_hash] = std::make_unique<NoClip>();
    m_mods["SkipPandora"_hash] = std::make_unique<SkipPandora>();
	m_mods["RestoreMaxHp"_hash] = std::make_unique<RestoreMaxHp>();
    m_mods["InputStates"_hash] = std::make_unique<InputStates>();
	m_mods["FpsLimit"_hash] = std::make_unique<FpsLimit>();
    m_mods["MoveIdsNero"_hash] = std::make_unique<MoveIdsNero>();
    m_mods["InfSkyStars"_hash] = std::make_unique<InfSkyStars>();
    m_mods["FreeCam"_hash] = std::make_unique<FreeCam>();
    m_mods["ForceLucifer"_hash] = std::make_unique<ForceLucifer>();
    m_mods["DisableLastEnemyZoom"_hash] = std::make_unique<DisableLastEnemyZoom>();
    m_mods["LdkWithDmd"_hash] = std::make_unique<LdkWithDmd>();
    m_mods["DmdBloodyPalace"_hash] = std::make_unique<DmdBloodyPalace>();
    m_mods["DmdLevelAi"_hash] = std::make_unique<DmdLevelAi>();
    m_mods["OneHitKill"_hash] = std::make_unique<OneHitKill>();
    m_mods["HideTimer"_hash] = std::make_unique<HideTimer>();
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
	m_mods["CameraLookdown"_hash] = std::make_unique<CameraLookdown>();
	m_mods["NoDeath"_hash] = std::make_unique<NoDeath>();
	m_mods["EnemySpawn"_hash] = std::make_unique<EnemySpawn>();
	m_mods["CameraReset"_hash] = std::make_unique<CameraReset>();
	m_mods["KnockbackEdits"_hash] = std::make_unique<KnockbackEdits>();
	m_mods["NoDtCooldown"_hash] = std::make_unique<NoDtCooldown>();
	m_mods["DtEnemiesDontStun"_hash] = std::make_unique<DtEnemiesDontStun>();
	m_mods["EnemySlotting"_hash] = std::make_unique<EnemySlotting>();
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
// @TODO: use this version once all the mods are converted to muh c++
#if 0
	auto cwd = hl::GetCurrentModulePath();
	cwd = cwd.substr(0, cwd.find_last_of("\\/"));
	std::string m_confPath = cwd + "\\dmc4_hook.cfg";
	utils::Config cfg{ m_confPath };

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigLoad()\n", mod->getModName().data());
		mod->onConfigLoad(cfg);
	}
#endif
	return std::nullopt;
}
// @TODO: use this version once all the mods are converted to muh c++
#if 0
void Mods::onConfigSave() {
	HL_LOG_RAW("Saving config to dmc4_hook.cfg\n");

	auto cwd = hl::GetCurrentModulePath();
	cwd = cwd.substr(0, cwd.find_last_of("\\/"));
	std::string m_confPath = cwd + "\\dmc4_hook.cfg";
	utils::Config cfg{};

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigSave()\n", mod->getModName().data());
		mod->onConfigSave(cfg);
	}
	if (!cfg.save(m_confPath)) {
		HL_LOG_RAW("Failed to save the config %s\n", m_confPath);
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
void Mods::onFrame() {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onFrame();
	}
}
// Called when drawing the gui
void Mods::onDrawUI(uint32_t hash) {
	m_mods[hash]->onGUIframe();
}

