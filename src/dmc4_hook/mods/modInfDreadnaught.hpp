#pragma once

#include "../mod.hpp"

class InfDreadnaught : public Mod
{
public:
    InfDreadnaught() = default;

    static bool modEnabled;
    static uintptr_t _infDreadnaughtContinue;
	
	void toggle(bool enable);
    
	std::string getModName() override { return "InfDreadnaught"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;

private:
    hl::Hooker hook;
    hl::Patch patch;
};