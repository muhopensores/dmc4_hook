#pragma once

#include "../mod.hpp"

class DmdLevelAi : public Mod
{
public:
    DmdLevelAi() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "DmdLevelAi"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    // hl::Hooker hook;
    hl::Patch patch;
};
