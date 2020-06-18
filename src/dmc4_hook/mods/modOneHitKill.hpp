#pragma once

#include "../mod.hpp"

class OneHitKill : public Mod
{
public:
    OneHitKill() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "OneHitKill"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    // hl::Hooker hook;
    hl::Patch patch;
};