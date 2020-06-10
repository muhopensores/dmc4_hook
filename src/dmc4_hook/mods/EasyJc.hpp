#pragma once

#include "../mod.hpp"

class EasyJc : public Mod
{
public:
    EasyJc();

    static bool modEnabled;
    static uintptr_t _easyJcContinue;

    std::string getModName() override { return "EasyJc"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};