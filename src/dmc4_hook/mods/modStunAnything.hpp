#pragma once

#include "../mod.hpp"

class StunAnything : public Mod
{
public:
    StunAnything() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "StunAnything"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Patch patch;
};