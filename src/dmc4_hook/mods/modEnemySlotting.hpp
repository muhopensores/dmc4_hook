#pragma once

#include "../mod.hpp"

class EnemySlotting : public Mod
{
public:
    EnemySlotting() = default;

    static bool modEnabled;
    static uintptr_t jmp_ret;

    std::string getModName() override { return "EnemySlotting"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};
