#pragma once
#include "../mod.hpp"

class GraphicsSettings : public Mod {
public:
    GraphicsSettings() = default;
    std::string get_mod_name() override { return "GraphicsSettings"; };
    std::vector<std::string> get_search_terms() override {
        return {"disable prop fade", "disable bullet casing fade", "increased shadow resolution"
        };
    }
    // Mod::ModType get_mod_type() override { return SLOW; };

    static bool disable_prop_fade_enabled;
    void prop_fade_toggle(bool enable);

    static bool disable_bullet_casing_fade_enabled;
    void bullet_casing_fade_toggle(bool enable);

    static bool shadow_upgrade_enabled;
    void shadowres_toggle(bool enable);

    // void on_frame(fmilliseconds& dt) override;
    void on_gui_frame(int display) override;
    std::optional<std::string> on_initialize() override;
    // void on_game_pause(bool toggle) override;
    // bool on_message(HWND wnd, UINT message, WPARAM wParam, LPARAM lParam) override;
    void on_config_load(const utility::Config& cfg) override;
    void on_config_save(utility::Config& cfg) override;

private:
    std::unique_ptr<Patch> prop_fade_patch1;
    std::unique_ptr<Patch> bullet_casing_fade_patch1, bullet_casing_fade_patch2, bullet_casing_fade_patch3, bullet_casing_fade_patch4;
    std::unique_ptr<Patch> shadows_patch1, shadows_patch2;
    // std::unique_ptr<FunctionHook> hook1;
};
