#pragma once

#include "../mod.hpp"

class BpPortal : public Mod
{
public:
    BpPortal() = default;

    static bool modEnabled;

    void toggle(bool enable);
    void toggleOffTimer(void);

    std::string getModName() override { return "BpPortal"; };
    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    void onUpdateInput(hl::Input& input) override;


private:
    hl::Patch patch;
};