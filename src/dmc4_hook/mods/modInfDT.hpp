#pragma once

#include "../mod.hpp"

class InfDT : public Mod
{
public:
    InfDT() = default;

    static bool modEnabled;
    static uintptr_t jmp_ret;
    static int hotkey;

    std::string getModName() override { return "InfDT"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;
    void onUpdateInput(hl::Input& input) override;

private:
    hl::Hooker hook;
};