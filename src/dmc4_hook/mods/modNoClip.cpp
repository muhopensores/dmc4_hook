#include "modNoClip.hpp"
#include "../utils/MessageDisplay.hpp"

#if 1
bool NoClip::modEnabled{ false };
int NoClip::hotkey{ NULL };

static void onTimerCallback() {
	NoClip::modEnabled = !NoClip::modEnabled;
}

std::optional<std::string> NoClip::onInitialize()
{
	m_timer = new utils::Timer(5.0f, onTimerCallback);
	m_command = std::hash<std::string>{}("\\" + getModName());
    return Mod::onInitialize();
}

void NoClip::toggle(bool enable)
{
    if (enable)
    {
        install_patch_offset(0x115BC4, patch1, "\x90\x90\x90", 3);
        install_patch_offset(0x10FA06, patch2, "\x01", 1);
    }
    else
    {
        patch1.revert();
        patch2.revert();
    }
}

void NoClip::onGUIframe()
{
    if (ImGui::Checkbox("Noclip", &modEnabled))
    {
        toggle(modEnabled);
    }
}

void NoClip::onTwitchCommand(std::size_t hash)
{
	HL_LOG_RAW("[TwitchCommand] got hash:%d our hash:%d\n", hash, m_command);
	if (hash == m_command) 
	{
		modEnabled = !modEnabled;
		toggle(modEnabled);
	}
}

void NoClip::onConfigLoad(const utils::Config& cfg)
{
    modEnabled = cfg.get<bool>("noclip").value_or(false);
    toggle(modEnabled);
    hotkey = cfg.get<int>("noclip_hotkey").value_or(0x74);
};

void NoClip::onConfigSave(utils::Config& cfg)
{
    cfg.set<bool>("noclip", modEnabled);
    cfg.set<int>("noclip_hotkey", hotkey);
};

void NoClip::onFrame(fmilliseconds& dt) 
{
	m_timer->tick(dt);
};

void NoClip::onUpdateInput(hl::Input& input)
{
    if (input.wentDown(hotkey))
    {
        if (modEnabled)
        {
            DISPLAY_MESSAGE("Noclip Off");
        }
        else
        {
            DISPLAY_MESSAGE("Noclip On");
        }
        modEnabled = !modEnabled;
        toggle(modEnabled);
    }
}

#endif
