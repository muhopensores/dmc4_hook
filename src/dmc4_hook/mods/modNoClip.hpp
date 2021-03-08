#pragma once

#include "../mod.hpp"

class NoClip : public Mod
{
public:
    NoClip() = default;

    static bool modEnabled;

    void toggle(bool enable);

    std::string getModName() override { return "NoClip"; };

    std::optional<std::string> onInitialize() override;

    void onConfigLoad(const utils::Config& cfg) override;
    void onConfigSave(utils::Config& cfg) override;

    void onGUIframe() override;
	void onTwitchCommand(std::size_t hash) override;

private:
	std::size_t m_command;
    hl::Patch patch1;
    hl::Patch patch2;
};