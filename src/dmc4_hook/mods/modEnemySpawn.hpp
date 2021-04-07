#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod
{
public:
    EnemySpawn() = default;

    std::string getModName() override { return "EnemySpawn"; };

	std::optional<std::string> onInitialize() override;
	void onGUIframe() override;
	void onTwitchCommand(std::size_t hash) override;

private:
	std::size_t m_spawnScarecrowArmCommand;
    std::size_t m_spawnScarecrowLegCommand;
	std::size_t m_spawnMegaCommand;
	std::size_t m_spawnFrostCommand;
	std::size_t m_spawnAssaultCommand;
	std::size_t m_spawnBasiliskCommand;
	std::size_t m_spawnMephistoCommand;
	std::size_t m_spawnFaustCommand;
	std::size_t m_spawnBiancoCommand;
	std::size_t m_spawnAltoCommand;
};
