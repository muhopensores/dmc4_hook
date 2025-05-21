#pragma once

#include "Mod.hpp"

class MutatorSelfAdvertisement : public Mod {
public:
	MutatorSelfAdvertisement() = default;

	std::string get_mod_name() override { return "MutatorSelfAdvertisement"; };

	std::optional<std::string> on_initialize() override;
	Mod::ModType get_mod_type() override { return SLOW; };
	void custom_imgui_window();
	void on_gui_frame(int display) override;
	void on_frame(fmilliseconds& dt) override;
    void on_reset() override;
    void after_reset() override;
	void on_config_load(const utility::Config& cfg) override;
	void on_config_save(utility::Config& cfg) override;
	ImColor rnd_hue(const glm::vec2& p);
private:
	PDIRECT3DTEXTURE9 m_texture_handle{ nullptr };
	ImColor m_bg_color{ 0.0f, 0.0f, 0.0f, 0.0f };
	ImColor m_tint_color{ 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec2 m_accel { 2.0f, 2.0f };
	glm::vec2 m_pos   { 512.0f, 512.0f };

	int m_texture_width{};
	int m_texture_height{};
};
