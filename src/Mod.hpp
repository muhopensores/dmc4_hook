#pragma once
#include "imgui/imgui.h"
#include "utility/Config.hpp"
#include "utility/Timer.hpp"
#include "utility/Patch.hpp"
#include "utility/Input.hpp"
#include "utility/MessageDisplay.hpp"
#include "utility/MoFile.hpp"

#include "ModFramework.hpp"
#include "sdk/Devil4.hpp"

#include "Mutators.hpp"
#include "Console.hpp"

#define naked static __declspec(naked)
#define _(string) utility::text_lookup(string)

#include <chrono>

class Mod {
public:

    Mod() = default;
    virtual ~Mod() {};

    enum ModType {
        REGULAR,
        SLOW
    };
    float sameLineWidth     = 300.0f;
    float sameLineItemWidth = sameLineWidth / 2;
    float lineIndent        = 20.0f;

    //std::unique_ptr<utility::Hotkey> m_hotkey;
    std::vector<std::unique_ptr<utility::Hotkey>> m_hotkeys;

    virtual Mod::ModType get_mod_type() { return REGULAR; };

    virtual std::string get_mod_name() { return "UnknownMod"; };
    // can be used for ModValues, like Mod_ValueName
    virtual std::string generate_name(std::string_view name) { return std::string{ get_mod_name() } + "_" + name.data(); }

	void help_marker(const char* desc) {
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
			ImGui::TextUnformatted(desc);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
	}

    void install_patch_absolute(uintptr_t location, std::unique_ptr<Patch>& patch, const char* patch_bytes, uint8_t length) {
        spdlog::info("{}: Installing patch at {:x}.\n", get_mod_name().c_str(), location);
        patch.reset(nullptr);
        std::vector<int16_t> bytes;
        while (length > 0) {
            bytes.push_back((short)(*patch_bytes) & 0x00FF);
            patch_bytes++;
            length--;
        }
        patch = Patch::create(location, bytes, true);
        //patch.reset(Patch::create_raw(location, bytes, true));
    }

	void install_patch_offset(ptrdiff_t offset, std::unique_ptr<Patch>& patch, const char* patch_bytes, uint8_t length) {
        uintptr_t base = g_framework->get_module().as<uintptr_t>();
        uintptr_t location = base + offset;
		spdlog::info("{}: Installing patch at {:x}.\n", get_mod_name().c_str(), location);
        patch.reset(nullptr);
        std::vector<int16_t> bytes;
        while (length > 0) {
            bytes.push_back((short)(*patch_bytes) & 0x00FF);
            patch_bytes++;
            length--;
        }
        patch = Patch::create(location, bytes, true);
        //patch.reset(Patch::create_raw(location, bytes, true));
	}
    
    inline bool install_hook_offset(ptrdiff_t offset, std::unique_ptr<FunctionHook>& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
        uintptr_t base = g_framework->get_module().as<uintptr_t>();
        uintptr_t location = base + offset;

		spdlog::info("{}: Installing hook at {:x}.\n", get_mod_name().c_str(), location);
        assert(!hook);
        
        hook = std::make_unique<FunctionHook>(location, detour);
        if (!hook->create()) {
            spdlog::error("Failed to create hook!");
            return false;
        }

        if (next_instruction_offset) {
            if (ret) {
                *ret = location + next_instruction_offset;
            }
        }
        else {
            if (ret) {
                *ret = hook->get_original();
            }
        }
        return true;
    }
    
    inline bool install_hook_absolute(uintptr_t location, std::unique_ptr<FunctionHook>& hook, void* detour, uintptr_t* ret, ptrdiff_t next_instruction_offset = 0) {
        assert(!hook);
		spdlog::info("{}: Installing hook at {:x}.\n", get_mod_name().c_str(), location);
        hook = std::make_unique<FunctionHook>(location, detour);
        if (!hook->create()) {
            spdlog::error("Failed to create hook!");
            return false;
        }

        if (next_instruction_offset) {
            if (ret) {
                *ret = location + next_instruction_offset;
            }
        }
        else {
            if (ret) {
                *ret = hook->get_original();
            }
        }
        return true;
    }

	virtual std::optional<std::string> on_initialize() { return std::nullopt; }
	// should be called from d3d hook
	virtual void on_frame(fmilliseconds& dt) {}
	// called only when imgui window displays
	virtual void on_gui_frame(int display = 0) {}

	virtual void on_config_load(const utility::Config& cfg) {}
	virtual void on_config_save(utility::Config& cfg) {}
    virtual void on_update_input(utility::Input& input) {}

	//game specific callbacks
	virtual void on_game_pause(bool toggle) {};
	virtual bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param) { return true; };

    // texture stuff
    virtual void on_reset() {};
    virtual void after_reset() {};
	// twitch
	//virtual void on_twitch_command(std::size_t hash) {};
};