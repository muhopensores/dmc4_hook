#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod
{
public:
    EnemySpawn() = default;

    std::string getModName() override { return "EnemySpawn"; };

	std::optional<std::string> onInitialize() override;
	void onGUIframe() override;

};