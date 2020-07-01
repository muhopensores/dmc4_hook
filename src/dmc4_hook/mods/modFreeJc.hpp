#pragma once

#include "../mod.hpp"

class FreeJc : public Mod
{
public:
    FreeJc() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "FreeJc"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Patch patch1;
    hl::Patch patch2;
};