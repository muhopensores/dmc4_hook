#pragma once

#include "../mod.hpp"

class SkipWeapons : public Mod
{
public:
    SkipWeapons() = default;

	static bool skipPandora;
    static bool skipLucifer;
    static uintptr_t _skipDanteGunContinue;
    static uintptr_t _skipDanteSwordContinue;

    std::string getModName() override { return "SkipWeapons"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Hooker hook2;
};