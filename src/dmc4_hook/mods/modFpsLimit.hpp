#pragma once

#include "../mod.hpp"

class FpsLimit : public Mod
{
public:
    FpsLimit();

    static bool modEnabled;
    static uintptr_t jmp_return;
    static float newfpslimit;

    std::string getModName() override { return "FpsLimit"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};