#pragma once

#include "../mod.hpp"

class EnemyTracker : public Mod {
public:
    EnemyTracker() = default;

    std::string get_mod_name() override { return "EnemyTracker"; };

    std::optional<std::string> on_initialize() override;

    static int get_enemy_specific_damage_offset(int enemy_id);
    static int get_enemy_specific_uCollision_offset(int enemy_id);

    static bool flyingAd;
    static bool flyingSpheres;

    void RenderExample();

    void on_gui_frame(int display) override;
    void on_frame(fmilliseconds& dt) override;
    void on_update_input(utility::Input& input) override;
    void on_config_load(const utility::Config& cfg);
    void on_config_save(utility::Config& cfg);

    Mod::ModType get_mod_type() override { return SLOW; };
    void on_reset() override;
    void after_reset() override;
private:
    std::unique_ptr<FunctionHook> hook;

    LPDIRECT3DDEVICE9 g_pd3dDevice{};
    PDIRECT3DTEXTURE9 m_texture_handle{};
    int m_texture_width{};
    int m_texture_height{};
};
