#pragma once

#include "../mod.hpp"

class FreeCam : public Mod
{
public:
    FreeCam() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "FreeCam"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch;
    hl::Patch patch2;
    hl::Patch patch3;
};