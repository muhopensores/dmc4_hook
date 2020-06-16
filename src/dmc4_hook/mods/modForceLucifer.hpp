#pragma once

#include "../mod.hpp"

class ForceLucifer : public Mod
{
public:
    ForceLucifer();

    static bool modEnabled;
    static uintptr_t _forceLuciferContinue;

    std::string getModName() override { return "ForceLucifer"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};