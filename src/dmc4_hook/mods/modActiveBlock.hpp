#pragma once

#include "../mod.hpp"

class ActiveBlock : public Mod
{
public:
    ActiveBlock() = default;
    static bool modEnabled;
    static uint32_t inputpressed;

    static uintptr_t jmp_return;
    static uintptr_t alt_ret;

    std::string getModName() override { return "ActiveBlock"; };

    std::optional<std::string> onInitialize() override;
    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;

private:
    hl::Hooker hook;
};