#pragma once

#include "../mod.hpp"

class NoDeath : public Mod
{
public:
    NoDeath() = default;

    static bool cantDie;
    static bool oneHitKill;

    static uintptr_t jmp_ret;
    static uintptr_t jmp_out;

    void NoDeathToggle(bool enable);
    // void toggle2(bool enable);

    std::string getModName() override { return "NoDeath"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    void onUpdateInput(hl::Input& input) override;

private:
    //hl::Patch patchhp;
    hl::Patch patchomen;
    hl::Hooker hook;
};