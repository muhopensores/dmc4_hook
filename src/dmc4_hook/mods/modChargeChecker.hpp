#pragma once

#include "../mod.hpp"

class ChargeChecker : public Mod
{
public:
    ChargeChecker() = default;

    static bool modEnabled;
    static uintptr_t jmp_ret;
    static uintptr_t jmp_ret2;

    std::string getModName() override { return "ChargeChecker"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
};
