#pragma once

#include "../mod.hpp"

class InfDT : public Mod
{
public:
    InfDT();

    static bool modEnabled;
    static uintptr_t jmp_ret;

    std::string getModName() override { return "InfDT"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};