#pragma once

#include "../mod.hpp"

#include "../utils/Timer.hpp"

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
	void onFrame(fmilliseconds& dt) override;
	void onTwitchCommand(std::size_t hash) override;

private:
	utils::Timer* m_timer;

	std::size_t m_command;
    hl::Patch patch1;
    hl::Patch patch2;
};