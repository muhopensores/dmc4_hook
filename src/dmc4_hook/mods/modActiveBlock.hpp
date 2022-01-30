#pragma once

#include "../mod.hpp"

class ActiveBlock : public Mod
{
public:
    ActiveBlock();
    static bool modEnabled;
    static uintptr_t jmp_return;
    static uint32_t inputpressed;
    static uint32_t alt_ret;

    static uintptr_t jmp_return2;
    static uintptr_t jmp_return3;
    static uintptr_t alt_ret3;

    std::string getModName() override { return "ActiveBlock"; };

    std::optional<std::string> onInitialize() override;
    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
    hl::Hooker hook3;
};