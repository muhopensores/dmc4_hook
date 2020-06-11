#pragma once

#include "../mod.hpp"

class InfDreadnought : public Mod
{
public:
    InfDreadnought() = default;

    static bool modEnabled;
    static uintptr_t _infDreadnoughtContinue;
	
	void toggle(bool enable);
    
	std::string getModName() override { return "InfDreadnought"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Patch patch;
};