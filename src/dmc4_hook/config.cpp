#include "config.h"
#include "mods.h"
#include "utils/ini.h"
#include <iostream>

CONFIG g_config;

struct INI {
	mINI::INIFile file;
	mINI::INIStructure ini;
	bool ini_present = false;
	INI(std::string const& filename) : file(filename) {}
};
static INI ini_struct("mods.ini");

enum CONF_TYPE {
	CONF_PLAYER,
	CONF_SYSTEM,
	CONF_PRACTICE,
	CONF_MAX
};

/* after initiazlizing CONFIG structure with keys=>values from ini we can pass it to mods.
or if the file was not present will populate CONFIG struct with default values (key=>false)
*/
static void updateMods() {
	// modNamespace::loadConfig(g_config) goes here
	modSelCancels::loadConfig(g_config);
	modLimitAdjust::loadConfig(g_config);
	modBackgroundRendering::loadConfig(g_config);
}
/* updating config values with data from mods */
void updateConfig() {
	// modNamespace::saveConfig(g_config) goes here
	modSelCancels::saveConfig(g_config);
	modLimitAdjust::saveConfig(g_config);
	modBackgroundRendering::saveConfig(g_config);
}

static void populateConfig(const std::string& key, const std::string& value, CONF_TYPE conf) {
	bool result = value != "0";
	switch (conf) {
	case CONF_PLAYER:
		g_config.player.options[key] = result;
		break;
	case CONF_SYSTEM:
		g_config.system.options[key] = result;
		break;
	case CONF_PRACTICE:
		g_config.practice.options[key] = result;
	default:
		break;
	}
}

void parseINI() {

#ifdef _DEBUG
	char cwd[MAX_PATH] = { 0 };
	GetCurrentDirectory(MAX_PATH, cwd);
	std::string cwd_std = cwd;
	std::cout << cwd_std << std::endl;
#endif
	// read ini file into ini structure in memory.
	auto read_result = ini_struct.file.read(ini_struct.ini);
	if (!read_result) {
		// file is not present
		ini_struct.ini_present = read_result;
		updateMods();
		return;
	}
	std::string categories[3];
	categories[0] = "player";
	categories[1] = "system";
	categories[2] = "practice";
	// iterating over all of the entries in [section]
	for (int i = CONF_PLAYER; i < CONF_MAX; i++) {
		auto const& section = categories[i];
		for (auto const& it : ini_struct.ini[section]) {
			auto const& key = it.first;
			auto const& value = it.second;
			populateConfig(key, value, (CONF_TYPE)i);
		}
	}
	updateMods();
}

/* saving CONFIG entries into ini data */
void updateINI() {

	for (auto const& it : g_config.player.options) {
		auto const& key = it.first;
		auto const& value = it.second;
		ini_struct.ini["player"][key] = value ? "1" : "0";
	}
	for (auto const& it : g_config.system.options) {
		auto const& key = it.first;
		auto const& value = it.second;
		ini_struct.ini["system"][key] = value ? "1" : "0";
	}
	for (auto const& it : g_config.practice.options) {
		auto const& key = it.first;
		auto const& value = it.second;
		ini_struct.ini["practice"][key] = value ? "1" : "0";
	}
}

/* call to save mod data into ini file.*/
void saveINI() {
	updateConfig();
	updateINI();
	ini_struct.file.write(ini_struct.ini, true);
	//ini_struct.file.generate(ini_struct.ini, true);
}