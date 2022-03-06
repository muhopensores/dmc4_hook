#pragma once

#include "hacklib/Main.h"
#include "hacklib/Patch.h"
#include "hacklib/Hooker.h"
#include "hacklib/ConsoleEx.h"
#include "hacklib/Input.h"

#include "utils/Config.hpp"
#include "utils/RingBuffer.hpp"

class Mods;

class hlMain* GetMain();

class hlMain : public hl::Main
{
public:

	const auto& getMods() const {
		return m_mods;
	}
	bool isReady() const {
		return m_gameDataInitialized;
	}
    
	//functions
    void GamePause();

    // hacklib init() / step() functions
    bool init() override;
    bool step() override;
	void shutdown() override;

    // define our pointers here, but init and search for them in init()
    // pointer definitions
    uintptr_t modBase;

    const hl::IHook* g_PresentHook;
    const hl::IHook* g_EndSceneHook;
    const hl::IHook* g_ResetHook;

    hl::Hooker m_hooker, m_hook;
    hl::Input input;
    hl::ConsoleEx m_con;

	void saveSettings();

private:
	std::atomic<bool> m_gameDataInitialized{ false };
	std::unique_ptr<Mods> m_mods;
	std::string m_confPath;
};
