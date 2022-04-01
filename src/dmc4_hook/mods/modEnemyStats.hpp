#pragma once

#include "../mod.hpp"

class EnemyStats : public Mod
{
public:
    EnemyStats() = default;

    std::string getModName() override { return "EnemyStats"; };

    std::optional<std::string> onInitialize() override;

    static int hotkey1;
    static int hotkey2;
    static int hotkey3;
    static int hotkey4;

    void onGUIframe() override;
    void onUpdateInput(hl::Input& input) override;
    void onConfigLoad(const utils::Config& cfg);
    void onConfigSave(utils::Config& cfg);

private:
    hl::Hooker hook;
};
