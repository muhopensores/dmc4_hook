#pragma once

#include "../mod.hpp"

class NoDeath : public Mod
{
public:
    NoDeath() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "NoDeath"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patchhp;
    hl::Patch patchomen;
};