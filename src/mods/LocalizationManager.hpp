#pragma once

#include "Mod.hpp"

// Define ModName class that is a Mod
class LocalizationManager : public Mod {
public:
	LocalizationManager() = default;

	std::string get_mod_name() override { return "LocalizationManager"; };
	Mod::ModType get_mod_type() override { return SLOW; }
	// override onInitialize() method with routines required to initialize the mod.
	// return Mod::onInitialize() on success or error string on failure.
	std::optional<std::string> on_initialize() override;

	// callbacks

	void on_config_load(const utility::Config& cfg) override;
	
	void on_config_save(utility::Config& cfg) override;
	
	void on_gui_frame() override;
	
private:
};
ImFont* load_locale_and_imfont(const char* country_code) noexcept;