/*
#pragma once

#include "../mod.hpp"

class EnemySpawn : public Mod
{
public:
    EnemySpawn();

    static bool modEnabled;
    static uintptr_t _enemySpawnContinue;

    std::string getModName() override { return "EnemySpawn"; };

    // std::optional<std::string> onInitialize() override;
    /*
    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override; //line 2091 of main.cpp

private:
    hl::Hooker hook;
};
*/