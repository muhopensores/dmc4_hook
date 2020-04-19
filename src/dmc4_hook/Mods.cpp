#include "Mods.hpp"
#include "mods/modAreaJump.hpp"
#include "mods/modLimitAdjust.hpp"
#include "mods/modMoveIDs.hpp"
#include "mods/modNoHBknockback.hpp"
#include "mods/modSelectiveCancels.hpp"
#include "mods/modWorkRate.hpp"
#include "utils/Hash.hpp"

Mods::Mods() {
	m_mods["AreaJump"_hash] = std::make_unique<AreaJump>();
	m_mods["LimitAdjust"_hash] = std::make_unique<LimitAdjust>();
	m_mods["MoveIds"_hash] = std::make_unique<MoveIds>();
	m_mods["NoHbKnockback"_hash] = std::make_unique<NoHbKnockback>();
	m_mods["SelectiveCancels"_hash] = std::make_unique<SelectiveCancels>();
	m_mods["WorkRate"_hash] = std::make_unique<WorkRate>();
}

std::optional<std::string> Mods::onInitialize() const {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onInitialize()\n", mod->getModName().data());

		if (auto e = mod->onInitialize(); e != std::nullopt) {
			HL_LOG_RAW("%s::onInitialize() has failed: %s\n", mod->getModName().data(), *e);
			return e;
		}
	}
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
void Mods::onConfigSave(utils::Config& cfg) {
	//HL_LOG_RAW("Saving config to dmc4_hook.cfg\n");

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigSave()\n", mod->getModName().data());
		mod->onConfigSave(cfg);
	}
}
void Mods::onConfigLoad(utils::Config& cfg) {
	//HL_LOG_RAW("Saving config to dmc4_hook.cfg\n");

	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		HL_LOG_RAW("%s::onConfigLoad()\n", mod->getModName().data());
		mod->onConfigLoad(cfg);
	}
}
#endif

void Mods::onGamePause(bool toggle) {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onGamePause(toggle);
	}
}

void Mods::onFrame() {
	for (auto& umod : m_mods) {
		auto& mod = umod.second;
		mod->onFrame();
	}
}

void Mods::onDrawUI(uint32_t hash) {
	m_mods[hash]->onGUIframe();
}

