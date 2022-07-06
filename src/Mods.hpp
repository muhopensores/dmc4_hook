#pragma once

#include "Mod.hpp"
//#include <unordered_map>

class Mods {
public:
	Mods();
    ~Mods() {};
	//virtual ~Mods() {};

	std::optional<std::string> on_initialize(Mod::ModType type) const;

    void on_config_save();

	void on_frame(fmilliseconds& dt);
	void on_draw_ui(uint32_t hash);

    void on_update_input(utility::Input& input);
	void on_game_pause(bool toggle);
	bool on_message(HWND wnd, UINT message, WPARAM w_param, LPARAM l_param);
	void on_chat_command(const std::string& message);
    void on_draw_custom_imgui_window();
    void on_hotkey_tab(utility::Input& input);

	const auto& get_mods() const {
		return m_mods;
    };

private:
	std::vector<std::unique_ptr<Mod>> m_mods;
    std::vector<std::uint32_t> m_hash;
};