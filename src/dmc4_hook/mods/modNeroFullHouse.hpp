#pragma once

#include "../mod.hpp"

class NeroFullHouse : public Mod
{
public:
    NeroFullHouse() = default;

    static bool modEnabled;

    static uintptr_t jmp_ret1;
    static uintptr_t full_house;
    static uintptr_t helm_splitter;

    static uintptr_t jmp_ret2;

    static uintptr_t jmp_ret3;

    static uintptr_t jmp_ret4;

    std::string getModName() override { return "NeroFullHouse"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;
    void onGUIframe() override;
    // void onUpdateInput(hl::Input& input) override;

private:
    hl::Hooker hook1;
    hl::Hooker hook2;
    hl::Hooker hook3;
    hl::Hooker hook4;
};
