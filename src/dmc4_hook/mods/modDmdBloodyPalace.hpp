#pragma once

#include "../mod.hpp"

class DmdBloodyPalace : public Mod
{
public:
    DmdBloodyPalace() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "DmdBloodyPalace"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    // hl::Hooker hook;
    hl::Patch patch;
};