#pragma once
#include <unordered_map>
struct CONFIG {
	struct player {
		// unordered_map is a container that stores elements formed
		// by combination of key value and a mapped value.
		// for example options["key"] = value.
		// we will use it to map ini entries to boolean variables
		// inside mod implementations file.
		std::unordered_map<std::string, bool> options;
	};
	player player;
	struct system {
		std::unordered_map<std::string, bool> options;
	};
	system system;
	struct practice {
		std::unordered_map<std::string, bool> options;
	};
	practice practice;
};
void parseINI();
void saveINI();
//void updateMods();
//extern CONFIG g_config;