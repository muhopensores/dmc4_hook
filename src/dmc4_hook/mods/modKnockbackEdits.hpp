#pragma once

#include "../mod.hpp"

class KnockbackEdits : public Mod
{
public:
    KnockbackEdits();

    static bool modEnabled;
    static uintptr_t jmp_return;

    std::string getModName() override { return "KnockbackEdits"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
};