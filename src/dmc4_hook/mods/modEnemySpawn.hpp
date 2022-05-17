#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod
{
public:
    EnemySpawn() = default;

    static int hotkeySpawnModifier;

    static int hotkeySpawnScarecrowLeg;
    static int hotkeySpawnScarecrowArm;
    static int hotkeySpawnMega;
    static int hotkeySpawnBianco;
    static int hotkeySpawnAlto;
    static int hotkeySpawnMephisto;
    static int hotkeySpawnFaust;
    static int hotkeySpawnFrost;
    static int hotkeySpawnAssault;
    static int hotkeySpawnBlitz;
    static int hotkeySpawnChimera;
    static int hotkeySpawnBasilisk;

    std::string getModName() override { return "EnemySpawn"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    void onTwitchCommand(std::size_t hash) override;
    void onUpdateInput(hl::Input& input) override;

private:
    std::size_t m_spawnScarecrowLegCommand;
    std::size_t m_spawnScarecrowArmCommand;
    std::size_t m_spawnMegaCommand;
    std::size_t m_spawnBiancoCommand;
    std::size_t m_spawnAltoCommand;
    std::size_t m_spawnMephistoCommand;
    std::size_t m_spawnFaustCommand;
    std::size_t m_spawnFrostCommand;
    std::size_t m_spawnAssaultCommand;
    std::size_t m_spawnBlitzCommand;
    std::size_t m_spawnChimeraCommand;
    std::size_t m_spawnBasiliskCommand;
    std::size_t m_spawnBerialCommand;
    std::size_t m_spawnBaelCommand;
    std::size_t m_spawnEchidnaCommand;
    std::size_t m_spawnCredoCommand;
    std::size_t m_spawnAgnusCommand;
    std::size_t m_spawnSanctusDiaCommand;
    std::size_t m_spawnKyrieCommand;
    std::size_t m_spawnDanteCommand;
};
