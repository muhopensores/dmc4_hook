#pragma once

#include "../mod.hpp"

class SkipPandora : public Mod
{
public:
    SkipPandora();

	static bool      modEnabled;
    static uintptr_t _skipPandoraContinue;

    std::string getModName() override { return "SkipPandora"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};