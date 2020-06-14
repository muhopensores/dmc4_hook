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

