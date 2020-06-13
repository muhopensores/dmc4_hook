#pragma once

#include "../mod.hpp"

class NoClip : public Mod
{
public:
    NoClip() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "NoClip"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
};